#ifndef __PANELINSPECTOR_H__
#define __PANELINSPECTOR_H__

#include "Panel.h"
#include <list>

class GameObject;
class Component;

class PanelObjectInspector : public Panel
{
public:

	PanelObjectInspector(const char* name, bool active);
	~PanelObjectInspector();

	void Draw();

	void DrawChildedInspector(GameObject* object);

private:
	void DrawTagSelection(GameObject * object);
};

#endif