 #include "ModuleUI.h"
#include "Globals.h"
#include "Application.h"
#include "ModuleScene.h"
#include "ModuleCamera3D.h"
#include "ModuleRenderer3D.h"
#include "ModuleInput.h"
#include "ModuleWindow.h"
#include "ModuleImporter.h"
#include "ModuleAudio.h"
#include "Applog.h"

#include "ImGui/imgui_impl_sdl.h"
#include "ImGui/imgui_impl_opengl2.h"
#include "ImGui/imgui_internal.h"

#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentAABB.h"

#include "Random.h"
#include "VRAM.h"
#include "WinItemDialog.h" 

#include "Assimp/include/version.h"

#include "glew-2.1.0\include\GL\glew.h"
#include "SDL\include\SDL_opengl.h"
#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment( lib, "glew-2.1.0/lib/glew32.lib")
#pragma comment( lib, "glew-2.1.0/lib/glew32s.lib")


ModuleUI::ModuleUI(Application* app, bool start_enabled) : Module(app, start_enabled) {
	name = "gui";
}


ModuleUI::~ModuleUI() {
}

bool ModuleUI::Init(const JSON_Object& config) {
	
	SDL_GL_SetSwapInterval(1); // Enable vsync

							   // Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	ImGui_ImplSDL2_InitForOpenGL(App->window->main_window->window, App->renderer3D->getContext());
	ImGui_ImplOpenGL2_Init();

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	LoadConfig(config);


	return true;
}

bool ModuleUI::Start()
{
	io = &ImGui::GetIO();

	ui_textures[PLAY]		= (Texture*)App->importer->Import("Play.png", I_TEXTURE);
	ui_textures[PAUSE]		= (Texture*)App->importer->Import("Pause.png", I_TEXTURE);
	ui_textures[STOP]		= (Texture*)App->importer->Import("Stop.png", I_TEXTURE);
	ui_textures[NO_TEXTURE] = (Texture*)App->importer->Import("no_texture.png", I_TEXTURE);

	//ui_fonts[TITLES]				= io->Fonts->AddFontFromFileTTF("Fonts/title.ttf", 16.0f);
	ui_fonts[REGULAR]				= io->Fonts->AddFontFromFileTTF("Fonts/regular.ttf", 18.0f);
	//ui_fonts[REGULAR_BOLD]			= io->Fonts->AddFontFromFileTTF("Fonts/regular_bold.ttf", 18.0f);
	//ui_fonts[REGULAR_ITALIC]		= io->Fonts->AddFontFromFileTTF("Fonts/regular_italic.ttf", 18.0f);
	//ui_fonts[REGULAR_BOLDITALIC]	= io->Fonts->AddFontFromFileTTF("Fonts/regular_bold_italic.ttf", 18.0f);
	
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	docking_background = true;
	close_app = false;

	return true;
}

update_status ModuleUI::PreUpdate(float dt) {

	// Start the ImGui frame
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame(App->window->main_window->window);
	ImGui::NewFrame();

	return UPDATE_CONTINUE;
}

update_status ModuleUI::Update(float dt) {


	InvisibleDockingBegin();

	if (App->input->GetKey(SDL_SCANCODE_C) == KEY_DOWN) 
		open_tabs[CONFIGURATION] = !open_tabs[CONFIGURATION];


	if (open_tabs[CONFIGURATION]) 
	{
		ImGui::Begin("Configuration", &open_tabs[CONFIGURATION]);

		if (ImGui::CollapsingHeader("Graphics")) 
			DrawGraphicsTab();
		if (ImGui::CollapsingHeader("Window"))
			DrawWindowConfig();
		if (ImGui::CollapsingHeader("Hardware"))
			DrawHardware();
		if (ImGui::CollapsingHeader("Application"))
			DrawApplication();

		if (ImGui::Button("Reset Camera"))
			App->camera->editor_camera->Reset();

		ImGui::End();
	}

	if (open_tabs[HIERARCHY])
		DrawHierarchyTab();

	if (open_tabs[OBJ_INSPECTOR])
		DrawObjectInspectorTab();

	if (open_tabs[PRIMITIVE])
		DrawPrimitivesTab();

	if (open_tabs[ABOUT])
		DrawAboutWindow();
	
	if (open_tabs[LOG])
		app_log->Draw("App log",&open_tabs[LOG]);
/*
	if (open_tabs[TIME_CONTROL])
		DrawTimeControl();*/
/*
	if (open_tabs[AUDIO])
		DrawAudioTab();*/



	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Quit"))
				close_app = true;
			if (ImGui::MenuItem("Import file"))
			{
				std::string file_path = openFileWID();
				App->importer->Import(file_path.c_str());
			}
			if(ImGui::BeginMenu("Configuration")){
				if (ImGui::MenuItem("Save Configuration"))
					App->SaveConfig();
				if (ImGui::MenuItem("Delete Configuration"))
					App->DeleteConfig();
				if (ImGui::MenuItem("Load Default Configuration"))
					App->LoadDefaultConfig();
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Hierarchy", NULL, &open_tabs[HIERARCHY]);
			ImGui::MenuItem("Object Inspector", NULL, &open_tabs[OBJ_INSPECTOR]);
			ImGui::MenuItem("Primitive", NULL, &open_tabs[PRIMITIVE]);
			ImGui::MenuItem("Configuration", NULL, &open_tabs[CONFIGURATION]);
			ImGui::MenuItem("Log", NULL, &open_tabs[LOG]);
			//ImGui::MenuItem("Time control", NULL, &open_tabs[TIME_CONTROL]);
			//ImGui::MenuItem("Audio", NULL, &open_tabs[AUDIO]);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("About", NULL, &open_tabs[ABOUT]);
			if(ImGui::MenuItem("Documentation"))
				App->requestBrowser("https://github.com/Skyway666/Kuroko-Engine/wiki");
			if(ImGui::MenuItem("Download latest"))
				App->requestBrowser("https://github.com/Skyway666/Kuroko-Engine/releases");
			if(ImGui::MenuItem("Report a bug"))
				App->requestBrowser("https://github.com/Skyway666/Kuroko-Engine/issues");
			ImGui::EndMenu();
		}
	
	}
	ImGui::EndMainMenuBar();
	ImGui::End();
	return UPDATE_CONTINUE;

}

update_status ModuleUI::PostUpdate(float dt) {
	// Rendering
	ImGui::Render();
	
	glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);

	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	if (!close_app)
		return UPDATE_CONTINUE;
	else
		return UPDATE_STOP;
}

bool ModuleUI::CleanUp() 
{	
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	return true;
}




void ModuleUI::DrawHierarchyTab()
{
	ImGui::Begin("Hierarchy Tab", &open_tabs[HIERARCHY]);
	ImGui::PushFont(ui_fonts[REGULAR]);

	int id = 0;
	std::list<GameObject*> root_objs;
	App->scene->getRootObjs(root_objs);

	for (auto it = root_objs.begin(); it != root_objs.end(); it++)
		DrawHierarchyNode(*(*it), id);

	ImGui::PopFont();
	ImGui::End();
}

void ModuleUI::DrawHierarchyNode(const GameObject& game_object, int& id) const
{
	id++;
	static int selection_mask = (1 << 2);
	ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ((selection_mask & (1 << id)) ? ImGuiTreeNodeFlags_Selected : 0);

	std::list<GameObject*> children;
	game_object.getChildren(children);

	if(children.empty())
		node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; 

	bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)id, node_flags, game_object.getName().c_str(), id) && !children.empty();

	if (ImGui::IsItemClicked())
	{
		selection_mask = (1 << id);
		App->scene->selected_obj = (GameObject*)&game_object;
	}

	if (node_open)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetFontSize() * 3);

		for (std::list<GameObject*>::iterator it = children.begin(); it != children.end(); it++)
				DrawHierarchyNode(*(*it), id);

		ImGui::PopStyleVar();
		ImGui::TreePop();
	}
}

void ModuleUI::DrawObjectInspectorTab()
{
	ImGui::Begin("Object inspector", &open_tabs[OBJ_INSPECTOR]);
	ImGui::PushFont(ui_fonts[REGULAR]);

	static bool show_rename = false;
	GameObject* selected_obj = App->scene->selected_obj;

	if (selected_obj)
	{
		ImGui::Text("Name: %s", selected_obj->getName().c_str());

		ImGui::SameLine();
		if (ImGui::Button("Rename"))
			show_rename = true;

		ImGui::SameLine();
		if (ImGui::Button("Delete"))
			App->scene->deleteGameObject(selected_obj);

		ImGui::Checkbox("Active", &selected_obj->is_active);
		ImGui::SameLine();
		ImGui::Checkbox("Static", &selected_obj->is_static);

		if (ImGui::CollapsingHeader("Add component"))
		{
			//if (ImGui::Button("Add Mesh"))	selected_obj->addComponent(MESH);
			if (ImGui::Button("Add AABB"))  selected_obj->addComponent(C_AABB);
		}

		std::list<Component*> components;
		selected_obj->getComponents(components);

		std::list<Component*> components_to_erase;
		for (std::list<Component*>::iterator it = components.begin(); it != components.end(); it++)
			if (!DrawComponent(*(*it)))
				components_to_erase.push_back(*it);

		for (std::list<Component*>::iterator it = components_to_erase.begin(); it != components_to_erase.end(); it++)
			selected_obj->removeComponent(*it);
	}
	else if (show_rename)
		show_rename = false;

	ImGui::PopFont();
	ImGui::End();
	
	if (show_rename)
	{
		ImGui::SetNextWindowPos(ImVec2(700, 320), ImGuiCond_FirstUseEver); 
		ImGui::Begin("Rename object");
		ImGui::PushFont(ui_fonts[REGULAR]);

		static char rename_buffer[64];
		ImGui::InputText("Rename to", rename_buffer, 64);

		ImGui::SameLine();
		if (ImGui::Button("Change"))
		{
			selected_obj->Rename(rename_buffer);
			show_rename = false;
		}

		ImGui::PopFont();
		ImGui::End();
	}
}

bool ModuleUI::DrawComponent(Component& component)
{
	switch (component.getType())
	{
	case MESH:
		if (ImGui::CollapsingHeader("Mesh"))
		{
			ComponentMesh* c_mesh = (ComponentMesh*)&component;
			static bool wireframe_enabled;
			static bool mesh_active;
			static bool draw_normals;

			wireframe_enabled = c_mesh->getWireframe();
			draw_normals = c_mesh->getDrawNormals();
			mesh_active = c_mesh->isActive();

			if (ImGui::Checkbox("Is active", &mesh_active))
				c_mesh->setActive(mesh_active);

			if (mesh_active)
			{

				if (ImGui::Checkbox("Wireframe", &wireframe_enabled))
					c_mesh->setWireframe(wireframe_enabled);
				ImGui::SameLine();
				if (ImGui::Checkbox("Draw normals", &draw_normals))
					c_mesh->setDrawNormals(draw_normals);
				
				
				if (Material* material = c_mesh->getMaterial())
				{
					if (ImGui::TreeNode("Material"))
					{
						static int preview_size = 128;
						ImGui::Text("Id: %d", material->getId());
						ImGui::SameLine();
						if (ImGui::Button("remove material"))
						{
							c_mesh->setMaterial(nullptr);
							ImGui::TreePop();
							return true;
						}

						ImGui::Text("preview size");
						ImGui::SameLine();
						if (ImGui::Button("64")) preview_size = 64;
						ImGui::SameLine();
						if (ImGui::Button("128")) preview_size = 128;
						ImGui::SameLine();
						if (ImGui::Button("256")) preview_size = 256;

						

						if(ImGui::TreeNode("diffuse"))
						{
							ImGui::Image(material->getTexture(DIFFUSE) ? (void*)material->getTexture(DIFFUSE)->getGLid() : (void*)ui_textures[NO_TEXTURE]->getGLid(), ImVec2(preview_size, preview_size));

							if (ImGui::Button("Dif: Load checkered "))
								material->setCheckeredTexture(DIFFUSE);
							ImGui::SameLine();
							if (ImGui::Button("Dif: Load "))
							{
								std::string texture_path = openFileWID();
								if (Texture* tex = (Texture*)App->importer->Import(texture_path.c_str(), I_TEXTURE))
									c_mesh->getMaterial()->setTexture(DIFFUSE, tex);
							}
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("ambient (feature not avaliable yet)"))
						{
							ImGui::Image(material->getTexture(AMBIENT) ? (void*)material->getTexture(AMBIENT)->getGLid() : (void*)ui_textures[NO_TEXTURE]->getGLid(), ImVec2(preview_size, preview_size));

							if (ImGui::Button("Amb: Load checkered texture"))
								material->setCheckeredTexture(AMBIENT);
							ImGui::SameLine();
							if (ImGui::Button("Amb: Load texture"))
							{
								std::string texture_path = openFileWID();
								if (Texture* tex = (Texture*)App->importer->Import(texture_path.c_str(), I_TEXTURE))
									c_mesh->getMaterial()->setTexture(AMBIENT, tex);
							}
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("normals (feature not avaliable yet)"))
						{
							ImGui::Image(material->getTexture(NORMALS) ? (void*)material->getTexture(NORMALS)->getGLid() : (void*)ui_textures[NO_TEXTURE]->getGLid(), ImVec2(preview_size, preview_size));

							if (ImGui::Button("Nor: Load checkered texture"))
								material->setCheckeredTexture(NORMALS);
							ImGui::SameLine();
							if (ImGui::Button("Nor: Load texture"))
							{
								std::string texture_path = openFileWID();
								if (Texture* tex = (Texture*)App->importer->Import(texture_path.c_str(), I_TEXTURE))
									c_mesh->getMaterial()->setTexture(NORMALS, tex);
							}
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("lightmap (feature not avaliable yet)"))
						{
							ImGui::Image(material->getTexture(LIGHTMAP) ? (void*)material->getTexture(LIGHTMAP)->getGLid() : (void*)ui_textures[NO_TEXTURE]->getGLid(), ImVec2(preview_size, preview_size));

							if (ImGui::Button("Lgm: Load checkered texture"))
								material->setCheckeredTexture(LIGHTMAP);
							ImGui::SameLine();
							if (ImGui::Button("Lgm: Load texture"))
							{
								std::string texture_path = openFileWID();
								if (Texture* tex = (Texture*)App->importer->Import(texture_path.c_str(), I_TEXTURE))
									c_mesh->getMaterial()->setTexture(LIGHTMAP, tex);
							}
							ImGui::TreePop();
						}
						ImGui::TreePop();
					}
				}
				else
				{
					if (ImGui::Button("Add material"))
					{
						Material* mat = new Material();
						c_mesh->setMaterial(mat);
					}
				}

				if (Mesh* mesh = c_mesh->getMesh())
				{
					if (ImGui::TreeNode("Mesh Data"))
					{
						uint vert_num, poly_count;
						bool has_normals, has_colors, has_texcoords;

						mesh->getData(vert_num, poly_count, has_normals, has_colors, has_texcoords);
						ImGui::Text("vertices: %d, poly count: %d, ", vert_num, poly_count);
						ImGui::Text(has_normals ? "normals: Yes," : "normals: No,");
						ImGui::Text(has_colors ? "colors: Yes," : "colors: No,");
						ImGui::Text(has_texcoords ? "tex coords: Yes" : "tex coords: No");

						ImGui::TreePop();
					}
				}
			}

			if (ImGui::Button("Remove mesh"))
				return false;
		}
		break;
	case TRANSFORM:
		if (ImGui::CollapsingHeader("Transform"))
		{
			ImGui::Text("Drag the parameters to change them, or ctrl+click on one of them to set it's value");
			ComponentTransform* transform = (ComponentTransform*)&component;

			static float3 position;
			static float3 rotation;
			static float3 scale;

			position = transform->position;
			rotation = transform->getRotationEuler();
			scale = transform->scale;


			//position
			ImGui::Text("Position:");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("p x", &position.x, 0.01f, 0.0f, 0.0f, "%.02f");

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("p y", &position.y, 0.01f, 0.0f, 0.0f, "%.02f");

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("p z", &position.z, 0.01f, 0.0f, 0.0f, "%.02f");

			//rotation
			ImGui::Text("Rotation:");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("r x", &rotation.x, 0.2f, -180.0f, 180.0f, "%.02f");

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("r y", &rotation.y, 0.2f, -180.0f, 180.0f, "%.02f");

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("r z", &rotation.z, 0.2f, -180.0f, 180.0f, "%.02f");

			//scale
			ImGui::Text("   Scale:");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("s x", &scale.x, 0.01f, -1000.0f, 1000.0f, "%.02f");

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("s y", &scale.y, 0.01f, -1000.0f, 1000.0f, "%.02f");

			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			ImGui::DragFloat("s z", &scale.z, 0.01f, -1000.0f, 1000.0f, "%.02f");

			transform->setPosition(position);
			transform->setRotationEuler(rotation);
			transform->setScale(scale);
		}
		break;

	case C_AABB:
		if (ImGui::CollapsingHeader("AABB"))
		{
			ComponentAABB* aabb = (ComponentAABB*)&component;

			static bool aabb_active;
			aabb_active = aabb->isActive();


			if (ImGui::Checkbox("active AABB", &aabb_active))
				aabb->setActive(aabb_active);

			if (aabb_active)
			{
				static bool aabb_drawn;
				aabb_drawn = aabb->draw_aabb;

				if (ImGui::Checkbox("AABB drawn", &aabb_drawn))
					aabb->draw_aabb = aabb_drawn;

				static bool obb_drawn;
				obb_drawn = aabb->draw_obb;

				if (ImGui::Checkbox("OBB drawn", &obb_drawn))
					aabb->draw_obb = obb_drawn;

				if (ImGui::Button("Reload AABB"))
					aabb->Reload();
			}

			if (ImGui::Button("Remove AABB"))
				return false;
		}
		break;
	default:
		break;
	}

	return true;
}

void ModuleUI::DrawCameraTab(Camera* camera)
{
	if (camera->frame_buffer)
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize;
		ImGui::SetNextWindowSize(ImVec2(camera->frame_buffer->size_x + 10, camera->frame_buffer->size_y + 40));
		ImGui::Begin("Game", nullptr, flags);
		ImGui::ImageButton((void*)camera->frame_buffer->tex->gl_id, ImVec2(camera->frame_buffer->size_x, camera->frame_buffer->size_y), ImVec2(0, 1), ImVec2(1, 0), 2);
		ImGui::End();
	}
}
//
//void ModuleUI::DrawAudioTab()
//{
//	ImGui::Begin("Audio", &open_tabs[AUDIO]);
//	ImGui::PushFont(ui_fonts[REGULAR]);
//	ImGui::Text("Use this tab to check and play loaded audio files");
//	
//	for (auto it = App->audio->audio_files.begin(); it != App->audio->audio_files.end(); it++)
//	{
//		if (ImGui::TreeNode((*it)->name.c_str()))
//		{
//			ImGui::Text("type: %s", (*it)->type == FX ? "FX" : "Music");
//			ImGui::SameLine();
//			if (ImGui::ImageButton((void*)ui_textures[PLAY]->getGLid(), ImVec2(16, 16)))
//				(*it)->Play();
//			ImGui::SameLine();
//			if (ImGui::ImageButton((void*)ui_textures[STOP]->getGLid(), ImVec2(16, 16)))
//				(*it)->Stop();
//			ImGui::TreePop();
//		}
//	}
//
//  ImGui::PopFont();
//	ImGui::End();
//}

void ModuleUI::DrawPrimitivesTab() 
{
	ImGui::Begin("Primitives", &open_tabs[PRIMITIVE]);
	ImGui::PushFont(ui_fonts[REGULAR]);

	if (ImGui::Button("Add cube")){
		App->scene->CleanUp();// Just for asignment 1

		GameObject* cube = new GameObject("Cube");
		Mesh* mesh = new Mesh(Primitive_Cube);
		cube->addComponent(new ComponentMesh(cube, mesh));
		Material* mat = new Material();
		ComponentMesh* comp_mesh = (ComponentMesh*)cube->getComponent(MESH);
		comp_mesh->setMaterial(mat);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add plane")){
		App->scene->CleanUp();// Just for asignment 1

		GameObject* plane = new GameObject("Plane");
		Mesh* mesh = new Mesh(Primitive_Plane);
		plane->addComponent(new ComponentMesh(plane, mesh));
		Material* mat = new Material();
		ComponentMesh* comp_mesh = (ComponentMesh*)plane->getComponent(MESH);
		comp_mesh->setMaterial(mat);
	}
	if (ImGui::Button("Add sphere")) {
		App->scene->CleanUp();// Just for asignment 1

		GameObject* sphere = new GameObject("Sphere");
		Mesh* mesh = new Mesh(Primitive_Sphere);
		sphere->addComponent(new ComponentMesh(sphere, mesh));
		Material* mat = new Material();
		ComponentMesh* comp_mesh = (ComponentMesh*)sphere->getComponent(MESH);
		comp_mesh->setMaterial(mat);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add cylinder")) {
		App->scene->CleanUp();// Just for asignment 1

		GameObject* cylinder = new GameObject("Cylinder");
		Mesh* mesh = new Mesh(Primitive_Cylinder);
		cylinder->addComponent(new ComponentMesh(cylinder, mesh));
		Material* mat = new Material();
		ComponentMesh* comp_mesh = (ComponentMesh*)cylinder->getComponent(MESH);
		comp_mesh->setMaterial(mat);
	}

	ImGui::PopFont();
	ImGui::End();
}


void ModuleUI::DrawAboutWindow()
{
	ImGui::Begin("About", &open_tabs[ABOUT]); 
	ImGui::PushFont(ui_fonts[REGULAR]);

	ImGui::Text("Kuroko Engine");
	ImGui::Separator();
	ImGui::Text("An engine to make videogames");
	ImGui::Text("By Rodrigo de Pedro Lombao and Lucas Garcia Mateu.");
	ImGui::Text("Kuroko Engine is licensed under the MIT License.\n");
	ImGui::Separator();

	ImGui::Text("Libraries used:");
	ImGui::Text("Assimp %i", ASSIMP_API::aiGetVersionMajor());
	ImGui::SameLine();
	if (ImGui::Button("Learn more##assimp"))
		App->requestBrowser("http://www.assimp.org/");
	ImGui::Text("Glew %s", glewGetString(GLEW_VERSION));
	ImGui::SameLine();
	if (ImGui::Button("Learn more##glew"))
		App->requestBrowser("http://glew.sourceforge.net/");
	ImGui::Text("DevIL 1.8.0");
	ImGui::SameLine();
	if (ImGui::Button("Learn more##devil"))
		App->requestBrowser("http://openil.sourceforge.net/");
	ImGui::Text("MathGeoLib ? version");
	ImGui::SameLine();
	if (ImGui::Button("Learn more##mathgeolib"))
		App->requestBrowser("http://clb.demon.fi/MathGeoLib/");
	SDL_version compiled; 
	SDL_GetVersion(&compiled);
	ImGui::Text("SDL %d.%d.%d", compiled.major, compiled.major, compiled.patch);
	ImGui::SameLine();
	if (ImGui::Button("Learn more##sdl"))
		App->requestBrowser("https://wiki.libsdl.org/FrontPage");
	ImGui::Text("OpenGL %s", glGetString(GL_VERSION));
	ImGui::Text("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	ImGui::SameLine();
	if (ImGui::Button("Learn more##opngl"))
		App->requestBrowser("https://www.opengl.org/");
	ImGui::Text("Parson");
	ImGui::SameLine();
	if (ImGui::Button("Learn more##parson"))
		App->requestBrowser("https://github.com/kgabis/parson");
	ImGui::Text("PCG Random");
	ImGui::SameLine();
	if (ImGui::Button("Learn more##pcg_random"))
		App->requestBrowser("http://www.pcg-random.org");
	
	ImGui::PopFont();
	ImGui::End();
}

void ModuleUI::DrawGraphicsTab() const {
	//starting values
	ImGui::PushFont(ui_fonts[REGULAR]);

	static bool depth_test = glIsEnabled(GL_DEPTH_TEST);
	static bool face_culling = glIsEnabled(GL_CULL_FACE);
	static bool lighting = glIsEnabled(GL_LIGHTING);
	static bool material_color = glIsEnabled(GL_COLOR_MATERIAL);
	static bool textures = glIsEnabled(GL_TEXTURE_2D);
	static bool fog = glIsEnabled(GL_FOG);
	static bool antialias = glIsEnabled(GL_LINE_SMOOTH);
	ImGui::Text("Use this tab to enable/disable openGL characteristics");

	if (ImGui::TreeNode("Depth test")) {
		if (ImGui::Checkbox("DT Enabled", &depth_test)) {
			if (depth_test)			glEnable(GL_DEPTH_TEST);
			else					glDisable(GL_DEPTH_TEST);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Face culling")) {
		if (ImGui::Checkbox("FC Enabled", &face_culling)) {
			if (face_culling)		glEnable(GL_CULL_FACE);
			else					glDisable(GL_CULL_FACE);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Lighting")) {
		if (ImGui::Checkbox("L Enabled", &lighting)) {
			if (lighting)			glEnable(GL_LIGHTING);
			else					glDisable(GL_LIGHTING);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Material color")) {
		if (ImGui::Checkbox("M Enabled", &material_color)) {
			if (material_color)		glEnable(GL_COLOR_MATERIAL);
			else					glDisable(GL_COLOR_MATERIAL);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Textures")) {
		if (ImGui::Checkbox("T Enabled", &textures)) {
			if (textures)			glEnable(GL_TEXTURE_2D);
			else					glDisable(GL_TEXTURE_2D);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Fog")) {
		static float fog_distance = 0.5f;
		if (ImGui::Checkbox("F Enabled", &fog)) {
			if (fog)				glEnable(GL_FOG);
			else					glDisable(GL_FOG);

			if (fog) {
				GLfloat fog_color[4] = { 0.8f, 0.8f, 0.8f, 0.0f };
				glFogfv(GL_FOG_COLOR, fog_color);
				glFogf(GL_FOG_DENSITY, fog_distance);
			}
		}

		if (ImGui::SliderFloat("Fog density", &fog_distance, 0.0f, 1.0f))
			glFogf(GL_FOG_DENSITY, fog_distance);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Antialias")) {
		if (ImGui::Checkbox("A Enabled", &antialias)) {
			if (antialias)			glEnable(GL_LINE_SMOOTH);
			else					glDisable(GL_LINE_SMOOTH);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Wireframe")) {
		ImGui::Checkbox("WF Enabled", &App->scene->global_wireframe);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Normals")) {
		ImGui::Checkbox("N Enabled", &App->scene->global_normals);
		ImGui::TreePop();
	}
	ImGui::PopFont();
}

void ModuleUI::DrawWindowConfig() const
{
	ImGui::PushFont(ui_fonts[REGULAR]);

	Window* window = App->window->main_window;
	if(ImGui::SliderFloat("Brightness", &window->brightness, 0, 1.0f))
		App->window->setBrightness(window->brightness);

	bool width_mod, height_mod = false;
	width_mod = ImGui::SliderInt("Width", &window->width, 640, 1920);
	height_mod = ImGui::SliderInt("Height", &window->height, 480, 1080);
	
	if(width_mod || height_mod)
		App->window->setSize(window->width, window->height);

	// Refresh rate
	ImGui::Text("Refresh Rate %i", (int)ImGui::GetIO().Framerate);
	//Bools
	if (ImGui::Checkbox("Fullscreen", &window->fullscreen))
		App->window->setFullscreen(window->fullscreen);
	ImGui::SameLine();
	if (ImGui::Checkbox("Resizable", &window->resizable))
		App->window->setResizable(window->resizable);
	if (ImGui::Checkbox("Borderless", &window->borderless))
		App->window->setBorderless(window->borderless);
	ImGui::SameLine();
	if (ImGui::Checkbox("FullDesktop", &window->fulldesk))
		App->window->setFullDesktop(window->fulldesk);

	ImGui::PopFont();
}

void ModuleUI::DrawHardware() const 
{
	ImGui::PushFont(ui_fonts[REGULAR]);
	
	//CPUs
	ImGui::Text("CPUs");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255),"%i", SDL_GetCPUCount());

	// RAM
	ImGui::Text("System RAM");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%i Gb", SDL_GetSystemRAM());

	// Caps
	ImGui::Text("Caps:");
	ImGui::SameLine();
	if(SDL_HasRDTSC())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "RDTSC, ");
	ImGui::SameLine();
	if (SDL_HasMMX())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "MMX, ");
	ImGui::SameLine();
	if (SDL_HasSSE())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "SSE, ");
	ImGui::SameLine();
	if (SDL_HasSSE2())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "SSE2, ");
	if (SDL_HasSSE3())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "SSE3, ");
	ImGui::SameLine();
	if (SDL_HasSSE41())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "SSE41, ");
	ImGui::SameLine();
	if (SDL_HasSSE42())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "SSE42, ");
	ImGui::SameLine();
	if (SDL_HasAVX())
		ImGui::TextColored(ImVec4(0, 255, 0, 255), "AVX.");
	ImGui::SameLine();

	ImGui::Separator();
	//GPU
	ImGui::Text("Caps:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%s", glGetString(GL_VENDOR));
	ImGui::Text("Brand:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%s", glGetString(GL_RENDERER));
	ImGui::Text("VRAM Budget:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%f Mb", getTotalVRAMMb_NVIDIA());
	ImGui::Text("VRAM Usage:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%f Mb", getTotalVRAMMb_NVIDIA() - getAvaliableVRAMMb_NVIDIA());
	ImGui::Text("VRAM Avaliable:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%f Mb", getAvaliableVRAMMb_NVIDIA());
	ImGui::Text("VRAM Reserved:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 255, 0, 255), "%f Mb", 0);
		
	ImGui::PopFont();
}

void ModuleUI::DrawApplication() const 
{
	// HARDCODED (?)
	ImGui::PushFont(ui_fonts[REGULAR]);
	
	ImGui::Text("App name: Kuroko Engine");
	ImGui::Text("Organization: UPC CITM");
	char title[25];
	sprintf_s(title, 25, "Framerate %.1f", App->fps_log[App->fps_log.size() - 1]);
	ImGui::PlotHistogram("##framerate", &App->fps_log[0], App->fps_log.size(), 0, title, 0.0f, 100.0f, ImVec2(310, 100));
	sprintf_s(title, 25, "Milliseconds %.1f", App->ms_log[App->ms_log.size() - 1]);
	ImGui::PlotHistogram("##milliseconds", &App->ms_log[0], App->ms_log.size(), 0, title, 0.0f, 100.0f, ImVec2(310, 100));
	
	ImGui::PopFont();
}

void ModuleUI::DrawTimeControl()
{
	ImGui::Begin("Time control", &open_tabs[TIME_CONTROL]);

	int w, h;
	ui_textures[PLAY]->getSize(w, h);
	if (ImGui::ImageButton((void*)ui_textures[PLAY]->getGLid(), ImVec2(w, h), ImVec2(0,0), ImVec2(1, 1), 0, ImVec4(0.0f, 0.7f, 0.7f, App->scene->getGameState() == PLAYING ? 1.0f : 0.0f)))
		App->scene->Play();

	ImGui::SameLine();
	ui_textures[PAUSE]->getSize(w, h);
	if(ImGui::ImageButton((void*)ui_textures[PAUSE]->getGLid(), ImVec2(w, h), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0.0f, 0.7f, 0.7f, App->scene->getGameState() == PAUSED ? 1.0f : 0.0f)))
		App->scene->Pause();

	ImGui::SameLine();
	ui_textures[STOP]->getSize(w, h);
	if (ImGui::ImageButton((void*)ui_textures[STOP]->getGLid(), ImVec2(w, h), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0.0f, 0.7f, 0.7f, App->scene->getGameState() == STOPPED ? 1.0f : 0.0f)))
		App->scene->Stop();

	ImGui::End();
}

void ModuleUI::SaveConfig(JSON_Object& config) const
{
	json_object_set_boolean(&config, "hierarchy", open_tabs[HIERARCHY]);
	json_object_set_boolean(&config, "obj_inspector", open_tabs[OBJ_INSPECTOR]);
	json_object_set_boolean(&config, "primitive", open_tabs[PRIMITIVE]);
	json_object_set_boolean(&config, "about", open_tabs[ABOUT]);
	json_object_set_boolean(&config, "configuration", open_tabs[CONFIGURATION]);
	json_object_set_boolean(&config, "log", open_tabs[LOG]);
	json_object_set_boolean(&config, "time_control", open_tabs[TIME_CONTROL]);
	//json_object_set_boolean(config, "audio", open_tabs[AUDIO]);
}

void ModuleUI::LoadConfig(const JSON_Object& config) 
{
	open_tabs[CONFIGURATION]	= json_object_get_boolean(&config, "configuration");
	open_tabs[HIERARCHY]		= json_object_get_boolean(&config, "hierarchy");
	open_tabs[OBJ_INSPECTOR]	= json_object_get_boolean(&config, "obj_inspector");
	open_tabs[PRIMITIVE]		= json_object_get_boolean(&config, "primitive");
	open_tabs[ABOUT]			= json_object_get_boolean(&config, "about");
	open_tabs[LOG]				= json_object_get_boolean(&config, "log");
	open_tabs[TIME_CONTROL]		= json_object_get_boolean(&config, "time_control");
	//open_tabs[AUDIO]			= json_object_get_boolean(config, "audio");
}

void ModuleUI::InvisibleDockingBegin() {
	ImGuiWindowFlags window = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	static ImGuiDockNodeFlags optional = ImGuiDockNodeFlags_PassthruDockspace;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Kuroko Engine", &docking_background, window);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGuiIO& io = ImGui::GetIO();

	ImGuiID dockspace_id = ImGui::GetID("The dockspace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), optional);
}

void ModuleUI::InvisibleDockingEnd() {
	ImGui::End();
}