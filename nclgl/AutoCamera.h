#pragma once
#include "Matrix4.h"
#include "Vector3.h"

class AutoCamera {
public:
	AutoCamera(void) {
		yaw = 0.0f;
		pitch = 0.0f;
	};

	AutoCamera(float pitch, float yaw, Vector3 position) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->position = position;
		this->startPosition = position;
	}

	~AutoCamera(void) {};

	void UpdateAutoCamera(float dt = 1.0f);

	Matrix4 BuildViewMatrix();

	Vector3		GetPosition() const { return position; }
	void		SetPosition(Vector3 val) { position = val; }

	float		GetYaw() const { return yaw; }
	void		SetYaw(float y) { yaw = y; }

	float		GetPitch() const { return pitch; }
	void		SetPitch(float p) { pitch = p; }

protected:
	float yaw;
	float pitch;
	Vector3 position;
	Vector3 startPosition;
	float sceneTime;
};

