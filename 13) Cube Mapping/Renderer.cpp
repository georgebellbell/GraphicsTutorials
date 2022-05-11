#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"

Renderer::Renderer(Window &parent) : OGLRenderer(parent) {
	camera = new Camera(-45.0f, 0.0f, Vector3(-32.0f, 75.0f, 8.0f));

	quad = Mesh::GenerateQuad();
	reflector = Mesh::LoadFromMeshFile("Sphere.msh");

	// normal texture loading technique
	normalTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	// texture loading technique for a cube map
	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if (!earthTex || !earthBump || !cubeMap || !normalTex) {return;}
	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	
	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() ) {
		return;
	}

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

Renderer::~Renderer(void) {
	delete camera;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete reflector;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	DrawReflection();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawReflection() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	glUniform1f(glGetUniformLocation(reflectShader->GetProgram(), "reflectionPower"), reflectionPower);

	if (expand == true)
	{
		Matrix4 translation;

		for (int i = 0; i < 99; i++)
		{
			for (int j = 0; j < 99; j++)
			{
				translation = Matrix4::Translation(Vector3(i * 5, 0, j * 5));
				modelMatrix = Matrix4::Rotation(rotation, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(25.0f, 25.0f, 25.0f)) * translation;
				UpdateShaderMatrices();
				reflector->Draw();
			}

		}
	}
	else if (expand == false)
	{
		modelMatrix = Matrix4::Rotation(rotation, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(25.0f, 25.0f, 25.0f));
		UpdateShaderMatrices();
		reflector->Draw();
	}
	
	
}

void Renderer::ToggleObject() {
	objectSelected = (objectSelected + 1) % 5;
	switch (objectSelected) {
	case(0):	reflector = Mesh::LoadFromMeshFile("Sphere.msh"); break;
	case(1):	reflector = Mesh::LoadFromMeshFile("Cube.msh"); break;
	case(2):	reflector = Mesh::GenerateQuad(); break;
	case(3):	reflector = Mesh::LoadFromMeshFile("Role_T.msh"); break;
	case(4):	reflector = Mesh::LoadFromMeshFile("Cylinder.msh"); break;
	}
}