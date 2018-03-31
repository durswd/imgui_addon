#include "fcurve.h"
#include "imgui_internal.h"

#include <stdint.h>
#include <algorithm>

namespace ImGui
{
	static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
	{
		return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
	}

	static ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		return lhs;
	}

	enum class FCurveStorageValues : ImGuiID
	{
		SCALE_X = 100,
		SCALE_Y,
		OFFSET_X,
		OFFSET_Y,
		IS_PANNING,
		START_X,
		START_Y,
	};

	void FCurveVec2(
		float* keys1, float* values1,
		float* leftHandleKeys1, float* leftHandleValue1,
		float* rightHandleKeys1, float* rightHandleValues1,
		int count1,

		float* keys2, float* values2,
		float* leftHandleKeys2, float* leftHandleValue2,
		float* rightHandleKeys2, float* rightHandleValues2,
		int count2,

		float* newCount1,
		float* newCount2)
	{
		if(!BeginChildFrame(1, ImVec2(200, 200), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			EndChildFrame();
			return;
		}

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
		{
			EndChildFrame();
			return;
		}

		bool isGridShown = true;
		float offset_x = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::OFFSET_X, 0.0f);
		float offset_y = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::OFFSET_Y, 0.0f);

		float scale_x = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::SCALE_X, 1.0f);
		float scale_y = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::SCALE_Y, 1.0f);

		const ImRect innerRect = window->InnerRect;

		auto transform_f2s = [&](const ImVec2& p) -> ImVec2
		{
			return ImVec2((p.x - offset_x) * scale_x + innerRect.Min.x, (p.y - offset_y) * scale_y + innerRect.Min.y);
		};

		auto transform_s2f = [&](const ImVec2& p) -> ImVec2
		{
			return ImVec2((p.x - innerRect.Min.x) / scale_x + offset_x, (p.y - innerRect.Min.y) / scale_y + offset_y);
		};

		// zoom with user
		if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowHovered())
		{
			auto s = powf(2, ImGui::GetIO().MouseWheel / 10.0);
			scale_x *= s;
			scale_y *= s;
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::SCALE_X, scale_x);
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::SCALE_Y, scale_y);
		}

		// pan with user
		if (ImGui::IsMouseReleased(1))
		{
			window->StateStorage.SetBool((ImGuiID)FCurveStorageValues::IS_PANNING, false);
		}
		if (window->StateStorage.GetBool((ImGuiID)FCurveStorageValues::IS_PANNING, false))
		{
			ImVec2 drag_offset = ImGui::GetMouseDragDelta(1);
			offset_x = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::START_X) - drag_offset.x;
			offset_y = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::START_Y) - drag_offset.y;

			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::OFFSET_X, offset_x);
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::OFFSET_Y, offset_y);
		}
		else if (ImGui::IsMouseDragging(1) && ImGui::IsWindowHovered())
		{
			window->StateStorage.SetBool((ImGuiID)FCurveStorageValues::IS_PANNING, true);
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::START_X, offset_x);
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::START_Y, offset_y);
		}

		// render grid
		if (isGridShown)
		{
			ImVec2 upperLeft_s = ImVec2(innerRect.Min.x, innerRect.Min.y);
			ImVec2 lowerRight_s = ImVec2(innerRect.Max.x, innerRect.Max.y);

			auto upperLeft_f = transform_s2f(upperLeft_s);
			auto lowerRight_f = transform_s2f(lowerRight_s);

			auto gridSize = 20.0f;
			auto sx = (int)(upperLeft_f.x / gridSize) * gridSize;
			auto sy = (int)(upperLeft_f.y / gridSize) * gridSize;
			auto ex = (int)(lowerRight_f.x / gridSize) * gridSize + gridSize;
			auto ey = (int)(lowerRight_f.y / gridSize) * gridSize + gridSize;

			for (auto y = sy; y <= ey; y += gridSize)
			{
				window->DrawList->AddLine(transform_f2s(ImVec2(upperLeft_f.x, y)), transform_f2s(ImVec2(lowerRight_f.x, y)), 0x55000000);
			}

			for (auto x = sx; x <= ex; x += gridSize)
			{
				window->DrawList->AddLine(transform_f2s(ImVec2(x, upperLeft_f.y)), transform_f2s(ImVec2(x, lowerRight_f.y)), 0x55000000);
			}
		}

		// move points
		for (int i = 0; i < count1; i++)
		{
			auto isChanged = false;
			float pointSize = 4;

			auto pos = transform_f2s(ImVec2(keys1[i], values1[i]));
			auto cursorPos = GetCursorPos();

			SetCursorScreenPos(ImVec2(pos.x - pointSize, pos.y - pointSize));
			PushID(i);

			InvisibleButton("", ImVec2(pointSize * 2, pointSize * 2));

			if (IsItemHovered())
			{
				window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
				window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
				window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
				window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
			}

			if (IsItemActive() && IsMouseClicked(0))
			{
				window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::START_X, pos.x);
				window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::START_Y, pos.y);
			}

			if (IsItemActive() && IsMouseDragging(0))
			{
				pos.x = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::START_X, pos.x);
				pos.y = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::START_Y, pos.y);
				pos += ImGui::GetMouseDragDelta();
				auto v = transform_s2f(pos);
				keys1[i] = v.x;
				values1[i] = v.y;
				isChanged = true;

				if (0 < i)
				{
					if (keys1[i] < keys1[i - 1])
					{
						std::swap(keys1[i], keys1[i - 1]);
						std::swap(values1[i], values1[i - 1]);
					}
				}

				if (i + 1 < count1)
				{
					if (keys1[i + 1] < keys1[i])
					{
						std::swap(keys1[i], keys1[i + 1]);
						std::swap(values1[i], values1[i + 1]);
					}
				}
			}

			PopID();
			SetCursorScreenPos(cursorPos);

			if (isChanged)
			{
				break;
			}
		}

		/*
		// move handles
		for (int i = 0; i < count1; i++)
		{
			auto isChanged = false;
			float pointSize = 4;

			auto pos = transform_f2s(ImVec2(leftHandleKeys1[i], leftHandleValue1[i]));
			auto cursorPos = GetCursorPos();

			SetCursorScreenPos(ImVec2(pos.x - pointSize, pos.y - pointSize));
			PushID(i);

			InvisibleButton("", ImVec2(pointSize * 2, pointSize * 2));

			if (IsItemHovered())
			{
				window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
				window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
				window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
				window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
			}

			if (IsItemActive() && IsMouseClicked(0))
			{
				window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::START_X, pos.x);
				window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::START_Y, pos.y);
			}

			if (IsItemActive() && IsMouseDragging(0))
			{
				pos.x = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::START_X, pos.x);
				pos.y = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::START_Y, pos.y);
				pos += ImGui::GetMouseDragDelta();
				auto v = transform_s2f(pos);
				keys1[i] = v.x;
				values1[i] = v.y;
				isChanged = true;

				if (0 < i)
				{
					if (keys1[i] < keys1[i - 1])
					{
						std::swap(keys1[i], keys1[i - 1]);
						std::swap(values1[i], values1[i - 1]);
					}
				}

				if (i + 1 < count1)
				{
					if (keys1[i + 1] < keys1[i])
					{
						std::swap(keys1[i], keys1[i + 1]);
						std::swap(values1[i], values1[i + 1]);
					}
				}
			}

			PopID();
			SetCursorScreenPos(cursorPos);

			if (isChanged)
			{
				break;
			}
		}
		*/

		// render curve
		for(int i = 0; i < count1 - 1; i++)
		{
			auto v1 = ImVec2(keys1[i+0], values1[i+0]);
			auto v2 = ImVec2(keys1[i+1], values1[i+1]);
			window->DrawList->AddLine(
				transform_f2s(v1),
				transform_f2s(v2),
				0x55000000);
		}


		EndChildFrame();
	}
}