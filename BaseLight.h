#pragma once

#include <DirectXMath.h>

enum LightTypes
{
	Invalid = -1,
	Directional = 0,
	Point
};

class BaseLight
{
protected:
	BaseLight(DirectX::XMFLOAT4 pos,
		DirectX::XMFLOAT4 ambientCol, DirectX::XMFLOAT4 diffuseCol, DirectX::XMFLOAT4 specularCol,
		float shininess, float attenuationRadius) :
		m_position(pos),
		m_ambientColour(ambientCol), m_diffuseColour(diffuseCol), m_specularColour(specularCol),
		m_shininess(shininess), m_attenuationRadius(attenuationRadius)
	{}

	int m_type = LightTypes::Invalid;

public:
	DirectX::XMFLOAT4 m_position;

	DirectX::XMFLOAT4 m_ambientColour;
	DirectX::XMFLOAT4 m_diffuseColour;
	DirectX::XMFLOAT4 m_specularColour;
	float m_shininess = 28;

	float m_attenuationRadius;

	int LightType() { return m_type; }
};
