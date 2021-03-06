#ifndef _COMPONENTPROGRESSUI_
#define _COMPONENTPROGRESSUI_
#include "Component.h"
#include "MathGeoLib/Math/float2.h"


class ComponentRectTransform;
class ComponentImageUI;
class ResourceTexture;

class ComponentProgressBarUI :
	public Component
{
public:
	ComponentProgressBarUI(GameObject* parent);
	ComponentProgressBarUI(JSON_Object* deff, GameObject* parent);
	~ComponentProgressBarUI();

	bool Update(float dt)override;
	bool DrawInspector(int id = 0) override;

	 void setPercent(float _percent);
	inline const float getPercent() { return percent; }

	inline void setPos(float2 pos);
	void setWidth(float width);
	inline const float2 getPos();

	void setInteriorWidth(float width);
	inline const float getInteriorWidth() { return initialWidth; }
	void setInteriorDepth(float depth);
	const float getInteriorDepth();

	inline const ResourceTexture* getResourceTexture() { return texBar; }
	void setResourceTexture(ResourceTexture* tex);
	inline void DeassignTexture() { texBar = nullptr; }

	inline const ResourceTexture* getResourceTextureInterior() { return intTexBar; }
	void setResourceTextureInterior(ResourceTexture* tex);
	inline void DeassignTextureInterior() { intTexBar = nullptr; }

	

	void Save(JSON_Object* config) override;

private:

	
	float percent = 100.f;
	float initialWidth;

	ComponentRectTransform * barTransform = nullptr;
	ComponentRectTransform * intBarTransform = nullptr;

	ComponentImageUI* bar = nullptr;
	ComponentImageUI* intBar = nullptr;

	ResourceTexture* texBar = nullptr;
	ResourceTexture* intTexBar = nullptr;
};


#endif