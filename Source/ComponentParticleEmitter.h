#ifndef __COMPONENTEMITTER_H__
#define __COMPONENTEMITTER_H__

#include "Component.h"
#include "Particle.h"
#include "Timer.h"
#include "ComponentTransform.h"
#include <list>
#include <queue>

#include "MathGeoLib/MathGeoLib.h"

#define MINSPAWNRATE 0.01f

enum ParticleAnimEvents {
	PARTICLE_CREATE,
	PARTICLE_AMOUNT_OF_EVENTS
};

enum AreaType
{
	SPHERE,
	AAB,
	AREA_NONE
};


struct shape
{
	shape() {};

	AreaType type = AAB;
	union
	{
		math::Sphere sphere;
		AABB aabb;
	};
};

class ComponentTransform;
class Billboard;

class ComponentParticleEmitter : public Component
{
	friend class ModuleUI;
	friend class Billboard;
	friend class ModuleRenderer3D;

public:
	ComponentParticleEmitter(GameObject* parent);
	ComponentParticleEmitter(GameObject* parent, const ComponentParticleEmitter& emitter);
	ComponentParticleEmitter(JSON_Object* deff, GameObject* parent);
	~ComponentParticleEmitter();

	bool Update(float dt);
	void Draw();
	bool DrawInspector(int id = 0) override;
	void Save(JSON_Object* json);


	void CreateParticle();
	void SpawnParticles(float dt);
	void UpdateParticles(float dt);
	void Set(float minSpeed = 0, float maxSpeed = 0, float minLife = 0, float maxLife = 0, float minSSize = 0, float maxSSize = 0, float minESize = 0, float maxESize = 0, float minSSpin = 0, float maxSSpin = 0, float minESpin = 0, float maxESpin = 0, Color sColorMin = White, Color sColorMax = White, Color eColorMin = White, Color eColorMax = White, float variation = 0, float3 direction = float3::zero, float3 gravity = float3::zero);

	float GetRandom(range<float> r);
	uint GetRandom(range<uint> r);
	float3 GetRandom(range<float3> r);
	Color GetRandom(range<Color> r);

	//Spawn area
	float3 GetRandomPosition();
	void DrawSpawnArea();
	void UpdateSpawnAreaPos();

	void SetArea(AreaType type);

	// Animation Events
	std::string EvTypetoString(int evt) override;
	int getEvAmount() override;
	void ProcessAnimationEvents(std::map<int, void*>& evts) override;

private:


	Timer emisionTimer;
	float emisionTime = 0;
	float period = 0;


	uint maxParticles = 0;
	uint currentParticles = 0;


	range <float> speed;
	range <float> particleLifetime;
	range <float> startSize;
	range <float> endSize;
	range <float> startSpin;
	range <float> endSpin;

	range <Color> startColor;
	range <Color> endColor;

	float3 gravity;
	float3 direction;
	float dirVartiation = 0;

	std::list<Particle*> particles;

	bool script_controlled = false;
	float emitterLifetime = 0;
	float time = 0;
	ComponentTransform* transform = nullptr;
	Mesh* mesh = nullptr;
	Billboard* billboard = nullptr;
	TransformMode transform_mode = GLOBAL;

	LCG lcg;

	//Area of spawn
	shape area;


};

#endif // !__COMPONENTEMITTER_H__