#include "ModuleResourcesManager.h"
#include "FileSystem.h"
#include "Application.h"
#include "Random.h"
#include "ModuleImporter.h"
#include "ModuleExporter.h"
#include "ModuleScene.h"
#include "ResourceTexture.h"
#include "ResourceMesh.h"
#include "Resource3dObject.h"
#include "ResourceScript.h"
#include "ResourceScene.h"
#include "ResourcePrefab.h"
#include "ResourceAnimation.h"
#include "ResourceBone.h"
#include "ResourceAudio.h"
#include "ResourceAnimationGraph.h"
#include "ResourceShader.h"
#include "Applog.h"
#include "Mesh.h"

// Temporal debug purposes
#include "ModuleInput.h"
#include <experimental/filesystem>
#include <iostream>

ModuleResourcesManager::ModuleResourcesManager(Application* app, bool start_enabled): Module(app, start_enabled)
{
	name = "resource manager";
}


ModuleResourcesManager::~ModuleResourcesManager()
{

}

bool ModuleResourcesManager::Init(const JSON_Object * config)
{
	update_ratio = 1000;
	return true;
}

bool ModuleResourcesManager::Start()
{
	GeneratePrimitiveResources();

	if (!App->is_game || App->debug_game)
		GenerateLibraryAndMeta();
	else
		GenerateResources();

	CompileAndGenerateScripts();
	update_timer.Start();
	App->exporter->AssetsToLibraryJSON();

	return true;
}

update_status ModuleResourcesManager::Update(float dt)
{
	if (!App->is_game || App->debug_game)
	{
		ManageUITextures();
		if (update_timer.Read() > update_ratio) {
			ManageAssetModification();

			if (reloadVM) {
				ReloadVM();
				reloadVM = false;
			}
			update_timer.Start();
		}
	}

	return UPDATE_CONTINUE;
}

update_status ModuleResourcesManager::PostUpdate(float dt)
{
	update_status ret = UPDATE_CONTINUE;
	// Debug purpuses
	if (cleanResources) {
		CleanMeta();
		CleanLibrary();
		cleanResources = false;
		App->CLOSE_APP();
	}

	return ret;
}




Resource * ModuleResourcesManager::newResource(resource_deff deff) {
	Resource* ret = nullptr;

	switch (deff.type) {
	case R_TEXTURE: ret = (Resource*) new ResourceTexture(deff); break;
	case R_MESH: ret = (Resource*) new ResourceMesh(deff); break;
	case R_3DOBJECT: ret = (Resource*) new Resource3dObject(deff); break;
	case R_SCRIPT: ret = (Resource*) new ResourceScript(deff); break;
	case R_SCENE: ret = (Resource*) new ResourceScene(deff); break;
	case R_PREFAB: ret = (Resource*) new ResourcePrefab(deff); break;
	case R_ANIMATION:
		ret = (Resource*) new ResourceAnimation(deff);
		break;
	case R_BONE:
		ret = (Resource*) new ResourceBone(deff);
		break;
	case R_AUDIO: ret = (Resource*) new ResourceAudio(deff); break;
	case R_ANIMATIONGRAPH:
		ret = (Resource*) new ResourceAnimationGraph(deff);
		break;
	case R_SHADER:
		ret = (Resource*) new ResourceShader(deff);
		break;
	}

	if (ret)
		resources[deff.uuid] = ret;

	return ret;
}

void ModuleResourcesManager::ManageUITextures() {
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_TEXTURE) {
			ResourceTexture* res_tex = (ResourceTexture*)(*it).second;
			if (res_tex->IsLoaded() && res_tex->components_used_by == 0) {
				if (res_tex->drawn_in_UI)	res_tex->drawn_in_UI = false;
				else						res_tex->UnloadFromMemory();
			}

			res_tex->drawn_in_UI = 0;
		}
	}
}

void ModuleResourcesManager::GeneratePrimitiveResources() {
	ResourceMesh* cube = new ResourceMesh(resource_deff());
	cube->mesh = new Mesh(PrimitiveTypes::Primitive_Cube);
	primitive_resources[Primitive_Cube] = cube;

	ResourceMesh* plane = new ResourceMesh(resource_deff());
	plane->mesh = new Mesh(PrimitiveTypes::Primitive_Plane);
	primitive_resources[Primitive_Plane] = plane;


	ResourceMesh* sphere = new ResourceMesh(resource_deff());
	sphere->mesh = new Mesh(PrimitiveTypes::Primitive_Sphere);
	primitive_resources[Primitive_Sphere] = sphere;

	ResourceMesh* cylinder = new ResourceMesh(resource_deff());
	cylinder->mesh = new Mesh(PrimitiveTypes::Primitive_Cylinder);
	primitive_resources[Primitive_Cylinder] = cylinder;
}


void ModuleResourcesManager::GenerateLibraryAndMeta()
{
	using std::experimental::filesystem::recursive_directory_iterator;
	for (auto& it : recursive_directory_iterator(ASSETS_FOLDER)) {
		if (it.status().type() == std::experimental::filesystem::v1::file_type::directory) // If the path is a directory, ignore it
			continue;

		resource_deff new_res_deff;
		if (ManageFile(it.path().generic_string(), new_res_deff))
			newResource(new_res_deff);
	}
}

void ModuleResourcesManager::ManageAssetModification() {
	using std::experimental::filesystem::recursive_directory_iterator;
	for (auto& it : recursive_directory_iterator(ASSETS_FOLDER)) {
		if (it.status().type() == std::experimental::filesystem::v1::file_type::directory) // If the path is a directory, ignore it
			continue;

		resource_deff deff;
		if (ManageFile(it.path().generic_string(), deff)) {
			switch (deff.requested_update) {
			case R_CREATE:
				newResource(deff);
				break;
			case R_UPDATE:
				resources[deff.uuid]->UnloadFromMemory();
				resources[deff.uuid]->LoadToMemory();
				break;
			}
			if (deff.requested_update == R_CREATE || deff.requested_update == R_UPDATE) {
				if (deff.type == R_SCRIPT || deff.type==R_SHADER)
					reloadVM = true;
			}


		}
		else {
			if (deff.requested_update == R_DELETE) {
				auto it = resources.find(deff.uuid);
				if (it != resources.end()) {
					Resource* delete_res = resources[deff.uuid];
					delete_res->UnloadFromMemory();
					delete delete_res;
					resources.erase(it);
				}
			}
		}
	}

}

bool ModuleResourcesManager::ManageFile(std::string file_path, resource_deff& deff)
{
	std::string path, name, extension;
	path = name = extension = file_path;	// Separate path, name and extension	
	App->fs.getExtension(extension);
	App->fs.getPath(path);
	App->fs.getFileNameFromPath(name);

	// Manage meta
	if (extension == META_EXTENSION) { // If it is a meta file
		deff = ManageMeta(path, name, extension);  // If has a corresponding asset, continue, else delete file from library and delete .meta
		return false;
	}
	// Manage asset
	deff = ManageAsset(path, name, extension);
	return true;
}

resource_deff ModuleResourcesManager::ManageMeta(std::string path, std::string name, std::string extension) {

	std::string full_meta_path = path + name + extension;
	resource_deff ret;

	std::string full_asset_path = path + name;

	if (App->fs.ExistisFile(full_asset_path.c_str())) // If file exists, MISSION ACOMPLISHED, return
	{
		ret.requested_update = R_NOTHING;
	}
	else {	
		JSON_Value* meta = json_parse_file(full_meta_path.c_str());
		std::string full_binary_path = json_object_get_string(json_object(meta), "binary_path");		// Delete file from library using the meta reference that points to it
		ResourceType r_type = type2enumType(json_object_get_string(json_object(meta), "type"));
		ret.uuid = json_object_get_number(json_object(meta), "resource_uuid");
		ret.requested_update = R_DELETE;
		if (r_type == R_3DOBJECT) {									// Handle meshes delete when a scene is deleted
			CleanMeshesFromLibrary(full_binary_path.c_str());
		}
		App->fs.DestroyFile(full_binary_path.c_str());	// Destroy binary			
		App->fs.DestroyFile(full_meta_path.c_str()); // Destroy meta
		json_value_free(meta);
	}

	return ret;
}

resource_deff ModuleResourcesManager::ManageAsset(std::string path, std::string name, std::string extension) {

	JSON_Value* meta;
	int file_last_mod = 0;
	// Needed for collisioning .meta against asset
	std::string full_meta_path = path + name + extension + META_EXTENSION; // TODO: Add original extension to .meta
	std::string full_asset_path = path + name + extension;
	// Needed for import
	ResourceType enum_type;
	std::string uuid_str;
	std::string binary_path;
	uint uuid_number = 0;
	resource_deff deff;
	if (App->fs.ExistisFile(full_meta_path.c_str())) {		// Check if .meta file exists
		// EXISTING RESOURCE WITH POSSIBLE MODIFICATION
		meta = json_parse_file(full_meta_path.c_str());
		file_last_mod = App->fs.getFileLastTimeMod(full_asset_path.c_str());
		binary_path = json_object_get_string(json_object(meta), "binary_path");
		enum_type = type2enumType(json_object_get_string(json_object(meta), "type"));
		uuid_number = json_object_get_number(json_object(meta), "resource_uuid");
		uuid_str = uuid2string(uuid_number);
		deff.requested_update = R_UPDATE;
		if (json_object_get_number(json_object(meta), "timeCreated") == file_last_mod) { // Check if the last time that was edited is the .meta timestamp
			// EXISTING RESOURCE WITH NO MODIFICATION
			deff.set(uuid_number, enum_type, binary_path, full_asset_path);
			deff.requested_update = R_NOTHING;
			json_value_free(meta);
			return deff;																		// Existing meta, and timestamp is the same, generate a resource to store it in the code
		}
	}
	else {
		// NEW RESOURCE IN FOLDER
		App->fs.CreateEmptyFile(full_meta_path.c_str());	// If .meta doesn't exist, generate it
		meta = json_value_init_object();
		std::string str_type;
		uuid_number = random32bits();
		str_type = assetExtension2type(extension.c_str());
		enum_type = type2enumType(str_type.c_str());
		uuid_str = uuid2string(uuid_number);
		binary_path = App->fs.getPathFromLibDir(enumType2libDir(enum_type)) + uuid_str;
		binary_path += ((str_type == "vertexShader") ? VERTEXSHADER_EXTENSION : ((str_type == "fragmentShader") ? FRAGMENTSHADER_EXTENSION : enumType2binaryExtension(enum_type)));
		file_last_mod = App->fs.getFileLastTimeMod(full_asset_path.c_str());
		json_object_set_number(json_object(meta), "resource_uuid", uuid_number); // Brand new uuid
		json_object_set_string(json_object(meta), "type", str_type.c_str()); // Brand new time
		json_object_set_string(json_object(meta), "binary_path", binary_path.c_str()); // Brand new binary path
		deff.requested_update = R_CREATE;
	}

	// If meta didn't exist, or existed but the asset was changed, 
	json_object_set_number(json_object(meta), "timeCreated", file_last_mod);	// Brand new / Updated time modification
	json_serialize_to_file_pretty(meta, full_meta_path.c_str());
	json_value_free(meta);

	switch (enum_type) {
	case R_TEXTURE:
		App->importer->ImportTexture(full_asset_path.c_str(), uuid_str);
		break;
	case R_3DOBJECT:
		App->importer->ImportScene(full_asset_path.c_str(), uuid_str);
		break;
	case R_SCRIPT:
		App->importer->ImportScript(full_asset_path.c_str(), uuid_str);
		break;
	case R_SHADER:
		App->importer->ImportShader(full_asset_path.c_str(), uuid_str);
		break;
	case R_PREFAB:
		App->fs.copyFileTo(full_asset_path.c_str(), LIBRARY_PREFABS, PREFAB_EXTENSION, uuid_str.c_str()); // Copy the file to the library
		break;
	case R_SCENE:
		App->fs.copyFileTo(full_asset_path.c_str(), LIBRARY_SCENES, SCENE_EXTENSION, uuid_str.c_str()); // Copy the file to the library
		break;
	case R_AUDIO:
		App->fs.copyFileTo(full_asset_path.c_str(), LIBRARY_AUDIO, AUDIO_EXTENSION, uuid_str.c_str()); // Copy the file to the library
	case R_ANIMATIONGRAPH:
		App->fs.copyFileTo(full_asset_path.c_str(), LIBRARY_GRAPHS, GRAPH_EXTENSION, uuid_str.c_str()); // Copy the file to the library
		break;
	}
	// Meta generated and file imported, create resource in code
	deff.set(uuid_number, enum_type, binary_path, full_asset_path);

	return deff;
}

void ModuleResourcesManager::GenerateResources()
{
	JSON_Value* assets = json_parse_file("Library/assetsUUIDs.json");

	GenerateFromMapFile(assets, R_MESH);
	GenerateFromMapFile(assets, R_TEXTURE);
	GenerateFromMapFile(assets, R_SCENE);
	GenerateFromMapFile(assets, R_PREFAB);
	GenerateFromMapFile(assets, R_SCRIPT);
	GenerateFromMapFile(assets, R_ANIMATION);
	GenerateFromMapFile(assets, R_BONE);
	GenerateFromMapFile(assets, R_AUDIO);
	GenerateFromMapFile(assets, R_SHADER);
	GenerateFromMapFile(assets, R_ANIMATIONGRAPH);
	GenerateFromMapFile(assets, R_UI);
}

void ModuleResourcesManager::GenerateFromMapFile(JSON_Value* file, ResourceType type)
{
	std::string name;
	std::string path;
	std::string extension;
	switch (type)
	{
	case R_MESH:
		name = "Meshes";
		path = MESHES_FOLDER;
		extension = OWN_MESH_EXTENSION;
		break;
	case R_TEXTURE:
		name = "Textures";
		path = TEXTURES_FOLDER;
		extension = DDS_EXTENSION;
		break;
	case R_SCENE:
		name = "Scenes";
		path = SCENES_FOLDER;
		extension = SCENE_EXTENSION;
		break;
	case R_PREFAB:
		name = "Prefabs";
		path = PREFABS_FOLDER;
		extension = PREFAB_EXTENSION;
		break;
	case R_SCRIPT:
		name = "Scripts";
		path = SCRIPTS_FOLDER;
		extension = JSON_EXTENSION;
		break;
	case R_ANIMATION:
		name = "Animations";
		path = ANIMATIONS_FOLDER;
		extension = OWN_ANIMATION_EXTENSION;
		break;
	case R_BONE:
		name = "Bones";
		path = BONES_FOLDER;
		extension = OWN_BONE_EXTENSION;
		break;
	case R_AUDIO:
		name = "Audio";
		path = AUDIO_FOLDER;
		extension = AUDIO_EXTENSION;
		break;
	case R_ANIMATIONGRAPH:
		name = "AnimationGraphs";
		path = GRAPHS_FOLDER;
		extension = GRAPH_EXTENSION;
		break;
	case R_SHADER:
		name = "Shaders";
		path = SHADERS_FOLDER;
		break;
	case R_UI:
		name = "UI";
		path = TEXTURES_FOLDER;
		extension = DDS_EXTENSION;
		type = R_TEXTURE; // As UI folder contains textures, we can add them as TEXTURES resources
		break;
	}

	//Generate resources
	JSON_Array* meshes = json_object_get_array(json_object(file), name.c_str());
	for (int i = 0; i < json_array_get_count(meshes); i++) {
		JSON_Object* meshMap = json_array_get_object(meshes, i);
		resource_deff deff;
		deff.asset = json_object_get_string(meshMap, "name");
		deff.uuid = json_object_get_number(meshMap, "uuid");
		deff.type = type;
		if (extension == "")
			extension = json_object_get_string(meshMap, "extension");
		deff.binary = path + std::to_string(deff.uuid) + extension;

		newResource(deff);
	}
}


void ModuleResourcesManager::LoadResource(uint uuid) {
	auto it = resources.find(uuid);
	if (it != resources.end()) {
		resources[uuid]->LoadToMemory();
	}
	else
		app_log->AddLog("WARNING: Trying to load non existing resource");
}

void ModuleResourcesManager::CleanMeshesFromLibrary(std::string prefab_binary)
{
	JSON_Value* scene = json_parse_file(prefab_binary.c_str());
	JSON_Array* objects = json_object_get_array(json_object(scene), "Game Objects");

	// Delete meshes from library
	for (int i = 0; i < json_array_get_count(objects); i++) {
		JSON_Object* obj_deff = json_array_get_object(objects, i);
		JSON_Array* components = json_object_get_array(obj_deff, "Components");
		std::string mesh_binary;
		// Iterate components and look for the mesh
		bool mesh_found = false;
		for (int a = 0; a < json_array_get_count(components); a++) {
			JSON_Object* mesh_resource = json_array_get_object(components, a);
			std::string type = json_object_get_string(mesh_resource, "type");
			if (type == "mesh") {
				mesh_binary = json_object_get_string(mesh_resource, "mesh_binary_path");
				mesh_found = true;
				break;
			}
		}

		if (mesh_found) {					  // If a mesh was found destroy its binary
			App->fs.DestroyFile(mesh_binary.c_str());
		}
	}
	
}




void ModuleResourcesManager::CompileAndGenerateScripts()
{
	std::vector<ResourceScript*> scripts;

	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_SCRIPT) {
			scripts.push_back((ResourceScript*)(*it).second);
		}
	}
	for (auto it = scripts.begin(); it != scripts.end(); it++) {
		(*it)->Compile();
	}
	for (auto it = scripts.begin(); it != scripts.end(); it++) {
		(*it)->GenerateScript();
	}

}

Resource * ModuleResourcesManager::getResource(uint uuid) {

	Resource* ret = nullptr;
	auto it = resources.find(uuid);
	if (it != resources.end()) {
		ret = resources[uuid];
	}
	//else
	//	app_log->AddLog("WARNING: Asking for non existing resource");

	return ret;
}

Resource * ModuleResourcesManager::getPrimitiveMeshResource(PrimitiveTypes type) {
	return primitive_resources[type];
}

int ModuleResourcesManager::assignResource(uint uuid) {

	auto it = resources.find(uuid);
	if (it != resources.end()) {
		resources[uuid]->components_used_by++;
		if (!resources[uuid]->IsLoaded()) {
			resources[uuid]->LoadToMemory();
		}
	}
	else
		app_log->AddLog("WARNING: Trying to assing non existing resource");

	return 0;
}

int ModuleResourcesManager::deasignResource(uint uuid) {
	auto it = resources.find(uuid);
	if (it != resources.end()) {
		resources[uuid]->components_used_by--;
		if (resources[uuid]->components_used_by == 0) {
			resources[uuid]->UnloadFromMemory();
		}
	}
	else
		app_log->AddLog("WARNING: Trying to deasign non existing resource");

	return 0;
}

void ModuleResourcesManager::Load3dObjectToScene(const char * file) {
	std::string full_meta_path = file;
	full_meta_path += META_EXTENSION;
	uint resource_uuid = -1;

	if (App->fs.ExistisFile(full_meta_path.c_str())) {
		JSON_Value* meta = json_parse_file(full_meta_path.c_str());
		resource_uuid = json_object_get_number(json_object(meta), "resource_uuid");
		json_value_free(meta);

		resources[resource_uuid]->LoadToMemory();
	}
	else{
		app_log->AddLog("%s has no .meta, can't be loaded", file);
		return;
	}


	
}

uint ModuleResourcesManager::getResourceUuid(const char * file) {
	std::string full_meta_path = file;
	full_meta_path += META_EXTENSION;
	uint ret = 0;
	if (App->fs.ExistisFile(full_meta_path.c_str())) {
		JSON_Value* meta = json_parse_file(full_meta_path.c_str());
		ret = json_object_get_number(json_object(meta), "resource_uuid");
		json_value_free(meta);
	}
	else {
		app_log->AddLog("%s has no .meta, can't be loaded", file);
	}

	return ret;
}

uint ModuleResourcesManager::getResourceUuid(const char* name, ResourceType type)
{
	if (name != nullptr)
	{
		for (auto it = resources.begin(); it != resources.end(); it++)
		{
			if ((*it).second->type == type && (*it).second->asset == name)
			{
				return (*it).second->uuid;
			}
		}
	}

	return 0;
}

uint ModuleResourcesManager::getMeshResourceUuid(const char * Parent3dObject, const char * name) {

	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_MESH){
			ResourceMesh* res_mesh = (ResourceMesh*)(*it).second;
			if (res_mesh->Parent3dObject == Parent3dObject && res_mesh->asset == name) {
				return res_mesh->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getTextureResourceUuid(const char * name) {

	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_TEXTURE) {
			ResourceMesh* res_tex = (ResourceMesh*)(*it).second;
			if (res_tex->asset == name) {
				return res_tex->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getAnimationResourceUuid(const char * Parent3dObject, const char * name)
{
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_ANIMATION) {
			ResourceAnimation* res_anim = (ResourceAnimation*)(*it).second;
			if (res_anim->Parent3dObject == Parent3dObject && res_anim->asset == name) {
				return res_anim->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getAnimationResourceUuid(const char * name) {
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_ANIMATION) {
			ResourceAnimation* res_anim = (ResourceAnimation*)(*it).second;
			if (res_anim->asset == name) {
				return res_anim->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getBoneResourceUuid(const char * Parent3dObject, const char * name)
{
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_BONE) {
			ResourceBone* res_bone = (ResourceBone*)(*it).second;
			if (res_bone->Parent3dObject == Parent3dObject && res_bone->asset == name) {
				return res_bone->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getAudioResourceUuid(const char* name)
{
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_AUDIO) {
			ResourceAudio* res_audio = (ResourceAudio*)(*it).second;
			if (res_audio->asset == name) {
				return res_audio->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getScriptResourceUuid(const char* name)
{
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_SCRIPT) {
			ResourceScript* res_script = (ResourceScript*)(*it).second;
			if (res_script->asset == name) {
				return res_script->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getAnimationGraphResourceUuid(const char * Parent3dObject, const char * name)
{
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_ANIMATIONGRAPH) {
			ResourceAnimation* res_anim = (ResourceAnimation*)(*it).second;
			if (res_anim->Parent3dObject == Parent3dObject && res_anim->asset == name) {
				return res_anim->uuid;
			}
		}
	}
	return 0;
}

uint ModuleResourcesManager::getShaderResourceUuid(const char * name)
{
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_SHADER) {
			ResourceShader* res_script = (ResourceShader*)(*it).second;
			if (res_script->asset == name) {
				return res_script->uuid;
			}
		}
	}
	return 0;
}

void ModuleResourcesManager::CleanMeta() {
	using std::experimental::filesystem::recursive_directory_iterator;
	for (auto& it : recursive_directory_iterator(ASSETS_FOLDER)) {
		if (it.status().type() == std::experimental::filesystem::v1::file_type::directory) // If the path is a directory, ignore it
			continue;

		std::string path, name, extension;
		path = name = extension = it.path().generic_string();	// Separate path, name and extension	
		App->fs.getExtension(extension);
		App->fs.getPath(path);
		App->fs.getFileNameFromPath(name);

		if (extension == META_EXTENSION)
			App->fs.DestroyFile((path + name + extension).c_str());

	}
}

void ModuleResourcesManager::CleanLibrary()
{
	using std::experimental::filesystem::recursive_directory_iterator;
	for (auto& it : recursive_directory_iterator(LIBRARY_FOLDER)) {
		if (it.status().type() == std::experimental::filesystem::v1::file_type::directory) // If the path is a directory, ignore it
			continue;
		App->fs.DestroyFile(it.path().generic_string().c_str());
	}
}

void ModuleResourcesManager::ReloadVM()
{
	JSON_Value* scene = App->scene->serializeScene(); // Save scene with all the editor variables

	App->scene->ClearScene(); //Release handle and data from components script
	ReleaseResourcesScriptHandles(); // Release handles and data from resources
	App->scripting->CleanUp();			// Delete VM
	App->scripting->Init(nullptr);		// Create new VM
	CompileAndGenerateScripts();		// Create handles and data for resources


	App->scene->loadSerializedScene(scene); // Create handles and data for components script and assign editor variables
	json_value_free(scene);
}

void ModuleResourcesManager::ReleaseResourcesScriptHandles()
{
	std::vector<ResourceScript*> scripts;

	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_SCRIPT) {
			scripts.push_back((ResourceScript*)(*it).second);
		}
	}
	for (auto it = scripts.begin(); it != scripts.end(); it++) {
		(*it)->CleanUp();
	}
}

void ModuleResourcesManager::getMeshResourceList(std::list<resource_deff>& meshes) {
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		if ((*it).second->type == R_MESH) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			meshes.push_back(deff);
		}
	}
}

void ModuleResourcesManager::getScriptResourceList(std::list<resource_deff>& scripts) {
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		if ((*it).second->type == R_SCRIPT) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			scripts.push_back(deff);
		}
	}
}

void ModuleResourcesManager::getAnimationResourceList(std::list<resource_deff>& animations)
{
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		if ((*it).second->type == R_ANIMATION) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			animations.push_back(deff);
		}
	}
}

void ModuleResourcesManager::getAnimationGraphResourceList(std::list<resource_deff>& graphs)
{
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		if ((*it).second->type == R_ANIMATIONGRAPH) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			graphs.push_back(deff);
		}
	}
}

void ModuleResourcesManager::getSceneResourceList(std::list<resource_deff>& scenes, std::list<resource_deff> ignore)
{
	for (auto it = resources.begin(); it != resources.end(); ++it)
	{
		bool exist = false;
		for (auto it_s = ignore.begin(); it_s != ignore.end(); ++it_s)
		{
			if ((*it).second->asset == (*it_s).asset)
			{
				exist = true;
				break;
			}
		}
		if (!exist && (*it).second->type == R_SCENE) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			scenes.push_back(deff);
		}
	}
}

void ModuleResourcesManager::getAudioResourceList(std::list<resource_deff>& audio)
{
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		if ((*it).second->type == R_AUDIO) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			audio.push_back(deff);
		}
	}
}

void ModuleResourcesManager::getShaderResourceList(std::list<resource_deff>& shader)
{
	for (auto it = resources.begin(); it != resources.end(); ++it) {
		if ((*it).second->type == R_SHADER) {
			Resource* curr = (*it).second;
			resource_deff deff(curr->uuid, curr->type, curr->binary, curr->asset);
			shader.push_back(deff);
		}
	}
}

std::string ModuleResourcesManager::getPrefabPath(const char * prefab_name) {
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_PREFAB) {
			ResourcePrefab* curr = (ResourcePrefab*)(*it).second;
			if(curr->prefab_name == prefab_name)
				return curr->binary;
		}
	}

	app_log->AddLog("%s prefab could not be found", prefab_name);
	return std::string();
}

std::string ModuleResourcesManager::getScenePath(const char * scene_name) {
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if ((*it).second->type == R_SCENE) {
			ResourceScene* curr = (ResourceScene*)(*it).second;
			if (curr->scene_name == scene_name)
				return curr->asset;
		}
	}

	app_log->AddLog("%s scene could not be found", scene_name);
	return std::string();
}

std::string ModuleResourcesManager::uuid2string(uint uuid) {
	return std::to_string(uuid);
}

const char * ModuleResourcesManager::assetExtension2type(const char * _extension) {

	char* ret = "unknown";

	std::string extension = _extension;

	if (extension == ".FBX" || extension == ".fbx" || extension == ".DAE" || extension == ".dae" || extension == ".blend" || extension == ".3ds" || extension == ".obj"
		|| extension == ".gltf" || extension == ".glb" || extension == ".dxf" || extension == ".x")
		ret = "3dobject";
	else if (extension == ".bmp" || extension == ".dds" || extension == ".jpg" || extension == ".pcx" || extension == ".png"
		|| extension == ".raw" || extension == ".tga" || extension == ".tiff")
		ret = "texture";
	else if (extension == ".wren")
		ret = "script";
	else if (extension == ".json")
		ret = "json";
	else if (extension == ".meta")
		ret = "meta";
	else if (extension == ".prefab")
		ret = "prefab";
	else if (extension == ".scene")
		ret = "scene";
	else if (extension == ".bnk")
		ret = "audio";
	else if (extension == GRAPH_EXTENSION)
		ret = "graph";
	else if (extension == VERTEXSHADER_EXTENSION)
		ret = "vertexShader";
	else if (extension == FRAGMENTSHADER_EXTENSION)
		ret = "fragmentShader";

	return ret;
}

ResourceType ModuleResourcesManager::type2enumType(const char * type) {
	ResourceType ret = R_UNKNOWN;
	std::string str_type = type;

	if (str_type == "3dobject")
		ret = R_3DOBJECT;
	if (str_type == "texture")
		ret = R_TEXTURE;
	if (str_type == "script")
		ret = R_SCRIPT;
	if (str_type == "prefab")
		ret = R_PREFAB;
	if (str_type == "scene")
		ret = R_SCENE;
	if (str_type == "audio")
		ret = R_AUDIO;
	if (str_type == "graph")
		ret = R_ANIMATIONGRAPH;
	if (str_type == "vertexShader")
		ret = R_SHADER;
	if (str_type == "fragmentShader")
		ret = R_SHADER;

	return ret;
}

const char * ModuleResourcesManager::enumType2binaryExtension(ResourceType type) {
	const char* ret = "";
	switch (type) {
		case R_TEXTURE:
			ret = ".dds";
			break;
		case R_MESH:
			ret = ".kr";
			break;
		case R_SCENE:
			ret = ".scene";
			break;
		case R_PREFAB:
			ret = ".prefab";
			break;
		case R_3DOBJECT:
		case R_SCRIPT:
			ret = ".json";
			break;
		case R_AUDIO:
			ret = ".bnk";
			break;
		case R_ANIMATIONGRAPH:
			ret = GRAPH_EXTENSION;
			break;
	}

	return ret;
}

lib_dir ModuleResourcesManager::enumType2libDir(ResourceType type) {
	lib_dir ret = NO_LIB;
	switch (type) {
	case R_TEXTURE:
		ret = LIBRARY_TEXTURES;
		break;
	case R_MESH:
		ret = LIBRARY_MESHES;
		break;
	case R_3DOBJECT:
		ret = LIBRARY_3DOBJECTS;
		break;
	case R_SCRIPT:
		ret = LIBRARY_SCRIPTS;
		break;
	case R_PREFAB:
		ret = LIBRARY_PREFABS;
		break;
	case R_SCENE:
		ret = LIBRARY_SCENES;
		break;
	case R_AUDIO:
		ret = LIBRARY_AUDIO;
		break;
	case R_ANIMATIONGRAPH:
		ret = LIBRARY_GRAPHS;
		break;
	case R_SHADER:
		ret = LIBRARY_SHADERS;
		break;
	}
	return ret;
}


bool ModuleResourcesManager::CleanUp() {
	// Unload all memory from resources and delete resources
	for (auto it = resources.begin(); it != resources.end(); it++) {
		if (Resource* resource = (*it).second) {
			if (resource->IsLoaded())
				resource->UnloadFromMemory();
			delete resource;
		}
	}
	resources.clear();

	for (auto it = primitive_resources.begin(); it != primitive_resources.end(); it++) {
		Resource* resource = (*it).second;
		resource->UnloadFromMemory();
		delete resource;
	}

	return true;
}