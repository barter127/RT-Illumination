#include "DebugCamera.h"
#include "Input.h"
#include <algorithm>

using namespace DirectX;

DebugCamera::DebugCamera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up,
	float windowWidth, float windowHeight, float nearPlane, float farPlane) : 

	BaseCamera(position, at, up, windowWidth, windowHeight, nearPlane, farPlane)
{
	m_type = "Debug";
}

DebugCamera::~DebugCamera() {}

void DebugCamera::Initialise(float yaw, float pitch, float sensitivity, float speed)
{
	m_yaw = yaw;
	m_pitch = pitch;
	m_sensitivity = sensitivity;
	m_speed = speed;

	constexpr XMFLOAT3 DEFAULT_FORWARD = { 0, 0, 1 };

	XMVECTOR eyeVector = XMLoadFloat3(&m_eye);
	XMVECTOR atVector = XMLoadFloat3(&m_at);
	XMVECTOR upVector = XMLoadFloat3(&m_up);

	XMVECTOR forwardVector = XMLoadFloat3(&DEFAULT_FORWARD);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), 0.0f);
	atVector = XMVector3TransformNormal(forwardVector, rotation);
	atVector = XMVectorAdd(atVector, eyeVector);

	XMMATRIX view = XMMatrixLookAtLH(eyeVector, atVector, upVector);

	forwardVector = XMVector3Normalize(forwardVector);
	forwardVector = XMVector3TransformNormal(forwardVector, rotation);
	XMStoreFloat3(&currentForward, forwardVector);
	XMStoreFloat3(&currentBackward, -forwardVector);

	XMVECTOR leftVector = XMVector3Cross(forwardVector, upVector);
	XMStoreFloat3(&currentRight, -leftVector);
	XMStoreFloat3(&currentLeft, leftVector);

	XMStoreFloat4x4(&m_view, view);
}

void DebugCamera::Update(float deltaTime)
{
	constexpr XMFLOAT3 DEFAULT_FORWARD = { 0, 0, 1 };
	constexpr XMFLOAT3 DEFAULT_UP = { 0, 1, 0 };

	bool dirtyFlag = false;

	if (Input::GetKeyHeld('W'))
	{
		Strafe(currentBackward, deltaTime);
		dirtyFlag = true;
	}
	if (Input::GetKeyHeld('S'))
	{
		Strafe(currentForward, deltaTime);
		
		dirtyFlag = true;
	}
	if (Input::GetKeyHeld('A'))
	{
		Strafe(currentLeft, deltaTime);
		dirtyFlag = true;
	}
	if (Input::GetKeyHeld('D'))
	{
		Strafe(currentRight, deltaTime);
		dirtyFlag = true;
	}
	if (Input::GetKeyHeld('E'))
	{
		Strafe(currentUp, deltaTime);
		dirtyFlag = true;
	}
	if (Input::GetKeyHeld('Q'))
	{
		Strafe(currentDown, deltaTime);
		dirtyFlag = true;
	}

	if (Input::GetMouseRButtonUp())
	{
		m_firstMouse = true;
	}

	if (Input::GetMouseRButtonHeld())
	{
		dirtyFlag = true;

		XMFLOAT2 mousePos = Input::GetMousePosition();

		if (m_firstMouse)
		{
			m_lastMousePos = mousePos;

			m_firstMouse = false;
		}

		float xOffset = m_lastMousePos.x - mousePos.x;
		float yOffset = m_lastMousePos.y - mousePos.y;

		xOffset *= m_sensitivity;
		yOffset *= m_sensitivity;

		m_yaw += xOffset;
		m_pitch += yOffset;

		m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

		m_lastMousePos = mousePos;
	}

	if (dirtyFlag)
	{
		XMVECTOR eyeVector = XMLoadFloat3(&m_eye);
		XMVECTOR atVector = XMLoadFloat3(&m_at);
		XMVECTOR upVector = XMLoadFloat3(&m_up);

		XMVECTOR forwardVector = XMLoadFloat3(&DEFAULT_FORWARD);

		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), 0.0f);
		atVector = XMVector3TransformNormal(forwardVector, rotation);
		atVector =  XMVectorAdd(atVector, eyeVector);

		XMMATRIX view = XMMatrixLookAtLH(eyeVector, atVector, upVector);

		forwardVector = XMVector3Normalize(forwardVector);
		forwardVector = XMVector3TransformNormal(forwardVector, rotation);
		XMStoreFloat3(&currentForward, forwardVector);
		XMStoreFloat3(&currentBackward, -forwardVector);

		XMVECTOR leftVector = XMVector3Cross(forwardVector, upVector);
		XMStoreFloat3(&currentRight, -leftVector);
		XMStoreFloat3(&currentLeft, leftVector);

		XMStoreFloat4x4(&m_view, view);
	}
}

void DebugCamera::Strafe(XMFLOAT3 direction, float deltaTime)
{
	XMVECTOR eyeVector = XMLoadFloat3(&m_eye) + (XMLoadFloat3(&direction) * m_speed * deltaTime);
	XMStoreFloat3(&m_eye, eyeVector);
}