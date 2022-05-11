#include "AutoCamera.h"
#include "Window.h"
#include <algorithm>
void AutoCamera::UpdateAutoCamera(float dt) {
	sceneTime += dt;
	pitch -= 1;
	yaw -= 1;

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	Matrix4 rotation = Matrix4::Rotation(pitch * 2 , Vector3(1, 0, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	float speed = 30.0f;

	position = startPosition + Vector3(0, 30 * sin(sceneTime), 0);

	
}

Matrix4 AutoCamera::BuildViewMatrix() {
	return	//Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) *
		Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) *
		Matrix4::Translation(-position);
}
