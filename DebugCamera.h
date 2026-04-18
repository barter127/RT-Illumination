#ifndef DEBUG_CAMERA_H
#define DEBUG_CAMERA_H

#include "BaseCamera.h"

class DebugCamera : public BaseCamera
{
public:
	DebugCamera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up,
		float windowWidth = 1280, float windowHeight = 768, float near = 0.01f, float far = 100.0f);
	~DebugCamera();

	void Initialise(float yaw, float pitch, float sensitivity, float speed);

	void Update(float deltaTime) override;

	inline float GetYaw() { return m_yaw; }
	inline float GetPitch() { return m_pitch; }

public:
	float m_sensitivity = 0.3f;
	float m_speed = 5.0f;

private:
	void Strafe(DirectX::XMFLOAT3 movementDir, float deltaTime);

public:
	// This could probably be moved to base Cam Class.
	float m_yaw = 0.0f;
	float m_pitch = 0.0f;

	bool m_firstMouse = true;

	DirectX::XMFLOAT2 m_lastMousePos;

	DirectX::XMFLOAT3 currentForward = { 0, 0, 1 };
	DirectX::XMFLOAT3 currentBackward = { 0, 0, -1 };
	DirectX::XMFLOAT3 currentLeft = { -1, 0, 0 };
	DirectX::XMFLOAT3 currentRight = { 1, 0, 0 };

	// Up and down currently do not change.
	DirectX::XMFLOAT3 currentDown = { 0, -1, 0 };
	DirectX::XMFLOAT3 currentUp = { 0, 1, 0 };
};

#endif // !DEBUG_CAMERA_H