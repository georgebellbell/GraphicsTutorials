#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"


#define SHADOWSIZE 2048
const int POST_PASSES = 10;

Renderer::Renderer(Window &parent) : OGLRenderer(parent)	{
	camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

	heightMap = new HeightMap(TEXTUREDIR"noise.png");
	skyboxQuad = Mesh::GenerateQuad();
	mirrorQuad = Mesh::GenerateQuad();
	sceneMeshes.emplace_back(Mesh::GenerateQuad());                 // 0 floor
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cube.msh"));	// 1 moving cube
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh")); // 2 - spinning cylinder
	

	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	sceneShadowShader = new Shader("shadowSceneVert.glsl", "shadowSceneFrag.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");


	if (!sceneShadowShader->LoadSuccess() || !shadowShader->LoadSuccess() || !reflectShader->LoadSuccess() ||
		!skyboxShader->LoadSuccess()) {
		return;
	}

	sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!sceneDiffuse || !sceneBump || !cubeMap) {
		return;
	}

	SetTextureRepeating(sceneDiffuse, true);
	SetTextureRepeating(sceneBump, true);



	// SHADOW
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glCullFace(GL_BACK);

	sceneTransforms.resize(4);
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	sceneTransforms[2] = Matrix4::Translation(Vector3(5, 2, 0)) *  Matrix4::Rotation(90, Vector3(1, 0, 0));
	
	mirrorTransforms.resize(1);
	mirrorTransforms[0] = Matrix4::Translation(Vector3(0, 2, 0)) * Matrix4::Scale(Vector3(3, 3, 1));

	sceneTime = 0.0f;

	init = true;

}
Renderer::~Renderer(void)	{
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);

	for (auto& i : sceneMeshes) {
		delete i;
	}

	delete mirrorQuad;
	delete skyboxQuad;
	delete camera;
	delete sceneShadowShader;
	delete shadowShader;
	delete skyboxShader;
	delete reflectShader;
	delete heightMap;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	sceneTime += dt;

	Vector3 t = Vector3(-5, 0.5f, 7.0f + sin(sceneTime));
	Vector3 s = Vector3(5, 0.5f, -7.0f + sin(sceneTime));
	sceneTransforms[1] = Matrix4::Translation(t);
	sceneTransforms[2] = Matrix4::Translation(s) * Matrix4::Rotation(sceneTime * 30 , Vector3(0, 1, 0)) * Matrix4::Rotation(90, Vector3(1, 0, 0));
}

void Renderer::RenderScene()	{
	/*DrawScene();
	DrawPostProcess();
	PresentScene();*/
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawSkybox();
	DrawShadowScene();
	DrawMainScene();
	DrawMirrors();
	
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();
	skyboxQuad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1, 100, 1, 45);
	shadowMatrix = projMatrix * viewMatrix;

	for (int i = 0; i < sceneMeshes.size(); i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
	
	heightMap->Draw();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawMainScene() {
	BindShader(sceneShadowShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(sceneShadowShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShadowShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShadowShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(sceneShadowShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneDiffuse);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sceneBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (int i = 0; i < sceneMeshes.size(); i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	modelMatrix = Matrix4::Translation(Vector3(0, -20, 0)) * Matrix4::Scale(Vector3(0.05f, 0.05f, 0.05f));
	UpdateShaderMatrices();

	heightMap->Draw();


}

void Renderer::DrawMirrors() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	//glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	
	modelMatrix = mirrorTransforms[0] * Matrix4::Rotation(rotation, Vector3(0, 1, 0));
	UpdateShaderMatrices();
	mirrorQuad->Draw();
	
}
