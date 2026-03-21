#include "stdafx.h"
#include "ImGuiWrapper.h"

#include "imgui.h"

using namespace DirectX;
using namespace std;

void ImGuiWrapper::TransformPanel(DrawableGameObject& object)
{
	ImGui::Begin("Transform");

	int index = 0;

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
