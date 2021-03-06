#include "../nclgl/window.h"
#include "Renderer.h"

int main() {
	Window w("Screen Space Reflections", 1280, 720, false);

	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w);
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	double currentFrameTime;
	double lastTime = w.GetTimer()->GetTotalTimeSeconds();

	float reflectionPower = 50.0f;

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_PLUS) && reflectionPower != 100)  reflectionPower = reflectionPower + 0.25f;
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_MINUS) && reflectionPower != 0) reflectionPower = reflectionPower - 0.25f;

		renderer.SetReflection(reflectionPower);

		currentFrameTime = w.GetTimer()->GetTimeDeltaSeconds();
		float currentTotalTime = w.GetTimer()->GetTotalTimeSeconds();

		if (currentTotalTime < 30.5 && currentTotalTime - lastTime >= 1.0) {
			//total time, current frame time
			printf("%f\n", currentFrameTime);
			lastTime = lastTime + 1.0;
		}

		renderer.UpdateScene(currentFrameTime);
		renderer.RenderScene();
		renderer.SwapBuffers();
		
		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_E)) {
			renderer.ToggleTechnique();
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}

	return 0;
}