#pragma once

#include "BaseLight.h"

class DirectionalLight : public BaseLight
{
public:
	DirectionalLight(DirectX::XMFLOAT4 direction, DirectX::XMFLOAT4 ambientCol, DirectX::XMFLOAT4 diffuseCol,
		DirectX::XMFLOAT4 specularCol, int specularPower) :
		BaseLight(direction, ambientCol, diffuseCol, specularCol, specularPower, attenuation)
	{
		m_type = LightTypes::Directional;
	}

private:
	const int attenuation = 0.0f;
};
