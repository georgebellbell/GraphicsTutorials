#pragma once
#include "../nclgl/OGLRenderer.h"
class Camera;
class Shader;
class HeightMap;
class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;
	void ToggleObject();
	inline void SetReflection(float r) { reflectionPower = r; }
	inline void SetRotation(float r) { rotation = r; }
protected:
	void DrawReflection();
	void DrawSkybox();

	Shader* reflectShader;
	Shader* skyboxShader;

	Mesh* quad;
	Mesh* reflector;

	Camera* camera;

	GLuint cubeMap;
	GLuint normalTex;
	GLuint earthTex;
	GLuint earthBump;
	
	float reflectionPower;
	float rotation;
	int objectSelected;
};

