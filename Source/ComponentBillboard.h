#ifndef __COMPONENTBILLBOARD_H__
#define __COMPONENTBILLBOARD_H__

#include "Component.h"
#include "Billboard.h"

class ComponentTransform;

class ComponentBillboard : public Component
{
	friend class Billboard;
public:
	ComponentBillboard(GameObject* parent, Material* tex = nullptr);
	ComponentBillboard(GameObject* parent, const Billboard& billboard);
	ComponentBillboard(JSON_Object* deff, GameObject* parent);
	~ComponentBillboard();

	bool Update(float dt);
	void Render() const;
	void Draw();
	bool DrawInspector(int id = 0) override;

	void Save(JSON_Object* config);

	Mesh* getMesh() { return mesh; };

	Billboard* billboard = nullptr;

private:

	ComponentTransform* transform = nullptr;
	Mesh* mesh = nullptr;
};

#endif // !__COMPONENTBILLBOARD_H__