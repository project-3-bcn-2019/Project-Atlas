#include "ComponentTextUI.h"
#include "GameObject.h"
#include "FontManager.h"
#include "Application.h"
#include "ModuleRenderer3D.h"
#include "MathGeoLib/Math/float4x4.h"
#include "ModuleCamera3D.h"
#include <vector>

#include "glew-2.1.0\include\GL\glew.h"
#include "ImGui/imgui.h"


ComponentTextUI::ComponentTextUI(GameObject* parent) : Component(parent, UI_TEXT)
{
	static const float uvs[] = {
	0, 1,
	1, 1,
	1, 0,
	0, 0
	};

	texCoords = new float2[4];
	memcpy(texCoords, uvs, sizeof(float2) * 4);

	rectTransform = (ComponentRectTransform*)parent->getComponent(RECTTRANSFORM);

	label.font = App->fontManager->GetFont(DEFAULT_FONT);

	//App->fontManager->LoadFont("federalescort.ttf", 20);
	SetText("Default Text");
	rectTransform->setWidth((labelFrame[3].x - labelFrame[0].x));
	rectTransform->setHeight((labelFrame[2].y - labelFrame[3].y));

}

ComponentTextUI::ComponentTextUI(JSON_Object * deff, GameObject * parent) : Component(parent, UI_TEXT)
{
	static const float uvs[] = {
	0, 1,
	1, 1,
	1, 0,
	0, 0
	};

	texCoords = new float2[4];
	memcpy(texCoords, uvs, sizeof(float2) * 4);

	rectTransform = (ComponentRectTransform*)parent->getComponent(RECTTRANSFORM);

	std::string fontName = std::string(json_object_dotget_string(deff, "fontSource"));
	int scale = json_object_dotget_number(deff, "scale");
	label.font = App->fontManager->LoadFont(fontName.c_str(), scale);
	label.font->scale = scale;
	
	SetText(json_object_dotget_string(deff, "text"));
	label.color = float3(json_object_dotget_number(deff, "colorX"), json_object_dotget_number(deff, "colorY"), json_object_dotget_number(deff, "colorZ"));

	rectTransform->setWidth((labelFrame[3].x - labelFrame[0].x));
	rectTransform->setHeight((labelFrame[2].y - labelFrame[3].y));
}


ComponentTextUI::~ComponentTextUI()
{
	CleanCharPlanes();
	rectTransform = nullptr;
	label.font = nullptr;
	delete[] texCoords;
	texCoords = nullptr;
}

bool ComponentTextUI::Update(float dt)
{
	return true;
}


void ComponentTextUI::Draw()
{
	App->renderer3D->orderedUI.push(this);

	if (App->camera->current_camera == App->camera->editor_camera && !App->is_game)
		App->renderer3D->opaqueMeshes.push_back(this);
}


void ComponentTextUI::Render() const
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	int charNum = 0;
	int line = 0;
	int lettersDrawn = 0;
	float lineDistance = 0;
	Character* firstChar = label.font->GetCharacter((GLchar)label.text[0]);

	float3 cursor = float3(label.textOrigin.x, label.textOrigin.y, 0);
	if (charPlanes.empty()) {
		int i = 0;//FillCharPlanes();
	}
	if (charPlanes.empty() || label.font->charsList.empty()) {
		return;
	}
	for (std::vector<CharPlane*>::const_iterator it = charPlanes.begin(); it != charPlanes.end(); it++, charNum++)
	{
		lettersDrawn++;
		float4x4 viewMatrix = float4x4::identity; // modelView Matrix
		float4x4 globalMatrix = float4x4::identity; // global Matrix ( from RectTransform)
		float4x4 incrementMatrix = float4x4::identity;// advance for every char Matrix ( from cursor)

		float3	pos = float3(rectTransform->getGlobalPos().x, rectTransform->getGlobalPos().y, 0);
		globalMatrix.SetTranslatePart(pos); 	// set global Mat	

		GLfloat matrix[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
		viewMatrix.Set((float*)matrix);		// set MV mat


		Character* currChar = label.font->GetCharacter((GLchar)label.text[charNum]);
		Character* nextChar = nullptr;

		if (!currChar) {
			currChar = label.font->GetCharacter(' ');//if not avaliable glyph, put a space (like "�")
		}

		if (charNum < label.text.size() - 1 && label.text.size()>0)
			nextChar = label.font->GetCharacter((GLchar)label.text[charNum + 1]);
		else
			nextChar = label.font->GetCharacter((GLchar)"");
		if (lettersDrawn != 1) {
			cursor.x += offsetPlanes[charNum].x;
		}
		cursor.y = label.textOrigin.y + offsetPlanes[charNum].y + -line * lineSpacing;

		if (charNum == 0) // to start exacly where the component text rect is
		{
			lineDistance = currChar->size.x / 1.5f;
		}
		else
			lineDistance += offsetPlanes[charNum].x;

		if (charNum == charPlanes.size() - 1)
			lineDistance += currChar->size.x / 1.5f;

		if ((lineDistance - 10) > rectTransform->getWidth()) //horizontal limit 
		{
			line++;
			cursor.x = label.textOrigin.x;
			cursor.y = offsetPlanes[charNum].y + label.textOrigin.y - (line * lineSpacing);
			lineDistance = 0;
		}
		if ((offsetPlanes[charNum].y + label.textOrigin.y)*(line + 1) > rectTransform->getHeight()) { //vertical limit 
			return;

		}

		incrementMatrix.SetTranslatePart(cursor); // set cursor mat

		if ((*it)->textureID != -1) {

			glEnable(GL_TEXTURE_2D);
			glColor4f(label.color.x, label.color.y, label.color.z, alpha);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf((GLfloat*)(((globalMatrix * incrementMatrix).Transposed() * viewMatrix).v)); // multiplies all mat

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindBuffer(GL_ARRAY_BUFFER, 0); //resets the buffer
			glBindTexture(GL_TEXTURE_2D, 0);

			glLineWidth(4.0f);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0);

			glBindTexture(GL_TEXTURE_2D, (*it)->textureID);
			glTexCoordPointer(2, GL_FLOAT, 0, &(texCoords[0]));

			glBindBuffer(GL_ARRAY_BUFFER, (*it)->vertexID);
			glVertexPointer(3, GL_FLOAT, 0, NULL);
			glDrawArrays(GL_QUADS, 0, 4);

			glLineWidth(1.0f);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0); //resets the buffer
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_TEXTURE_2D);

			glColor3f(1.0f, 1.0f, 1.0f);


			if (drawCharPanel) {
				glBegin(GL_LINES);
				glColor3f(0.0f, 1.0f, 1.0f);

				glVertex3f((*it)->vertex[0].x, (*it)->vertex[0].y, (*it)->vertex[0].z);
				glVertex3f((*it)->vertex[1].x, (*it)->vertex[1].y, (*it)->vertex[1].z);

				glVertex3f((*it)->vertex[1].x, (*it)->vertex[1].y, (*it)->vertex[1].z);
				glVertex3f((*it)->vertex[2].x, (*it)->vertex[2].y, (*it)->vertex[2].z);

				glVertex3f((*it)->vertex[2].x, (*it)->vertex[2].y, (*it)->vertex[2].z);
				glVertex3f((*it)->vertex[3].x, (*it)->vertex[3].y, (*it)->vertex[3].z);

				glVertex3f((*it)->vertex[3].x, (*it)->vertex[3].y, (*it)->vertex[3].z);
				glVertex3f((*it)->vertex[0].x, (*it)->vertex[0].y, (*it)->vertex[0].z);

				glEnd();
			}

			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf((GLfloat*)viewMatrix.v);

			glColor3f(1.0f, 1.0f, 1.0f);

		}
		if (drawLabelrect) {
			glBegin(GL_LINES);
			glColor3f(0.0f, 0.0f, 1.0f);

			glVertex3f(labelFrame[0].x, labelFrame[0].y, labelFrame[0].z);
			glVertex3f(labelFrame[1].x, labelFrame[1].y, labelFrame[1].z);

			glVertex3f(labelFrame[1].x, labelFrame[1].y, labelFrame[1].z);
			glVertex3f(labelFrame[2].x, labelFrame[2].y, labelFrame[2].z);

			glVertex3f(labelFrame[2].x, labelFrame[2].y, labelFrame[2].z);
			glVertex3f(labelFrame[3].x, labelFrame[3].y, labelFrame[3].z);

			glVertex3f(labelFrame[3].x, labelFrame[3].y, labelFrame[3].z);
			glVertex3f(labelFrame[0].x, labelFrame[0].y, labelFrame[0].z);

			glEnd();
		}
		glColor3f(1.0f, 1.0f, 1.0f);
	}
}

bool ComponentTextUI::DrawInspector(int id)
{
	if (ImGui::CollapsingHeader("UI Text"))
	{
		static const int maxSize = 32;
		if (ImGui::InputText("Label Text", (char*)label.text.c_str(), maxSize)) {
			SetText(label.text.c_str());
		}
		if (ImGui::SliderFloat("Scale", &(label.font->scale), 8, MAX_CHARS, "%0.f")) {
			SetFontScale(label.font->scale);
		}
		ImGui::Checkbox("Draw Characters Frame", &drawCharPanel);
		ImGui::Checkbox("Draw Label Frame", &drawLabelrect);
		std::string currentFont = label.font->fontSrc;
		if (ImGui::BeginCombo("Fonts", currentFont.c_str()))
		{
			std::vector<std::string> fonts = App->fontManager->singleFonts;

			for (int i = 0; i < fonts.size(); i++)
			{
				bool isSelected = false;

				if (strcmp(currentFont.c_str(), fonts[i].c_str()) == 0) {
					isSelected = true;
				}

				if (ImGui::Selectable(fonts[i].c_str(), isSelected)) {
					std::string newFontName = std::string(fonts[i].c_str());
					std::string newFontExtension = std::string(fonts[i].c_str());
					App->fs.getFileNameFromPath(newFontName);
					App->fs.getExtension(newFontExtension);
					newFontName += newFontExtension;
					SetFont(newFontName.c_str());

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}

				}

			}
			ImGui::EndCombo();

		}
		ImGui::Spacing();
		ImGui::ColorPicker3("Color##2f", (float*)&label.color);
	}

	return true;
}

void ComponentTextUI::Save(JSON_Object * config)
{
	json_object_set_string(config, "type", "UItext");
	json_object_set_string(config, "text", label.text.c_str());
	json_object_set_number(config, "colorX", label.color.x);
	json_object_set_number(config, "colorY", label.color.y);
	json_object_set_number(config, "colorZ", label.color.z);
	if (label.font) {
		json_object_set_string(config, "fontSource", label.font->fontSrc.c_str());
		json_object_set_number(config, "scale", label.font->scale);
	}
}

void ComponentTextUI::CharPlane::GenBuffer(float2 size)
{
	float2 halfSize = size / 2;

	const float vtx[] = { -halfSize.x ,-halfSize.y, 0,
								halfSize.x ,-halfSize.y, 0,
								halfSize.x, halfSize.y, 0,
								-halfSize.x, halfSize.y, 0 };
	vertex = new float3[4];
	memcpy(vertex, vtx, sizeof(float3) * 4);

	glGenBuffers(1, (GLuint*) &(vertexID));
	glBindBuffer(GL_ARRAY_BUFFER, vertexID); // set the type of buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * 4, &vertex[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void ComponentTextUI::ShiftNewLine(float3& cursor, int& current_line, int i)
{
	current_line++;
	cursor.x = label.textOrigin.x;
	cursor.y = offsetPlanes[i].y + label.textOrigin.y - (current_line * lineSpacing);

}

void ComponentTextUI::SetFontScale(int scale) {
	std::string name = label.font->fontSrc;
	//App->fontManager->RemoveFont(name.c_str());	
	label.font = App->fontManager->LoadFont(name.c_str(), scale);
	SetText(label.text.c_str());

}

void ComponentTextUI::SetFont(const char* font) {

	label.font = App->fontManager->GetFont(font);
	SetText(label.text.c_str());

}


void ComponentTextUI::CleanCharPlanes()
{
	for (int i = charPlanes.size() - 1; i >= 0; i--) {

		delete[] charPlanes[i]->vertex;
		charPlanes[i]->vertex = nullptr;
		glDeleteBuffers(1,(GLuint*) &charPlanes[i]->vertexID);
		delete charPlanes[i];
		charPlanes[i] = nullptr;
	}
	charPlanes.clear();
}

void ComponentTextUI::SetText(const char * txt)
{

	label.text = txt;
	CleanCharPlanes();

	FillCharPlanes();

	if (label.text.size() != 0) {//Adjust the container plane to the new text size 
		label.textOrigin.x = initOffsetX;
		EnframeLabel(labelFrame);
	}

}
void ComponentTextUI::EnframeLabel(float3 * points) {

	labelFrame[0] = GetCornerLabelPoint(0) + GetCornerLabelPoint(3);//bottomleft;
	labelFrame[1] = GetCornerLabelPoint(0) + GetCornerLabelPoint(1);//topleft;
	labelFrame[2] = GetCornerLabelPoint(2) + GetCornerLabelPoint(1);//topright;
	labelFrame[3] = GetCornerLabelPoint(2) + GetCornerLabelPoint(3);//bottomrigth;
}

float3 ComponentTextUI::GetCornerLabelPoint(int corner) {
	float3 ret = float3(0, 0, 0);
	switch (corner) {
	case 0: {//minX		
		ret = { charPlanes[0]->vertex[0].x + rectTransform->getGlobalPos().x + label.textOrigin.x,0,0 };
		break;
	}
	case 1: {//maxY		
		float max_y = -100;

		int counter = 0;
		CharPlane* planeMaxY = nullptr;

		for (std::vector<CharPlane*>::iterator it = charPlanes.begin(); it != charPlanes.end(); it++, counter++)
		{
			float offsetY = offsetPlanes[counter].y;

			if (max_y < offsetY)
			{
				max_y = offsetY;
				planeMaxY = (*it);
			}

		}
		ret = { 0, planeMaxY->vertex[2].y + max_y + rectTransform->getGlobalPos().y + label.textOrigin.y, 0 };
		break;
	}
	case 2: {//maxX		
		CharPlane* planeMaxX = nullptr;
		int counter = 0;
		float offset = -1;

		for (std::vector<CharPlane*>::iterator it = charPlanes.begin(); it != charPlanes.end(); it++, counter++)
		{
			if (counter == charPlanes.size() - 1)
			{
				planeMaxX = (*it);
				offset += offsetPlanes[counter].x;
				break;
			}

			offset += offsetPlanes[counter].x;
		}


		ret = { planeMaxX->vertex[2].x + offset + rectTransform->getGlobalPos().x + label.textOrigin.x, 0, 0 };

		break;
	}
	case 3: {//minY	

		float minY = 100;
		CharPlane* planeMinY = nullptr;
		int counter = 0;

		for (std::vector<CharPlane*>::iterator it = charPlanes.begin(); it != charPlanes.end(); it++, counter++)
		{
			float offsetY = offsetPlanes[counter].y;

			if (minY > offsetY && offsetY != 0) //if plane_min_y == 0 it's an space
			{
				minY = offsetY;
				planeMinY = (*it);
			}
		}

		if (planeMinY != nullptr) {
			ret = { 0, planeMinY->vertex[0].y + minY + rectTransform->getGlobalPos().y + label.textOrigin.y, 0 };
		}
		else {
			ret = { 0, 0 + minY + rectTransform->getGlobalPos().y + label.textOrigin.y, 0 };
		}
		break;
	}
	}
	return ret;
}

void ComponentTextUI::AddCharPanel(char character, bool first)
{
	FT_Load_Char(label.font->face, (GLchar)character, FT_LOAD_RENDER);
	if (label.font->face->glyph == nullptr) { return; }
	float2 size = { (float)label.font->face->glyph->bitmap.width, (float)label.font->face->glyph->bitmap.rows };
	CharPlane* nwCharPlane = new CharPlane();
	nwCharPlane->GenBuffer(size);
	if (first) { initOffsetX = size.x / 2; }
	if (character != ' ') {
		nwCharPlane->textureID = label.font->GetCharacterTexture(character);
	}
	charPlanes.push_back(nwCharPlane);
}

void ComponentTextUI::FillCharPlanes()
{
	bool first = false;
	for (int i = 0; i < label.text.size(); i++) {
		if (i == 0) { first = true; }
		AddCharPanel(label.text[i], first);
	}

	int counter = 0;
	offsetPlanes.clear();
	for (std::vector<CharPlane*>::const_iterator it = charPlanes.begin(); it != charPlanes.end(); it++, counter++)
	{
		float distancex = 0;
		float distancey = 0;

		Character* currChar = label.font->GetCharacter(label.text[counter]);
		Character* prevChar = nullptr;
		if (!currChar) {
			currChar = label.font->GetCharacter(' ');

		}
		if (counter - 1 >= 0)
			prevChar = label.font->GetCharacter(label.text[counter - 1]);

		if (prevChar)
			distancex = prevChar->advance / 2.f;

		distancex += currChar->advance / 2.f;

		//Y offset
		float size = (float)currChar->size.y;
		float bearingy = (float)currChar->bearing.y;
		float center = (currChar->size.y / 2);
		distancey = -(size - bearingy) + center;

		if (counter == 0)
		{
			distancex = 0;
		}

		offsetPlanes.push_back({ distancex, distancey, 0 });
	}


}