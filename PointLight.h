#pragma once
#include <DirectXMath.h>

class PointLight
{
public:
	PointLight(DirectX::XMFLOAT4 pos,
		DirectX::XMFLOAT4 ambientCol, DirectX::XMFLOAT4 diffuseCol, DirectX::XMFLOAT4 specularCol,
		float shininess, float attenuationRadius) :
		m_position(pos),
		m_ambientColour(ambientCol), m_diffuseColour(diffuseCol), m_specularColour(specularCol),
		m_shininess(shininess), m_attenuationRadius(attenuationRadius)
	{
	}

	DirectX::XMFLOAT4 m_position;

	DirectX::XMFLOAT4 m_ambientColour;
	DirectX::XMFLOAT4 m_diffuseColour;
	DirectX::XMFLOAT4 m_specularColour;
	float m_shininess = 28;
		 
	float m_attenuationRadius;
};

