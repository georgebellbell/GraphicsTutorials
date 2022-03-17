#include "Renderer.h"

#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"

#define SHADOWSIZE 2048

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	

	if (!sceneShader->LoadSuccess()) {
		return;
	}



	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));
	sceneMeshes.emplace_back(Mesh::GenerateQuad());

	sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"brickDOT3.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	mirrorTex = SOIL_load_OGL_texture(
		TEXTUREDIR"stainedglass.tga",
		SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);

	SetTextureRepeating(sceneDiffuse, true);
	SetTextureRepeating(sceneBump, true);
	SetTextureRepeating(mirrorTex, true);

	glEnable(GL_DEPTH_TEST);

	sceneTransforms.resize(5);
	sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Translation(Vector3(-5, 0, 0)) * Matrix4::Scale(Vector3(5, 5, 1));
	sceneTransforms[4] = Matrix4::Translation(Vector3(0, -5, 0)) * Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	sceneTime = 0.0f;
	init = true;


}

Renderer::~Renderer(void) {
	

	for (auto& i : sceneMeshes) {
		delete i;
	}

	delete camera;
	delete sceneShader;
	

}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	sceneTime += dt;

	for (int i = 1; i < 4; i++)
	{
		Vector3 t = Vector3(-10 + (5 * i), 3.5f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(2, 2, 2));

	}
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//DrawShadowScene();
	DrawMainScene();
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

	for (int i = 0; i < 5; i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawMainScene() {
	BindShader(sceneShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	//glScalef(1.0f, -1.0f, 1.0f);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneDiffuse);

	
	/*
	for (int i = 0; i < 5; i++)
	{
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
	*/
	
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	modelMatrix = sceneTransforms[1];
	UpdateShaderMatrices();
	sceneMeshes[1]->Draw();
	
	
	glEnable(GL_STENCIL_TEST);
	glColorMask(0, 0, 0, 0);
	glDisable(GL_DEPTH_TEST);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	modelMatrix = sceneTransforms[0];
	UpdateShaderMatrices();

	sceneMeshes[0]->Draw();

	glColorMask(1, 1, 1, 1);
	glEnable(GL_DEPTH_TEST);
	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	modelMatrix = Matrix4::Rotation(180, Vector3(1, 0, 0)) * sceneTransforms[1];
	UpdateShaderMatrices();
	sceneMeshes[1]->Draw();
	glPopMatrix();

	
	glDisable(GL_STENCIL_TEST);
	
	glEnable(GL_BLEND);

	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mirrorTex);

	
	glColor4f(1, 1, 1, 0.7f);
	modelMatrix = sceneTransforms[0];
	UpdateShaderMatrices();
	sceneMeshes[0]->Draw();

	glDisable(GL_BLEND);
	
}
