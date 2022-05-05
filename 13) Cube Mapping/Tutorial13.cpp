#include "../nclgl/window.h"
#include "Renderer.h"

int main() {
	Window w("Cube Mapping", 1280, 720,false);

	if(!w.HasInitialised()) {
		return -1;
	}
	
	Renderer renderer(w);
	if(!renderer.HasInitialised()) {
		return -1;
	}

    w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	float currentFrameTime = 0;
	float rotation = 0;
	float reflectionPower = 100.0f;

	while(w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_PLUS) && reflectionPower != 100)  reflectionPower = reflectionPower + 0.25f;
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_MINUS) && reflectionPower != 0) reflectionPower = reflectionPower - 0.25f;
		renderer.SetReflection(reflectionPower);

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_LEFT))  ++rotation;
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_RIGHT)) --rotation;
		renderer.SetRotation(rotation);

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1)) {
			renderer.ToggleObject();
		}

		currentFrameTime = w.GetTimer()->GetTimeDeltaSeconds();

		renderer.UpdateScene(currentFrameTime);
		renderer.RenderScene();
		renderer.SwapBuffers();

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}

	return 0;
}