#pragma once
#include "../NCLGL/OGLRenderer.h"

class Camera;
class Mesh;
class HeightMap;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene()				override;
	void UpdateScene(float msec)	override;

	inline void SetReflection(float r) { reflection = r; }

	inline void ToggleTechnique() { expand = !expand; }

private:
	void CreateGBuffer(int width, int heigth);
	void CreatePostBuffer(int width, int heigth);

protected:

	void PresentScene();
	void DrawPostProcess();
	void DrawScene();

	void DrawMainScene();
	void DrawSkybox();

	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint sceneDiffuse;
	GLuint sceneBump;
	float sceneTime;

	GLuint cubeMap;
	GLuint planeTex;
	GLuint planeBump;

	//Framebuffer and textures used for post processing
	GLuint postProcessingFBO;
	GLuint postProcessingTexture;

	//Framebuffer for rendering the scene
	GLuint gBufferFBO;
	GLuint gBufferColourTex;
	GLuint gBufferPositionTex;
	GLuint gBufferNormalTex;
	GLuint gBufferDepthTex;

	Shader* sceneShader;
	Shader* shadowShader;
	Shader* skyboxShader;
	Shader* reflectShader;
	Shader* pprocessShader;

	Mesh* quad;
	HeightMap* heightMap;
	Mesh* skyboxQuad;
	Mesh* mirrorQuad;
	vector<Mesh*> sceneMeshes;
	vector<Matrix4> sceneTransforms;

	vector<Matrix4> mirrorTransforms;

	Camera* camera;
	Light* light;

	float reflection;
	bool expand = false;
};

