#pragma once
#include <DirectXMath.h>
#include "BaseLight.h"

//#include "common.h"

class PointLight : public BaseLight
{
public:
	PointLight(DirectX::XMFLOAT4 pos,
		DirectX::XMFLOAT4 ambientCol, DirectX::XMFLOAT4 diffuseCol, DirectX::XMFLOAT4 specularCol,
		float shininess, float attenuationRadius) : 
		BaseLight(pos, ambientCol, diffuseCol, specularCol, shininess, attenuationRadius)
	{
		m_type = LightTypes::Point;
	}
};