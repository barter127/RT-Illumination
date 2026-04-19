#pragma once

#include <dxcapi.h>
#include <string>

#include "DrawableGameObject.h"

class ImGuiWrapper
{
public:
	void TransformPanel(DrawableGameObject& object);

	void DrawVec3Control(std::string displayString, DirectX::XMFLOAT3& vector, int barWidth, int columnWidth, int resetTo);

	void LightPanel(float* ambientCol, float* diffuseCol,
		float* specularCol, float* specularPower,
		DirectX::XMFLOAT4& lightDir);
};

