#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"

#define SHADOWSIZE 2048

Shader* loadShader(const std::string& vertex, const std::string& fragment, const std::string& geometry = "", const std::string& domain = "", const std::string& hull = "") {
	Shader* shader = new Shader(vertex, fragment, geometry, domain, hull);
	
	if (!shader->LoadSuccess()) {

		std::cout << "Error when loading shader\n Pressing any key to exit...";
		std::cin.get();

		std::exit(-1);
	}

	return shader;
}

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	light = new Light(Vector3(-20.0f, 10.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

	quad = Mesh::GenerateQuad();
	skyboxQuad = Mesh::GenerateQuad();

	sceneMeshes.emplace_back(Mesh::GenerateQuad());                 
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cube.msh"));	
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh")); 
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cube.msh"));

	
	sceneShader = loadShader("shadowSceneVert.glsl", "shadowSceneFrag.glsl");
	skyboxShader = loadShader("skyboxVertex.glsl", "skyboxFragment.glsl");
	pprocessShader = loadShader("TexturedVertex.glsl", "ssrFragment.glsl");

	sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	planeTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	planeBump = SOIL_load_OGL_texture(TEXTUREDIR"NormalMap.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!sceneDiffuse || !sceneBump || !cubeMap || !planeTex || !planeBump) {
		return;
	}

	SetTextureRepeating(sceneDiffuse, true);
	SetTextureRepeating(sceneBump, true);
	SetTextureRepeating(planeTex, true);
	SetTextureRepeating(planeBump, true);

	CreateGBuffer(width, height);
	CreatePostBuffer(width, height);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glCullFace(GL_BACK);

	sceneTransforms.resize(4);
	
	sceneTransforms[1] = Matrix4::Translation(Vector3(2, 0.5, 0));
	sceneTransforms[2] = Matrix4::Translation(Vector3(5, 0.5, 0)) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	sceneTransforms[3] = Matrix4::Translation(Vector3(0, 0.5, 0)) * Matrix4::Translation(Vector3(0, 1, 0));;

	mirrorTransforms.resize(1);
	mirrorTransforms[0] = Matrix4::Translation(Vector3(0, 2, 0)) * Matrix4::Scale(Vector3(3, 3, 1));

	sceneTime = 0.0f;

	init = true;
}

Renderer::~Renderer(void) {
	
	for (auto& i : sceneMeshes) {
		delete i;
	}

	delete skyboxQuad;
	delete camera;
	delete sceneShader;
	delete skyboxShader;
	delete pprocessShader;
	delete quad;

	glDeleteFramebuffers(1, &postProcessingFBO);
	glDeleteTextures(1, &postProcessingTexture);

	glDeleteFramebuffers(1, &gBufferFBO);
	glDeleteTextures(1, &gBufferColourTex);
	glDeleteTextures(1, &gBufferPositionTex);
	glDeleteTextures(1, &gBufferNormalTex);
	glDeleteTextures(1, &gBufferDepthTex);
}

void Renderer::CreateGBuffer(int width, int heigth) {
	glGenFramebuffers(1, &gBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

	//Create colour texture
	{
		glGenTextures(1, &gBufferColourTex);
		glBindTexture(GL_TEXTURE_2D, gBufferColourTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBufferColourTex, 0);

		glObjectLabel(GL_TEXTURE, gBufferColourTex, -1, "G-Buffer Colour");
	}

	//Create depth texture
	{
		glGenTextures(1, &gBufferDepthTex);
		glBindTexture(GL_TEXTURE_2D, gBufferDepthTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBufferDepthTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gBufferDepthTex, 0);

		glObjectLabel(GL_TEXTURE, gBufferDepthTex, -1, "G-Buffer Depth");
	}


	//Create position texture
	{
		glGenTextures(1, &gBufferPositionTex);
		glBindTexture(GL_TEXTURE_2D, gBufferPositionTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBufferPositionTex, 0);

		glObjectLabel(GL_TEXTURE, gBufferPositionTex, -1, "G-Buffer Position");
	}


	//Create normal texture
	{
		glGenTextures(1, &gBufferNormalTex);
		glBindTexture(GL_TEXTURE_2D, gBufferNormalTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBufferNormalTex, 0);

		glObjectLabel(GL_TEXTURE, gBufferNormalTex, -1, "G-Buffer Normal");

	}


	//Tell OpenGL which buffers to use for rendering
	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//Check framebuffer status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		//Should probably print an error here 
		return;
	}

	//Unbind for sanity reasons
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CreatePostBuffer(int width, int heigth) {
	glGenFramebuffers(1, &postProcessingFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);

	glGenTextures(1, &postProcessingTexture);
	glBindTexture(GL_TEXTURE_2D, postProcessingTexture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingTexture, 0);

	glObjectLabel(GL_TEXTURE, postProcessingTexture, -1, "Post-Processing Colour");

	//Check framebuffer status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		//Should probably print an error here 
		return;
	}

	//Unbind for sanity reasons
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	sceneTime += dt;

	//move shapes
	Vector3 t = Vector3(-5, 0.5f, 10.0f + sin(sceneTime * 2));
	Vector3 s = Vector3(5, 0.5f, -7.0f + sin(sceneTime));
	sceneTransforms[1] = Matrix4::Translation(t);
	sceneTransforms[2] = Matrix4::Translation(s) * Matrix4::Rotation(sceneTime * 30, Vector3(0, 1, 0)) * Matrix4::Rotation(90, Vector3(1, 0, 0));
}

void Renderer::RenderScene() {
	DrawScene();
	DrawPostProcess();
	PresentScene();
}

void Renderer::DrawScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	if (expand == true) {
		sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Translation(Vector3(240, 240, 0)) * Matrix4::Scale(Vector3(250, 250, 1));
	}
	else if (expand == false) {

		sceneTransforms[0] = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));
	}
	DrawSkybox();
	DrawMainScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();
	skyboxQuad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawMainScene() {

	BindShader(sceneShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1f(glGetUniformLocation(sceneShader->GetProgram(), "reflection"), 100);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, planeTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, planeBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	

	modelMatrix = sceneTransforms[0];
	UpdateShaderMatrices();
	sceneMeshes[0]->Draw();

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneDiffuse);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sceneBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	if (expand == true) 
	{
		Matrix4 translation;

		for (int i = 0; i < 99; i++)
		{
			for (int j = 0; j < 99; j++)
			{
				translation = Matrix4::Translation(Vector3(i * 5, 0, j * 5));
				modelMatrix = sceneTransforms[1] * translation;
				UpdateShaderMatrices();
				sceneMeshes[1]->Draw();
			}

		}
	}
	else if (expand == false)
	{
			for (int i = 1; i < sceneMeshes.size(); i++)
			{
				modelMatrix = sceneTransforms[i];
				UpdateShaderMatrices();
				sceneMeshes[i]->Draw();
			}
	}

	modelMatrix = Matrix4::Translation(Vector3(0, -20, 0)) * Matrix4::Scale(Vector3(0.05f, 0.05f, 0.05f));
	UpdateShaderMatrices();
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);

	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(pprocessShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	//Bind colour buffer to slot 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBufferColourTex);
	glUniform1i(glGetUniformLocation(pprocessShader->GetProgram(), "colourTex"), 0);

	//Bind position buffer to slot 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBufferPositionTex);
	glUniform1i(glGetUniformLocation(pprocessShader->GetProgram(), "positionTex"), 1);

	//Bind normal buffer to slot 2
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBufferNormalTex);
	glUniform1i(glGetUniformLocation(pprocessShader->GetProgram(), "normalTex"), 2);
	
	//Bind depth buffer to slot 3
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gBufferDepthTex);
	glUniform1i(glGetUniformLocation(pprocessShader->GetProgram(), "depthTex"), 3);

	//Bind cube map to slot 4
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
	glUniform1i(glGetUniformLocation(pprocessShader->GetProgram(), "cubeTex"), 4);

	//Bind camera position to shader
	glUniform3fv(glGetUniformLocation(pprocessShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	//Bind pixel size to shader
	glUniform2f(glGetUniformLocation(pprocessShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	//Bind inverse View Project Matrix
	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pprocessShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	//Bind reflection strength
	glUniform1f(glGetUniformLocation(pprocessShader->GetProgram(), "reflection"), reflection); 

	//Bind a new colour attachment to the framebuffer and draw
	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(pprocessShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, postProcessingTexture);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();
}