#include "BaseCamera.h"

using namespace DirectX;

BaseCamera::BaseCamera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up,
float windowWidth, float windowHeight, float near, float far) :
	m_eye(position),
	m_at(at),
	m_up(up),
	m_windowWidth(windowWidth),
	m_windowHeight(windowHeight),
	m_nearPlane(near),
	m_farPlane(far)
{
	m_aspect = m_windowWidth / m_windowHeight;

	// Create View Mat.
	XMStoreFloat4x4(&m_view, XMMatrixLookAtLH(XMLoadFloat3(&m_eye), XMLoadFloat3(&m_at), XMLoadFloat3(&m_up)));

	// Create Projection Mat.
	XMMATRIX perspective = XMMatrixPerspectiveFovLH(XMConvertToRadians(90), m_aspect, m_nearPlane, m_farPlane);
	XMStoreFloat4x4(&m_projection, perspective);

	// Maybe Orthographic mode?
}

BaseCamera::~BaseCamera()
{
}

void BaseCamera::Update(float deltaTime)
{

}