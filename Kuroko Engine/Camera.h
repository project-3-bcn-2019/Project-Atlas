#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "MathGeoLib\Math\float4x4.h"
#include "MathGeoLib\Math\float3.h"
#include "MathGeoLib\Geometry\Frustum.h"
#include "MathGeoLib\Geometry\OBB.h"

#include "Globals.h"

class ComponentCamera;
class FrameBuffer;
class Texture;

#define EXTRA_DIST 3.0f

struct FrameBuffer
{
	Texture* tex = nullptr;
	Texture* depth_tex = nullptr;

	uint size_x = 0;
	uint size_y = 0;
};

class Camera
{
	friend class ModuleCamera3D;
	friend class ComponentCamera;
public:
	Camera(float3 position = float3::zero, float3 reference = float3(0.0f, 0.0f, 5.0f), float n_plane = 0.5f, float f_plane = 1250.0f, float hor_fov = 90.0f, float ver_fov = 59.0f);
	~Camera();

	void LookAt(const float3 &Spot);
	void Move(const float3 &Movement);
	void FitToSizeSelectedGeometry(float extra_distance = EXTRA_DIST);
	void LookAtSelectedGeometry();
	void Reset();

	Frustum* getFrustum() const { return frustum; }; 
	FrameBuffer* getFrameBuffer() const { return frame_buffer; };
	ComponentCamera* getParent() const { return attached_to; }; // can be null

	void setFrameBuffer(FrameBuffer* fb) { frame_buffer = fb; };
	void setFrustum(Frustum* f) { frustum = f; };
	void setFov(float hor_fov, float ver_fov);
	void setPlaneDistance(float n_plane, float f_plane);
	bool frustumCull(const OBB& obb);  // returns true if inside the frustum

	void updateFrustum();
	void initFrameBuffer();

public:

	float3 X = { 1.0f,0.0f,0.0f };
	float3 Y = { 0.0f,1.0f,0.0f };
	float3 Z = { 0.0f,0.0f,1.0f };
	float3 Reference = { 0.0f,0.0f,5.0f };
	bool active = false;

private:

	ComponentCamera* attached_to = nullptr;  // can be null
	FrameBuffer* frame_buffer = nullptr;
	Frustum* frustum = nullptr;

};


#endif
