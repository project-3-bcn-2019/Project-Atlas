#ifndef __MODULE_SCENE
#define __MODULE_SCENE
#include "Module.h"
#include "MathGeoLib\MathGeoLib.h"
#include "MathGeoLib\Geometry\Frustum.h"
#include <list>

class GameObject; 
class Material;
class Mesh;
class Texture;
class Skybox;
class Quadtree;

enum GameState { STOPPED, PAUSED, PLAYING};

struct RayHit
{
	RayHit(float distance, GameObject*obj) : distance(distance), obj(obj) {};
	float distance = 0.0f;
	GameObject* obj = nullptr;
};

class ModuleScene : public Module
{

	friend class ModuleUI;
public:
	ModuleScene(Application* app, bool start_enabled = true);
	~ModuleScene();

	bool Start();
	update_status PostUpdate(float dt);
	update_status Update(float dt);
	bool CleanUp();

	void DrawScene(float3 camera_pos);
	void DrawGuizmo();

	void Play() { game_state = PLAYING; };
	void Pause() { game_state = PAUSED; };
	void Stop() { game_state = STOPPED; };
	GameState getGameState() { return game_state; };

	void addMaterial(Material* mat)			{ materials.push_back(mat); };
	void addGameObject(GameObject* gobj)	{ game_objects.push_back(gobj); };
	void addMesh(Mesh* mesh)				{ meshes.push_back(mesh); };
	void addTexture(Texture* tex)			{ textures.push_back(tex); };

	Material* getMaterial(uint id) const;
	GameObject* getGameObject(uint id) const;
	Mesh* getMesh(uint id) const;
	Texture* getTexture(uint id) const;

	void deleteMaterial(Material* mat)		{ materials_to_delete.push_back(mat); };
	void deleteGameObject(GameObject* gobj)	{ game_objs_to_delete.push_back(gobj); };
	void deleteGameObjectRecursive(GameObject* gobj);
	void deleteMesh(Mesh* mesh)				{ meshes_to_delete.push_back(mesh); };
	void deleteTexture(Texture* tex)		{ textures_to_delete.push_back(tex); };
	void ClearScene();

	void getRootObjs(std::list<GameObject*>& list_to_fill);

	void AskSceneSave(char* scene_name); 
	void AskSceneLoad(char* path);

	GameObject* MousePicking();
private:
	
	void ManageSceneSaveLoad();
	void SaveScene(std::string name);
	void LoadScene(const char* path);
private:

	std::list<GameObject*>	game_objects; 
	std::list<Material*>	materials;
	std::list<Mesh*>		meshes;
	std::list<Texture*>		textures;

	std::list<GameObject*>	game_objs_to_delete;
	std::list<Material*>	materials_to_delete;
	std::list<Mesh*>		meshes_to_delete;
	std::list<Texture*>		textures_to_delete;

	Skybox* skybox			= nullptr;
	Quadtree * quadtree		= nullptr;
	GameState game_state	= STOPPED;
	bool want_save_scene	= false;
	bool want_load_scene	= false;

	std::string scene_to_save_name;
	std::string path_to_load_scene;
public:

	GameObject* selected_obj	= nullptr;

	uint last_mat_id			= 0;
	uint last_gobj_id			= 0;
	uint last_mesh_id			= 0;
	uint last_tex_id			= 0;

	bool global_wireframe		= false;
	bool global_normals			= false;
};
#endif