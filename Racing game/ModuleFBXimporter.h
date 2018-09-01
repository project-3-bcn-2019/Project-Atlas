#pragma once
#include "Module.h"

class GameObject;
class ComponentMesh;
class aiNode;
class aiScene;

class ModuleFBXimporter : public Module
{
public:
	ModuleFBXimporter(Application* app, bool start_enabled = true);
	~ModuleFBXimporter();

	bool Init();
	bool CleanUp();

	GameObject* LoadFBX(const char* file);
	GameObject* LoadAssimpNode(aiNode* node, const aiScene* scene, GameObject* parent = nullptr);
	bool LoadRootMesh(const char* file, ComponentMesh* component_to_load);	 // this method will only load the root mesh of an FBX, if existent. To load a full FBX scene, use LoadFBX()
};