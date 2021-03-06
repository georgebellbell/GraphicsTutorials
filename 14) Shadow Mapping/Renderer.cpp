#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"

#define SHADOWSIZE 2048

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

	quad = Mesh::GenerateQuad();
	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));

	sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"brickDOT3.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	mirrorTex = SOIL_load_OGL_texture(
		TEXTUREDIR"stainedglass.tga",
		SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	SetTextureRepeating(sceneDiffuse, true);
	SetTextureRepeating(sceneBump, true);
	SetTextureRepeating(mirrorTex, true);

	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	if (!sceneShader->LoadSuccess() || !skyboxShader->LoadSuccess()) {
		return;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	sceneTransforms.resize(5);
	sceneTime = 0.0f;
	init = true;


}

Renderer::~Renderer(void) {

	for (auto& i : sceneMeshes) {
		delete i;
	}

	delete camera;
	delete sceneShader;
	delete quad;
	delete skyboxShader;

}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	sceneTime += dt;

	for (int i = 1; i < 4; i++)
	{
		Vector3 t = Vector3(0, 3.5f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Scale(Vector3(1.5, 1.5, 1.5));

	}
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	if (expand == true) {
		sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Translation(Vector3(240, 240, 0)) * Matrix4::Scale(Vector3(250, 250, 1));
	}
	else if (expand == false) {

		sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	}

	DrawSkybox();
	DrawMainScene();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();
	quad->Draw();

	glDepthMask(GL_TRUE);
}
void Renderer::DrawMainScene() {


	BindShader(sceneShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneDiffuse);

	Vector4 objectColour = Vector4(1, 0, 0, 1);
	Vector4 mirrorColour = Vector4(0.5, 0.5, 0.5, 0.7);

	glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "colour"), 1, (float*)&objectColour);

	glClear(GL_DEPTH_BUFFER_BIT |  GL_STENCIL_BUFFER_BIT);

	Matrix4 translation;
	if (expand == true) {
		for (int i = 0; i < 99; i++)
		{
			for (int j = 0; j < 99; j++)
			{
				translation = Matrix4::Translation(Vector3(i * 5, 0, j * 5));
				modelMatrix = translation * sceneTransforms[1];
				UpdateShaderMatrices();
				sceneMeshes[1]->Draw();
			}

		}
	}
	else if (expand == false) 
	{
		modelMatrix = sceneTransforms[1];
		UpdateShaderMatrices();
		sceneMeshes[1]->Draw();
	}
	

	
	glEnable(GL_STENCIL_TEST);
	glColorMask(0, 0, 0, 0);
	glDisable(GL_DEPTH_TEST);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	modelMatrix = sceneTransforms[0];
	UpdateShaderMatrices();

	glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "colour"), 1, (float*)&objectColour);

	sceneMeshes[0]->Draw();

	glColorMask(1, 1, 1, 1);
	glEnable(GL_DEPTH_TEST);
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	if (expand == true) 
	{
		for (int i = 0; i < 99; i++)
		{
			for (int j = 0; j < 99; j++)
			{
				translation = Matrix4::Translation(Vector3(i * 5, 0, j * 5));
				modelMatrix = Matrix4::Rotation(180, Vector3(1, 0, 1)) * translation * sceneTransforms[1];
				UpdateShaderMatrices();
				sceneMeshes[1]->Draw();
			}

		}
	}
	else if (expand == false)
	{
		modelMatrix = Matrix4::Rotation(180, Vector3(1, 0, 0)) * sceneTransforms[1];
		UpdateShaderMatrices();
		sceneMeshes[1]->Draw();
	}
	
	glDisable(GL_STENCIL_TEST);
	
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mirrorTex);

	modelMatrix = sceneTransforms[0];
	UpdateShaderMatrices();

	glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "colour"), 1, (float*)&mirrorColour);
	//Bind reflection strength
	glUniform1f(glGetUniformLocation(sceneShader->GetProgram(), "reflection"), reflectionPower);
	sceneMeshes[0]->Draw();

	glDisable(GL_BLEND);
	
}
