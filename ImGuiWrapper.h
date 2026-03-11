#pragma once

#include <dxcapi.h>
#include <string>

class ImGuiWrapper
{
public:
	void DrawVec3Control(std::string displayString, DirectX::XMFLOAT3 vector, int barWidth, int columnWidth, int resetTo);


};

