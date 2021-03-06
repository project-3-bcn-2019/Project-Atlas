#include "ComponentRectTransform.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ModuleTimeManager.h"
#include "ModuleRenderer3D.h"
#include "ModuleCamera3D.h"

#include "glew-2.1.0\include\GL\glew.h"
#include "ImGui/imgui.h"


ComponentRectTransform::ComponentRectTransform(GameObject* parent) : Component(parent, RECTTRANSFORM)
{
	setWidth(100.0f);
	setHeight(100.0f);

	debug_draw = true;
	
	static const float vtx[] = {
		0, 0, 0,
		1, 0, 0,
		1, 1, 0,
		0, 1, 0
	};
	rect.vertex = new float3[4];
	memcpy(rect.vertex, vtx, sizeof(float3) * 4);
	if (parent!=nullptr && parent->getParent() != nullptr && parent->getParent()->getComponent(CANVAS) == nullptr && parent->getParent()->getComponent(RECTTRANSFORM) != nullptr) {
		ComponentRectTransform* parentRect = (ComponentRectTransform*)parent->getParent()->getComponent(RECTTRANSFORM);
		rect.local = rect.global-parentRect->getGlobalPos();
	}


	GenBuffer();
}

ComponentRectTransform::ComponentRectTransform(JSON_Object * deff, GameObject * parent) : Component(parent, RECTTRANSFORM) {
	
	// Position
	//setLocalPos(float2(json_object_get_number(deff, "localX"), json_object_get_number(deff, "localY")));
	rect.local.x = json_object_get_number(deff, "localX");
	rect.local.y = json_object_get_number(deff, "localY");
	rect.global.x = json_object_get_number(deff, "globalX");
	rect.global.y = json_object_get_number(deff, "globalY");

	rect.anchor.x = json_object_get_number(deff, "anchorX");
	rect.anchor.y = json_object_get_number(deff, "anchorY");

	//Dimension
	rect.width = json_object_get_number(deff, "width");
	rect.height = json_object_get_number(deff, "height");

	rect.depth = json_object_get_number(deff, "depth");


	debug_draw = json_object_get_boolean(deff, "debug");

	static const float vtx[] = {
		0, 0, 0,
		1, 0, 0,
		1, 1, 0,
		0, 1, 0
	};
	   
	rect.vertex = new float3[4];
	memcpy(rect.vertex, vtx, sizeof(float3) * 4);

	GenBuffer();
}


ComponentRectTransform::~ComponentRectTransform()
{

	glDeleteBuffers(1, (GLuint*)&rect.vertexID);
	RELEASE_ARRAY( rect.vertex);

}

bool ComponentRectTransform::Update(float dt)
{
	return true;
}


void ComponentRectTransform::Draw()
{
	App->renderer3D->orderedUI.push(this);
	if (App->camera->current_camera == App->camera->editor_camera && !App->is_game)
		App->renderer3D->opaqueMeshes.push_back(this);
}


void ComponentRectTransform::Render() const
{
	if (debug_draw && App->time->getGameState() != GameState::PLAYING) {

		glPushMatrix();
		float4x4 globalMat;
		globalMat = float4x4::FromTRS(float3(rect.global.x, rect.global.y, rect.depth), Quat(0, 0, 0, 0), float3(rect.width, rect.height, 0));
		glMultMatrixf(globalMat.Transposed().ptr());

		glEnableClientState(GL_VERTEX_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0); //resets the buffer

		glLineWidth(1.5f);
		glColor3f(1.0f, 0.0f, 0.0f); // red
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // draws quad unfilled

		glBindBuffer(GL_ARRAY_BUFFER, rect.vertexID);
		glVertexPointer(3, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_QUADS, 0, 4);

		glBindBuffer(GL_ARRAY_BUFFER, 0); //resets the buffer
		glDisableClientState(GL_VERTEX_ARRAY);
		glPopMatrix();

		App->scene->global_wireframe ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // returns back to previous polygon fill mode

		glLineWidth(3.0f);

		//----Anchor Point
		glBegin(GL_LINES);
		glVertex3f(rect.anchor.x, rect.anchor.y + 0.1f, rect.depth);
		glVertex3f(rect.anchor.x, rect.anchor.y - 0.1f, rect.depth);
		glVertex3f(rect.anchor.x + 0.1f, rect.anchor.y, rect.depth);
		glVertex3f(rect.anchor.x - 0.1f, rect.anchor.y, rect.depth);
		glEnd();

		glColor3f(1.0f, 1.0f, 1.0f);
		glLineWidth(1.0f);
	}
}

bool ComponentRectTransform::DrawInspector(int id)
{
	
		if (ImGui::CollapsingHeader("Rect Transform"))
		{

			static float2 position;
			static float width;
			static float height;
			static float depth;

			position = getLocalPos();
			width = getWidth();
			height = getHeight();
			depth = getDepth();

			//position
			ImGui::Text("Position:");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);
			if (ImGui::DragFloat2("##p", (float*)&position, 0.5f,0.0f,0.0f, "%.01f")) { setPos(position); }
			//Width
			ImGui::Text("Dimensions:");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			if (ImGui::DragFloat("##h", &width, 0.5f, 0.0f, 0.0f, "%.01f")) { setWidth(width); }
			//Height
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			if (ImGui::DragFloat("##w", &height, 0.5f, 0.0f, 0.0f, "%.01f")) {
				setHeight(height);
			}
			//Depth
			ImGui::Text("Depth:");
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.15f);
			if (ImGui::DragFloat("##d", &depth, 0.01f, 0.0f, 0.0f, "%.02f")) { setDepth(depth); }

			ImGui::Checkbox("Debug draw", &debug_draw);
			if (parent->getParent() != nullptr) {
				ComponentRectTransform* parent_rect_transform = (ComponentRectTransform*)parent->getParent()->getComponent(Component_type::RECTTRANSFORM);
				if (ImGui::TreeNode("Basic Positions")) {
					if (ImGui::Button("Top Left")) {
						float height = parent_rect_transform->getHeight() - rect.height;
						setPos(float2(0, height));
					}
					ImGui::SameLine();
					if (ImGui::Button("Top center")) {
						float height = parent_rect_transform->getHeight() - rect.height;
						float width = parent_rect_transform->getWidth() / 2 - (rect.width / 2);
						setPos(float2(width, height));
					}
					ImGui::SameLine();
					if (ImGui::Button("Top Right")) {
						float height = parent_rect_transform->getHeight() - rect.height;
						float width = parent_rect_transform->getWidth() - rect.width;
						setPos(float2(width, height));
					}
					if (ImGui::Button("Center Left")) {
						float height = parent_rect_transform->getHeight() / 2 - (rect.height / 2);
						setPos(float2(0, height));
					}
					ImGui::SameLine();
					if (ImGui::Button("Center")) {
						setToMid();
					}
					ImGui::SameLine();
					if (ImGui::Button("Center Right")) {
						float height = parent_rect_transform->getHeight() / 2 - (rect.height / 2);
						float width = parent_rect_transform->getWidth() - rect.width;
						setPos(float2(width, height));
					}
					if (ImGui::Button("Down Left")) {
						setPos(float2(0, 0));
					}
					ImGui::SameLine();
					if (ImGui::Button("Down center")) {
						float width = parent_rect_transform->getWidth() / 2 - (rect.width / 2);
						setPos(float2(width, 0));
					}

					ImGui::SameLine();
					if (ImGui::Button("Down Right")) {
						float width = parent_rect_transform->getWidth() - rect.width;
						setPos(float2(width, 0));
					}
					//ImGui::SameLine();
					ImGui::TreePop();
				}
			}
		}
		return true;
}

const float2 ComponentRectTransform::getMid() const
{
	return float2(rect.width / 2, rect.height / 2);
}

void ComponentRectTransform::setToMid()
{
	ComponentRectTransform* parent_rect_transform = (ComponentRectTransform*)parent->getParent()->getComponent(Component_type::RECTTRANSFORM);
	float2 newPos = parent_rect_transform->getMid();
	newPos.x -= rect.width / 2;
	newPos.y -= rect.height / 2;
	setPos(newPos);
}

void ComponentRectTransform::Save(JSON_Object * config)
{
	// Set component type
	json_object_set_string(config, "type", "rectTransform");

	// Position
	json_object_set_number(config, "localX", rect.local.x);
	json_object_set_number(config, "localY", rect.local.y);
	json_object_set_number(config, "globalX", rect.global.x);
	json_object_set_number(config, "globalY", rect.global.y);

	json_object_set_number(config, "anchorX", rect.anchor.x);
	json_object_set_number(config, "anchorY", rect.anchor.y);

	//Dimension
	json_object_set_number(config, "width", rect.width);
	json_object_set_number(config, "height", rect.height);
	json_object_set_number(config, "depth", rect.depth);

	json_object_set_boolean(config, "debug", debug_draw);
	

}


void ComponentRectTransform::setPos(float2 pos)
{
	float2 dist = pos - rect.local;
	rect.local = pos;
	rect.global +=dist;	

	UpdateGlobalMatrixRecursive(this);
	UpdateAnchorPos();
}

void ComponentRectTransform::setWidth(float width)
{
	float dif = width + rect.width;
	rect.width = width;
	rect.anchor.x += dif;

	UpdateAnchorPos();
}

void ComponentRectTransform::setHeight(float height)
{
	float dif = height + rect.height;
	rect.height = height;
	rect.anchor.y += dif;

	UpdateAnchorPos();
}


void ComponentRectTransform::GenBuffer()
{
	glGenBuffers(1, (GLuint*) &(rect.vertexID));
	glBindBuffer(GL_ARRAY_BUFFER, rect.vertexID); // set the type of buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * 4, &rect.vertex[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void ComponentRectTransform::UpdateGlobalMatrixRecursive(ComponentRectTransform* rect)
{
	std::list<GameObject*> childs = std::list<GameObject*>();
	rect->getParent()->getChildren(childs);	

	for (auto it = childs.begin(); it != childs.end(); it++) {
		ComponentRectTransform* childRect = (ComponentRectTransform*)(*it)->getComponent(RECTTRANSFORM);
		childRect->setGlobalPos(rect->getGlobalPos() + childRect->getLocalPos());

		childRect->UpdateAnchorPos();
		UpdateGlobalMatrixRecursive(childRect);
	}
}

void ComponentRectTransform::UpdateAnchorPos()
{
	rect.anchor.x = rect.global.x + rect.width / 2;
	rect.anchor.y = rect.global.y + rect.height / 2;
}
