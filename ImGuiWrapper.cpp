#include "stdafx.h"
#include "ImGuiWrapper.h"

#include "imgui.h"
#include "PointLight.h"

using namespace DirectX;
using namespace std;

void ImGuiWrapper::TransformPanel(DrawableGameObject& object, int& index, int objCount)
{
	ImGui::Begin("Transform");

	SwitchIndex(index, objCount);

	DrawVec3Control("Position", object.m_position, 75.0f, 60.0f, 0);
	DrawVec3Control("Rotation", object.m_eulerRotation, 75.0f, 60.0f, 0);
	DrawVec3Control("Scale", object.m_scale, 75.0f, 60.0f, 1);

	ImGui::End();
}

void ImGuiWrapper::DrawVec3Control(string displayString, XMFLOAT3& vector, int barWidth, int columnWidth, int resetTo)
{
	static int index = 0;

	{
		std::string id = "##Table" + index;

		bool tableCreated = ImGui::BeginTable(id.c_str(), 2, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_SizingFixedFit);

		// Program can crash if table is not verified.
		if (!tableCreated) return;

		ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60);
		ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_None);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		ImGui::Text(displayString.c_str());

		ImGui::TableSetColumnIndex(1);

		ImGui::PushItemWidth(barWidth);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });

		float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		id = "X##" + std::to_string(index);

		ImGui::SameLine(20.0f);
		if (ImGui::Button(id.c_str()))
		{
			vector.x = resetTo;
		}

		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		id = "##X" + std::to_string(index);
		ImGui::DragFloat(id.c_str(), &vector.x, 0.2f, 0.0f, 0.0f, "%.3f");
		ImGui::PopItemWidth();
		ImGui::SameLine(120.0f);

		ImGui::PushItemWidth(barWidth);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

		id = "Y##" + std::to_string(index);
		if (ImGui::Button(id.c_str()))
		{
			vector.y = resetTo;
		}

		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		id = "##Y" + std::to_string(index);
		ImGui::DragFloat(id.c_str(), &vector.y, 0.2f, 0.0f, 0.0f, "%.3f");
		ImGui::PopItemWidth();
		ImGui::SameLine(220.0f);


		ImGui::PushItemWidth(barWidth);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

		id = "Z##" + std::to_string(index);
		if (ImGui::Button(id.c_str()))
		{
			vector.z = resetTo;
		}

		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		id = "##Z" + std::to_string(index);
		ImGui::DragFloat(id.c_str(), &vector.z, 0.2f, 0.0f, 0.0f, "%.3f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::EndTable();
	}

}

void ImGuiWrapper::SwitchIndex(int& index, int objCount)
{
	float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	if (ImGui::Button("<", buttonSize))
	{
		index--;

		if (index < 0)
			index = objCount - 1;
	}

	ImGui::SameLine();

	if (ImGui::Button(">", buttonSize))
	{
		index++;
		index = index % objCount;
	}
}

void ImGuiWrapper::LightPanel(PointLight& point, int& lightIndex, int lightCount)
{
	constexpr int resetToZero = 0;

	constexpr float minSpecPower = 1.0f;
	constexpr float maxSpecPower = 64.0f;

	constexpr float minAttenuation = 1.0f;
	constexpr float maxAttenuation = 32.0f;

	ImGui::Begin("Edit Light");

	SwitchIndex(lightIndex, lightCount);

	ImGui::ColorEdit4("Ambient Colour", &point.m_ambientColour.x);

	ImGui::NewLine();

	ImGui::ColorEdit4("Diffuse Colour", &point.m_diffuseColour.x);

	ImGui::NewLine();

	ImGui::ColorEdit4("Specular Colour", &point.m_specularColour.x);
	ImGui::SliderFloat("Specular Power", &point.m_shininess, minSpecPower, maxSpecPower, "%.1f");

	ImGui::NewLine();

	XMFLOAT3 lightDir3 = XMFLOAT3(point.m_position.x, point.m_position.y, point.m_position.z);
	DrawVec3Control("Direction", lightDir3, 75.0f, 60.0f,resetToZero);
	point.m_position.x = lightDir3.x; point.m_position.y = lightDir3.y; point.m_position.z = lightDir3.z;

	ImGui::SliderFloat("Attenuation", &point.m_attenuationRadius, minAttenuation, maxAttenuation, "%.1f");

	ImGui::End();
}

void ImGuiWrapper::GPUDebugPanel(int* shadowSampleCount, float* matAlbedo, 
	float* matRoughness, float* matMetalness)
{
	constexpr int minShadowSample = 1;
	constexpr int maxShadowSample = 200; // I stalled the GPU like 6 times. Don't change this future Heather.


	ImGui::Begin("GPU Debug");

	ImGui::SliderInt("Shadow Sample Count", shadowSampleCount, minShadowSample, maxShadowSample);
	ImGui::SliderFloat("Albedo", matAlbedo, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness", matRoughness, 0.0f, 1.0f);
	ImGui::SliderFloat("Metalness", matMetalness, 0.0f, 1.0f);


	ImGui::End();
}
