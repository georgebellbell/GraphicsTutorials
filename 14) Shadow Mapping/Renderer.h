#pragma once

#include "../nclgl/OGLRenderer.h"

class Camera;
class Mesh;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override; 

	inline void SetReflection(float r) { reflectionPower = r; }

protected:
	void DrawMainScene();

	void DrawSkybox();

	Shader* skyboxShader;

	Mesh* quad;

	GLuint sceneDiffuse;
	GLuint sceneBump;
	float sceneTime;
	GLuint cubeMap;
	GLuint mirrorTex;

	Shader* sceneShader;
	Shader* shadowShader;

	vector<Mesh*> sceneMeshes;
	vector<Matrix4> sceneTransforms;

	Camera* camera;
	Light* light;

	float reflectionPower;
};

