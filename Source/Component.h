#ifndef _COMPONENT_
#define _COMPONENT_


#define DELTA_ALPHA 0.01f

#include "Parson\parson.h"
#include "Globals.h"
#include "Random.h"
#include <vector>
#include <string>
class GameObject;

enum Component_type { NONE, MESH, TRANSFORM, C_AABB, CAMERA, SCRIPT, BONE, ANIMATION, CANVAS, RECTTRANSFORM, UI_IMAGE, UI_CHECKBOX, UI_BUTTON, UI_TEXT, AUDIOLISTENER, AUDIOSOURCE
};

class Component
{
public:

	Component(GameObject* gameobject, Component_type type) : parent(gameobject), type(type), uuid(random32bits()) {};
	virtual ~Component() {};

	virtual bool Update(float dt) { return true; };
	virtual void Draw() const {};

	bool isActive() { return is_active; } const
	void setActive(bool state) { is_active = state; }

	Component_type getType() const { return type; };
	GameObject* getParent() const { return parent; };
	void setParent(GameObject* new_parent) { parent = new_parent; } // Can recieve nullptr
	uint getUUID() const { return uuid; }

	virtual void Save(JSON_Object* config) {}

	// For animation curr
	bool AnimSel = false;

protected:

	bool is_active = true;
	const Component_type type = NONE;
	GameObject* parent = nullptr;
	uint uuid = 0;

public:
	// Helper Functions
	std::string TypeToString();
};
#endif