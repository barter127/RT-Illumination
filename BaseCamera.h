#ifndef BASE_CAMERA_H
#define BASE_CAMERA_H

#include <DirectXMath.h>
#include <string>

class BaseCamera
{
public:
	BaseCamera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up,
		float windowWidth = 1280, float windowHeight = 768, float near = 0.01f, float far = 100.0f);
	~BaseCamera();

	void virtual Update(float deltaTime);

	inline std::string GetType() { return m_type; }

	inline virtual DirectX::XMFLOAT3 GetAt() { return m_at; }
	inline void SetAt(DirectX::XMFLOAT3 at) { m_at = at; }

	inline DirectX::XMFLOAT3 GetEye() { return m_eye; }
	inline DirectX::XMFLOAT3 GetUp() { return m_up; }
 
	const DirectX::XMMATRIX GetView() { return XMLoadFloat4x4(&m_view); }
	const DirectX::XMMATRIX GetProj() { return XMLoadFloat4x4(&m_projection); }

protected:
	std::string m_type = "Base";

	DirectX::XMFLOAT3 m_eye;
	DirectX::XMFLOAT3 m_at;
	DirectX::XMFLOAT3 m_up;

	float m_windowWidth;
	float m_windowHeight;
	float m_nearPlane;
	float m_farPlane;

	float m_aspect;

	DirectX::XMFLOAT4X4 m_view;
	DirectX::XMFLOAT4X4 m_projection;
};

#endif // !BASE_CAMERA_H