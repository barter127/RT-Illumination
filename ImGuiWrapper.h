#pragma once

#include <dxcapi.h>
#include <string>

#include "DrawableGameObject.h"

class ImGuiWrapper
{
public:
	void TransformPanel(DrawableGameObject& object);

	void DrawVec3Control(std::string displayString, DirectX::XMFLOAT3& vector, int barWidth, int columnWidth, int resetTo);
};

