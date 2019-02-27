#include "ModuleScripting.h"
#include "Wren/wren.hpp"
#include "Applog.h"
#include "ModuleInput.h"
#include "ScriptData.h"
#include "ModuleScene.h"
#include "ModuleInput.h"
#include "ModuleResourcesManager.h"
#include "ModuleTimeManager.h"

// May be better to manage in scene
#include "GameObject.h"
#include "ComponentTransform.h"
#include "Transform.h"

// Engine comunicator
void ConsoleLog(WrenVM* vm); 
void InstantiatePrefab(WrenVM* vm);
void getTime(WrenVM* vm);
void BreakPoint(WrenVM* vm);
void FindGameObjectByTag(WrenVM* vm);

// Math
void sqrt(WrenVM* vm);

// Object comunicator
void SetGameObjectPos(WrenVM* vm);
void ModGameObjectPos(WrenVM* vm);
void lookAt(WrenVM* vm);

void getGameObjectPosX(WrenVM* vm);
void getGameObjectPosY(WrenVM* vm);
void getGameObjectPosZ(WrenVM* vm);
void getGameObjectPitch(WrenVM* vm);
void getGameObjectYaw(WrenVM* vm);
void getGameObjectRoll(WrenVM* vm);

void KillGameObject(WrenVM* vm);
void MoveGameObjectForward(WrenVM* vm);

//Time
void GetDeltaTime(WrenVM* vm);
void GetTimeScale(WrenVM* vm);
void SetTimeScale(WrenVM* vm);

// Input comunicator
void getKey(WrenVM* vm);
void getButton(WrenVM* vm);
void getMouseRaycastX(WrenVM* vm);
void getMouseRaycastY(WrenVM* vm);
void getMouseRaycastZ(WrenVM* vm);



WrenForeignMethodFn bindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature); // Wren foraign methods
WrenForeignClassMethods bindForeignClass(WrenVM* vm, const char* module, const char* className);
char* loadModule(WrenVM* vm, const char* name);
void* reallocate(void* memory, size_t newSize);
void error(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message);
void write(WrenVM* vm, const char* text);

ModuleScripting::ModuleScripting(Application* app, bool start_enabled) : Module(app, start_enabled) 
{ 
	name = "scripting";
}


bool ModuleScripting::Init(const JSON_Object* config) 
{

	WrenConfiguration wconfig;
	wrenInitConfiguration(&wconfig);

	wconfig.bindForeignClassFn = bindForeignClass;
	wconfig.bindForeignMethodFn = bindForeignMethod;
	wconfig.loadModuleFn = loadModule;
	wconfig.writeFn = write;
	wconfig.errorFn = error;

	LoadConfig(config);

	tags.push_back("undefined");

	//wconfig.reallocateFn = reallocate;

	if (vm = wrenNewVM(&wconfig))
	{
		// Create base signature handles to call in every existing class
		base_signatures.insert(std::make_pair(std::string("Start()"), wrenMakeCallHandle(vm, "Start()")));
		base_signatures.insert(std::make_pair(std::string("Update()"), wrenMakeCallHandle(vm, "Update()")));
		base_signatures.insert(std::make_pair(std::string("new()"), wrenMakeCallHandle(vm, "new()")));
		base_signatures.insert(std::make_pair(std::string("gameObject=(_)"), wrenMakeCallHandle(vm, "gameObject=(_)")));

		
		object_linker_code = App->fs.GetFileString(OBJECT_LINKER);
		return true;
	}
	else
		return false;
}

update_status ModuleScripting::Update(float dt)
{
	// Iterate all script instances. Change the variables if edited and update them so they match value in script
	for (auto instance = loaded_instances.begin(); instance != loaded_instances.end(); instance++)
	{
		for (auto var = (*instance)->vars.begin(); var != (*instance)->vars.end(); var++)
		{
			// Update variables in script with editor values
			if (var->isEdited())
			{
				wrenEnsureSlots(vm, 2);
				wrenSetSlotHandle(vm, 0, (*instance)->class_handle);

				switch ((*var).getType())
				{
				case ImportedVariable::WREN_BOOL:
					wrenSetSlotBool(vm, 1, (*var).GetValue().value_bool);
					break;
				case ImportedVariable::WREN_NUMBER:
					wrenSetSlotDouble(vm, 1, (*var).GetValue().value_number);
					break;
				case ImportedVariable::WREN_STRING:
					wrenSetSlotString(vm, 1, (*var).value_string.c_str());
					break;
				}
				for (auto setter = (*instance)->methods.begin(); setter != (*instance)->methods.end(); setter++)
				{
					if ((*setter).getName() == (*var).getName() + "=(_)")
						wrenCall(vm, (*setter).getWrenHandle());
				}

				var->setEdited(false);
			}
			// Update editor values with variables in script
			wrenEnsureSlots(vm, 1);
			wrenSetSlotHandle(vm, 0, (*instance)->class_handle);
			wrenCall(vm, (*var).getGetter());
			Var script_value;
			switch ((*var).getType()) {
			case ImportedVariable::WREN_BOOL:
				script_value.value_bool = wrenGetSlotBool(vm, 0);
				(*var).SetValue(script_value, (*var).getType());
				break;
			case ImportedVariable::WREN_NUMBER:
				script_value.value_number = wrenGetSlotDouble(vm, 0);
				(*var).SetValue(script_value, (*var).getType());
				break;
			case ImportedVariable::WREN_STRING:
				static char buf[256] = "";
				strcpy(buf, wrenGetSlotString(vm, 0));
				(*var).value_string = buf;
				break;
			}

		}

		if ((*instance)->getState() == SCRIPT_STARTING)
		{
			for (auto it = (*instance)->methods.begin(); it != (*instance)->methods.end(); it++)
			{
				if ((*it).getName() == "Start()")
				{
					wrenEnsureSlots(vm, 1);
					wrenSetSlotHandle(vm, 0, (*instance)->class_handle);
					wrenCall(vm, (*it).getWrenHandle());
					break;
				}
			}
			(*instance)->setState(SCRIPT_UPDATING);
		}
		else if ((*instance)->getState() == SCRIPT_UPDATING)
		{
			for (auto it = (*instance)->methods.begin(); it != (*instance)->methods.end(); it++)
			{
				if ((*it).getName() == "Update()")
				{
					wrenEnsureSlots(vm, 1);
					wrenSetSlotHandle(vm, 0, (*instance)->class_handle);
					wrenCall(vm, (*it).getWrenHandle());
					break;
				}
			}
		}
	}
	return UPDATE_CONTINUE;
}


void ModuleScripting::StartInstances() { for (auto it : loaded_instances) (*it).setState(SCRIPT_STARTING); }
void ModuleScripting::PauseInstances() { for (auto it : loaded_instances) (*it).setState(SCRIPT_PAUSED); }
void ModuleScripting::StopInstances() { for (auto it : loaded_instances)  (*it).setState(SCRIPT_STOPPED); }


void ModuleScripting::SaveConfig(JSON_Object * config) const {
	JSON_Array* tags_arr = json_array(json_value_init_array());

	for (auto it = tags.begin(); it != tags.end(); it++) {
		if ((*it) == "undefined") // Hardcoded value, omit saving to avoid repetition
			continue;
		json_array_append_string(tags_arr, (*it).c_str());
	}

	json_object_set_value(config, "tags", json_array_get_wrapping_value(tags_arr));

}
void ModuleScripting::LoadConfig(const JSON_Object * config) {
	tags.clear();

	JSON_Array* tags_arr = json_object_get_array(config, "tags");

	for (int i = 0; i != json_array_get_count(tags_arr); i++)
		tags.push_back(json_array_get_string(tags_arr, i));
}


bool ModuleScripting::CleanUp() 
{
	for(auto it = base_signatures.begin(); it != base_signatures.end(); it++)
		wrenReleaseHandle(vm, (*it).second);

	base_signatures.clear();

	// Resources already hold pointers to the original handlers
	//for (int i = 0; i < loaded_scripts.size(); i++)
	//	delete loaded_scripts[i];

	wrenFreeVM(vm);
	loaded_scripts.clear();
	loaded_instances.clear();

	return true;
}


ScriptData* ModuleScripting::GenerateScript(const char* file_name_c)
{
	std::string file_name = file_name_c;

	ScriptData* script = new ScriptData();
	script->class_name = file_name;
	script->class_handle = GetHandlerToClass(file_name.c_str(), file_name.c_str());
		
	std::vector<std::string> methods = GetMethodsFromClassHandler(script->class_handle);
	for (int i = 0; i < methods.size(); i++)
	{
		std::string method_name = methods[i];
		if (method_name.find("(") != std::string::npos)					
		{
			if (method_name.find("=") == std::string::npos)				// method
			{
				ImportedVariable::WrenDataType return_type_test = ImportedVariable::WREN_NUMBER;
				std::string var_name_test = "test";
				std::vector<ImportedVariable> args;
				float var_value_test = 0.0f;

				size_t offset = method_name.find_first_of("(");
				for (auto it = method_name.find_first_of("_", offset + 1); it != std::string::npos; it = method_name.find_first_of("_", offset + 1))
				{
					offset = it;
					args.push_back(ImportedVariable(var_name_test.c_str(), return_type_test, (void*)&var_value_test, nullptr)); // Don't call getters on arguments
				}

				script->methods.push_back(ImportedMethod(method_name, return_type_test, args, wrenMakeCallHandle(vm, method_name.c_str())));
			}													 
			else	
				// setter
				script->methods.push_back(ImportedMethod(method_name, wrenMakeCallHandle(vm, method_name.c_str())));
		}						
		else															// var
		{

			float value_test = 0.0f;
			ImportedVariable var(method_name.c_str(), ImportedVariable::WrenDataType::WREN_NUMBER, &value_test, wrenMakeCallHandle(vm, method_name.c_str()));

			if (method_name == "gameObject")
				var.setPublic(false);

			script->vars.push_back(var);
		}
	}

	loaded_scripts.push_back(script);
	return script;
}

bool ModuleScripting::CompileIntoVM(const char* module, const char* code) 
{
	return (wrenInterpret(vm, module, code) == WREN_RESULT_SUCCESS);
}

WrenHandle* ModuleScripting::GetHandlerToClass(const char* module, const char* class_name) 
{
	// Get static class
	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, module, class_name, 0); // Class is now on slot 0
	

	// Call constructor to get instance
	wrenCall(vm, base_signatures.at("new()")); // instance of the class is now at slot 0
	
	// Get the prize
	return wrenGetSlotHandle(vm, 0);
}

void ModuleScripting::ReleaseHandler(WrenHandle* handle)
{
	wrenReleaseHandle(vm, handle);
}

WrenHandle* ModuleScripting::getHandlerToMethod(const char* method_name)
{
	return wrenMakeCallHandle(vm, method_name);
}

std::vector<std::string> ModuleScripting::GetMethodsFromClassHandler(WrenHandle * wrenClass) {

	if (!IS_OBJ(wrenClass->value)) {
		app_log->AddLog("Trying to get methods from a non object handler");
		return std::vector<std::string>();
	}

	std::vector<std::string> ret;
	Obj* cls = AS_OBJ(wrenClass->value);

	for (int i = 0; i < cls->classObj->methods.count; ++i) 
	{
		Method& method = cls->classObj->methods.data[i];

		if (method.type != METHOD_PRIMITIVE && method.type != METHOD_FOREIGN && method.type != METHOD_NONE ) 
			ret.push_back(method.as.closure->fn->debug->name);
	}

	return ret;
}


// Wren callbacks  ======================================================================

void write(WrenVM* vm, const char* text)
{
	app_log->AddLog(text);
}

void error(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message)
{
	if (type == WrenErrorType::WREN_ERROR_COMPILE)
		app_log->AddLog("Couldn't compile %s. %s error in %i line", module, message, line);
	else if (type == WrenErrorType::WREN_ERROR_RUNTIME)
		app_log->AddLog("Error when running %s. %s error in %i line", module, message, line);
}

char* loadModule(WrenVM* vm, const char* name)
{
	char* ret = nullptr;
	if (strcmp(name, "ObjectLinker") == 0) {

		int string_size = strlen(App->scripting->object_linker_code.c_str()) + 1; // 1 for the /0
		ret = new char[string_size];
		strcpy(ret, App->scripting->object_linker_code.c_str());
		ret[string_size - 1] = '\0';
	}
	return ret;	
}


void* reallocate(void* memory, size_t newSize) {

	if (!memory) {
		return malloc(newSize);
	}
	else{
		if (newSize != 0)
			return realloc(memory, newSize);
		else
			free(memory);
	}

	return nullptr;
}

WrenForeignClassMethods bindForeignClass(WrenVM* vm, const char* module, const char* className)
{
	WrenForeignClassMethods methods;

	//if (strcmp(className, "File") == 0) {
	//	methods->allocate = fileAllocate;
	//	methods->finalize = fileFinalize;
	//}
	//else {
	//	// Unknown class.
	methods.allocate = NULL;
	methods.finalize = NULL;
	//}

	return methods;
}

WrenForeignMethodFn bindForeignMethod(WrenVM* vm, const char* module, const char* className, bool isStatic, const char* signature) // Wren foraign methods
{

	// OBJECT COMUNICATOR
	if(strcmp(module, "ObjectLinker") == 0){
		if(strcmp(className, "ObjectComunicator") == 0){
			if (strcmp(signature, "C_setPos(_,_,_,_)") == 0) {
				return SetGameObjectPos; //  C function for ObjectComunicator.C_setPos
			}
			if (strcmp(signature, "C_modPos(_,_,_,_)") == 0) {
				return ModGameObjectPos; // C function for ObjectComunicator.C_modPos
			}
			if (strcmp(signature, "C_lookAt(_,_,_,_)") == 0) {
				return lookAt; // C function for ObjectComunicator.C_LookAt
			}
			if (strcmp(signature, "C_getPosX(_,_)") == 0) {
				return getGameObjectPosX; // C function for ObjectComunicator.C_getPosX
			}
			if (strcmp(signature, "C_getPosY(_,_)") == 0) {
				return getGameObjectPosY; // C function for ObjectComunicator.C_getPosY
			}
			if (strcmp(signature, "C_getPosZ(_,_)") == 0) {
				return getGameObjectPosZ; // C function for ObjectComunicator.C_getPosZ
			}
			if (strcmp(signature, "C_getPitch(_)") == 0) {
				return getGameObjectPitch; // C function for ObjectComunicator.C_getPitch
			}
			if (strcmp(signature, "C_getYaw(_)") == 0) {
				return getGameObjectYaw; // C function for ObjectComunicator.C_getYaw
			}
			if (strcmp(signature, "C_getRoll(_)") == 0) {
				return getGameObjectRoll; // C function for ObjectComunicator.C_getRoll
			}
			if (strcmp(signature, "C_Kill(_)") == 0) {
				return KillGameObject; // C function for ObjectComunicator.C_Kill
			}
			if (strcmp(signature, "C_MoveForward(_,_)") == 0) {
				return MoveGameObjectForward; // C function for ObjectComunicator.C_MoveForward
			}
		}
		if (strcmp(className, "Math") == 0) {
			if (isStatic && strcmp(signature, "C_sqrt(_)") == 0)
				return sqrt; // C function for Math.C_sqrt(_)
		}
		if (strcmp(className, "Time") == 0) {
			if (isStatic && strcmp(signature, "C_GetDeltaTime()") == 0) {
				return GetDeltaTime;
			}
			if (isStatic && strcmp(signature, "C_GetTimeScale()") == 0) {
				return GetTimeScale;
			}
			if(isStatic && strcmp(signature,"C_SetTimeScale(_)") == 0){
				return SetTimeScale;
			}
		}
		if (strcmp(className, "EngineComunicator") == 0) {
			if (isStatic && strcmp(signature, "consoleOutput(_)") == 0)
				return ConsoleLog; // C function for EngineComunicator.consoleOutput
			if (isStatic && strcmp(signature, "Instantiate(_,_,_,_,_,_,_)") == 0)
				return InstantiatePrefab; // C function for EngineComunicator.Instantiate
			if (isStatic && strcmp(signature, "getTime()") == 0)
				return getTime;
			if (isStatic && strcmp(signature, "BreakPoint(_,_,_)") == 0)
				return BreakPoint;
			if (isStatic && strcmp(signature, "FindGameObjectsByTag(_)") == 0)
				return FindGameObjectByTag;
		}
		if (strcmp(className, "InputComunicator") == 0) {
			if (isStatic && strcmp(signature, "getKey(_,_)") == 0)
				return getKey; // C function for InputComunicator.getKey
			if (isStatic && strcmp(signature, "getMouseRaycastX()") == 0)
				return getMouseRaycastX; // C function for InputComunicator.getMouseRaycastX
			if (isStatic && strcmp(signature, "getMouseRaycastY()") == 0)
				return getMouseRaycastY; // C function for InputComunicator.getMouseRaycastY
			if (isStatic && strcmp(signature, "getMouseRaycastZ()") == 0)
				return getMouseRaycastZ; // C function for InputComunicator.getMouseRaycastZ
			if (isStatic && strcmp(signature, "getButton(_,_,_)") == 0)
				return getButton;
		}
	}


}

// C++ functions to be called from wren  ==================================================

void ConsoleLog(WrenVM* vm)
{
	static int call_times = 1;
	const char* message = wrenGetSlotString(vm, 1);

	app_log->AddLog("This is a C function called from wren, from a wren function called from C\nIt has been called %i times now\nIt contained this message: %s", call_times, message);
	call_times++;
}

void SetGameObjectPos(WrenVM* vm) {

	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	float x = wrenGetSlotDouble(vm, 2);
	float y = wrenGetSlotDouble(vm, 3);
	float z = wrenGetSlotDouble(vm, 4);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing resource");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);
	c_trans->local->setPosition(float3(x, y, z));
}

void ModGameObjectPos(WrenVM* vm) {

	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	float x = wrenGetSlotDouble(vm, 2);
	float y = wrenGetSlotDouble(vm, 3);
	float z = wrenGetSlotDouble(vm, 4);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go){
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	float3 mod_trans_pos = c_trans->local->getPosition() + float3(x,y,z);
	c_trans->local->setPosition(mod_trans_pos);
}

void lookAt(WrenVM* vm) {

	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	float3 target = { (float)wrenGetSlotDouble(vm, 2), (float)wrenGetSlotDouble(vm, 3), (float)wrenGetSlotDouble(vm, 4) };

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for not existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);
	c_trans->local->LookAt(float3(c_trans->global->getPosition().x, target.y, c_trans->global->getPosition().z), target);
}

void getKey(WrenVM* vm) {
	int pressed_key = wrenGetSlotDouble(vm, 1);
	int mode = wrenGetSlotDouble(vm, 2);

	bool key_pressed = App->input->GetKey(pressed_key) == mode; 

	wrenSetSlotBool(vm, 0,  key_pressed);
}

void getMouseRaycastX(WrenVM* vm)
{
	float3 hit = App->scene->MousePickingHit();
	wrenSetSlotDouble(vm, 0, hit.x);
}

void getMouseRaycastY(WrenVM* vm)
{
	float3 hit = App->scene->MousePickingHit();
	wrenSetSlotDouble(vm, 0, hit.y);
}

void getMouseRaycastZ(WrenVM* vm)
{
	float3 hit = App->scene->MousePickingHit();
	wrenSetSlotDouble(vm, 0, hit.z);
}

void InstantiatePrefab(WrenVM* vm) {  

	std::string prefab_name = wrenGetSlotString(vm, 1);

	float x = wrenGetSlotDouble(vm, 2);
	float y = wrenGetSlotDouble(vm, 3);
	float z = wrenGetSlotDouble(vm, 4);

	float pitch = wrenGetSlotDouble(vm, 5);
	float yaw = wrenGetSlotDouble(vm, 6);
	float roll = wrenGetSlotDouble(vm, 7);

	std::string prefab_path = App->resources->getPrefabPath(prefab_name.c_str());

	App->scene->AskPrefabLoadFile(prefab_path.c_str(), float3(x,y,z), float3(pitch, yaw, roll));
}

void getGameObjectPosX(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);
	const char* mode = wrenGetSlotString(vm, 2);
	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	float ret = 0;

	if (strcmp(mode, "global") == 0)
		ret = c_trans->global->getPosition().x;
	else
		ret = c_trans->local->getPosition().x;

	wrenSetSlotDouble(vm, 0, ret);
}
void getGameObjectPosY(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);
	const char* mode = wrenGetSlotString(vm, 2);
	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	float ret = 0;

	if (strcmp(mode, "global") == 0)
		ret = c_trans->global->getPosition().y;
	else
		ret = c_trans->local->getPosition().y;

	wrenSetSlotDouble(vm, 0, ret);
}
void getGameObjectPosZ(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);
	const char* mode = wrenGetSlotString(vm, 2);
	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	float ret = 0;

	if (strcmp(mode, "global") == 0)
		ret = c_trans->global->getPosition().z;
	else
		ret = c_trans->local->getPosition().z;

	wrenSetSlotDouble(vm, 0, ret);
}

void KillGameObject(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}
	
	App->scene->deleteGameObject(go);
}

void MoveGameObjectForward(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);
	float speed = wrenGetSlotDouble(vm, 2);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	float3 mod_trans_pos = c_trans->local->getPosition() + (c_trans->local->Forward() * speed);
	c_trans->local->setPosition(mod_trans_pos);

}

void GetDeltaTime(WrenVM * vm)
{
	wrenSetSlotDouble(vm, 0, App->time->getDeltaTime());
}

void GetTimeScale(WrenVM * vm)
{
	wrenSetSlotDouble(vm, 0, App->time->getTimeScale());
}

void SetTimeScale(WrenVM * vm)
{
	App->time->setTimeScale(wrenGetSlotDouble(vm,1));
}

void getGameObjectPitch(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	wrenSetSlotDouble(vm, 0, c_trans->local->getRotationEuler().x);
}
void getGameObjectYaw(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	wrenSetSlotDouble(vm, 0, c_trans->local->getRotationEuler().y);
}
void getGameObjectRoll(WrenVM* vm) {
	uint gameObjectUUID = wrenGetSlotDouble(vm, 1);

	GameObject* go = App->scene->getGameObject(gameObjectUUID);
	if (!go) {
		app_log->AddLog("Script asking for none existing gameObject");
		return;
	}

	ComponentTransform* c_trans = (ComponentTransform*)go->getComponent(TRANSFORM);

	wrenSetSlotDouble(vm, 0, c_trans->local->getRotationEuler().z);
}

void getTime(WrenVM* vm) {
	wrenSetSlotDouble(vm, 0, SDL_GetTicks());
}

void sqrt(WrenVM* vm) {
	int number = wrenGetSlotDouble(vm, 1);
	wrenSetSlotDouble(vm, 0, sqrt(number));
}

void BreakPoint(WrenVM* vm) {
	std::string message = wrenGetSlotString(vm, 1);
	WrenType type = wrenGetSlotType(vm, 2);
	std::string var_name = wrenGetSlotString(vm, 3);
	int number = 0;
 	std::string str;
	bool boolean = false;

	switch (type) {
	case WREN_TYPE_NUM:
		number = wrenGetSlotDouble(vm, 2);
		break;
	case WREN_TYPE_BOOL:
		boolean = wrenGetSlotBool(vm, 2);
		break;
	case WREN_TYPE_STRING:
		str = wrenGetSlotString(vm, 2);
		break;
	case WREN_TYPE_UNKNOWN:
		app_log->AddLog("Variable in the break point is not a C native type");
		return;
	}

	return; // PUT A BREAK POINT HERE TO SEE THE VALUE AND NAME OF THE VARIABLE

}

void getButton(WrenVM* vm) {
	uint controller_id = wrenGetSlotDouble(vm, 1);
	CONTROLLER_BUTTON input = (CONTROLLER_BUTTON)(uint)wrenGetSlotDouble(vm, 2);
	KEY_STATE mode = (KEY_STATE)(uint)wrenGetSlotDouble(vm, 3);


	Controller* controller = App->input->getController(controller_id);

	if (!controller) {
		wrenSetSlotBool(vm, 0, false);
		app_log->AddLog("Asking for non-plugged controller %i", controller_id);
		return;
	}
	
	wrenSetSlotBool(vm, 0, controller->isPressed(input, mode));

}

void FindGameObjectByTag(WrenVM* vm) {
	std::string tag = wrenGetSlotString(vm, 1);

	std::list<GameObject*> tagged_gos = App->scene->getGameObjectsByTag(tag);

	// Initialized list
	wrenSetSlotNewList(vm, 0); 

	// Fill list
	wrenEnsureSlots(vm, 3);
	for(auto it = tagged_gos.begin(); it != tagged_gos.end(); it++){
		wrenSetSlotDouble(vm, 2, (*it)->getUUID());
		wrenInsertInList(vm, 0, -1, 2);
	}


	// retrun the list in slot 0
}