#pragma once

#include <dxcapi.h>
#include <string>

#include "DrawableGameObject.h"

class BaseLight;

class ImGuiWrapper
{
public:
	void TransformPanel(DrawableGameObject& object, int& index, int objectCount);

	void DrawVec3Control(std::string displayString, DirectX::XMFLOAT3& vector, int barWidth, int columnWidth, int resetTo);

	void SwitchIndex(int& index, int objCount);

	void LightPanel(BaseLight& point, int& lightIndex, int lightCount);

	void GPUDebugPanel(int* shadowSampleCount, float* matAlbedo, float* matRoughness, float* matMetalness);
};

