#include "Application.h"
#include "ModuleShaders.h"
#include "ModuleRenderer3D.h"
#include "ModuleWindow.h"
#include "Globals.h"
#include "Applog.h"
#include "FileSystem.h"

#include "glew-2.1.0/include/GL/glew.h"
#include "SDL\include\SDL_opengl.h"
#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */
#pragma comment( lib, "glew-2.1.0/lib/glew32.lib")
#pragma comment( lib, "glew-2.1.0/lib/glew32s.lib")


ModuleShaders::ModuleShaders(Application* app, bool start_enabled):Module(app,start_enabled)
{
	name = "Shaders";
}

ModuleShaders::~ModuleShaders()
{
	RELEASE(defVertexShader);
	RELEASE(defFragmentShader);
	RELEASE(defShaderProgram);
}

bool ModuleShaders::Init(const JSON_Object * config)
{
	bool ret = true;

	defVertexShader = new Shader(VERTEX);
	defFragmentShader = new Shader(FRAGMENT);
	defShaderProgram = new ShaderProgram();

	animationShader = new Shader(VERTEX);
	animationShaderProgram = new ShaderProgram();


	return ret;
}

bool ModuleShaders::InitializeDefaulShaders()
{
	//Default shaders
	bool findFile = App->fs.ExistisFile(defaultVertexFile.c_str(), LIBRARY_MATERIALS, ".vex");
	if (findFile)
	{
		defVertexShader->script = App->fs.ImportFile(defaultVertexFile.c_str());
	}
	else
	{
		CreateDefVertexShader();
	}

	CompileShader(defVertexShader);
	defShaderProgram->shaders.push_back(defVertexShader->shaderId);

	findFile = App->fs.ExistisFile(defaultFragmentFile.c_str(), LIBRARY_MATERIALS, ".frag");
	if (findFile)
	{
		defVertexShader->script = App->fs.ImportFile(defaultVertexFile.c_str());
	}
	else
	{
		CreateDefFragmentShader();
	}

	CompileShader(defFragmentShader);
	defShaderProgram->shaders.push_back(defFragmentShader->shaderId);

	CompileProgram(defShaderProgram);

	//Animation Shaders
	CompileShader(animationShader);
	animationShaderProgram->shaders.push_back(animationShader->shaderId);

	return true;
}

void ModuleShaders::CreateDefVertexShader()
{
	defVertexShader->script =
	"#version 330\n"

	"layout(location = 0) in vec3 position;\n"
	"layout(location = 1) in vec4 color;\n"
	"layout(location = 2) in vec2 texCoord;\n"
	"layout(location = 3) in vec3 normal;\n"

	"out vec4 ourColor;\n"
	"out vec2 TexCoord;\n"
	"out vec3 ret_normal;\n"
	"out vec3 FragPos;\n"

	"uniform mat4 model_matrix;\n"
	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"

	"void main()\n"
	"{\n"
		"FragPos=vec3(model_matrix*vec4(position,1.0));\n"
		"gl_Position = projection * view * model_matrix * vec4(position,1.0f);\n"
		"ourColor = color;\n"
		"TexCoord = texCoord;\n"
		"ret_normal = normal;\n"
	"}\n";

	animationShader->script =
	"#version 330\n"

	"layout(location = 0) in vec3 Position;\n"
	"layout(location = 1) in vec2 TexCoord;\n"
	"layout(location = 2) in vec3 Normal;\n"
	"layout(location = 3) in ivec4 BoneIDs;\n"
	"layout(location = 4) in vec4 Weights;\n"

	"out vec2 TexCoord0;\n"
	"out vec3 Normal0;\n"
	"out vec3 WorldPos0;\n"

	"const int MAX_BONES = 100;\n"

	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"
	"uniform mat4 gBones[MAX_BONES];\n"

	"void main()\n"
	"{\n"
	"	mat4 BoneTransform = gBones[BoneIDs[0]] * Weights[0];\n"
	"	BoneTransform += gBones[BoneIDs[1]] * Weights[1];\n"
	"	BoneTransform += gBones[BoneIDs[2]] * Weights[2];\n"
	"	BoneTransform += gBones[BoneIDs[3]] * Weights[3];\n"

	"	vec4 PosL = BoneTransform * vec4(Position, 1.0);\n"
	"	gl_Position = projection * PosL;\n"
	"	TexCoord0 = TexCoord;\n"
	"	vec4 NormalL = BoneTransform * vec4(Normal, 0.0);\n"
	"	Normal0 = (view * NormalL).xyz;\n"
	"	WorldPos0 = (view * PosL).xyz;\n"
	"}\n";
}

void ModuleShaders::CreateDefFragmentShader()
{
	defFragmentShader->script =
	"#version 330\n"

	"in vec3 FragPos;\n"
	"in vec4 ourColor;\n"
	"in vec2 TexCoord;\n"
	"in vec3 ret_normal;\n"

	"out vec4 color;\n"

	"uniform sampler2D ourTexture;\n"
	"uniform int test;\n"
	"uniform vec3 lightPos;\n"
	"uniform vec3 lightColor;\n"

	"void main()\n"
	"{\n"
		"if(test==1)\n"
		"{\n"
			"color=texture(ourTexture,TexCoord);\n"
		"}\n"

		"else \n"
		"{\n"
			//ambient
			"float ambientStrength=1.0;\n"
			"vec3 ambient= ambientStrength*lightColor;\n"

			//diffuse
			"vec3 norm = normalize(ret_normal);\n"
			"vec3 lightDir = normalize(lightPos - FragPos);\n"
			"float diff = max(dot(norm, lightDir), 0.0);\n"
			"vec3 diffuse = diff * lightColor;\n"

			"vec3 result = (ambient + diffuse) * vec3(ourColor.x,ourColor.y,ourColor.z);\n"
			"color=vec4(result, 1.0);\n"
		"}\n"
	"}\n";
}

void ModuleShaders::CompileShader(Shader* shader)
{
	if (shader->type == VERTEX)
	{
		shader->shaderId = glCreateShader(GL_VERTEX_SHADER);
	}
	else
	{
		shader->shaderId = glCreateShader(GL_FRAGMENT_SHADER);
	}
	
	glShaderSource(shader->shaderId, 1, &shader->script, NULL);
	glCompileShader(shader->shaderId);

	int success;
	char Error[512];
	glGetShaderiv(shader->shaderId, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shader->shaderId, 512, NULL, Error);
		app_log->AddLog(Error);
	}
	else
	{
		if(shader->type == VERTEX)
			app_log->AddLog("Vertex Shader compiled successfully!");
		else
			app_log->AddLog("Fragment Shader compiled successfully!");
	}
}

void ModuleShaders::CompileProgram(ShaderProgram* program)
{
	program->programID = glCreateProgram();

	for (int i = 0; i < program->shaders.size(); i++)
	{
		glAttachShader(program->programID, program->shaders[i]);
	}

	glLinkProgram(program->programID);

	int success;
	char Error[512];
	glGetProgramiv(program->programID, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(program->programID, 512, NULL, Error);
		app_log->AddLog(Error);
	}
	else
	{
		app_log->AddLog("Program linked and compiled Successfully!");
	}

}

ShaderProgram * ModuleShaders::GetDefaultShaderProgram() const
{
	return defShaderProgram;
}

ShaderProgram * ModuleShaders::GetAnimationShaderProgram() const
{
	return animationShaderProgram;
}

