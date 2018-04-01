#include "fcurve.h"
#include "imgui_internal.h"

#include <stdint.h>
#include <algorithm>

namespace ImGui
{
	/**
		Todo
		how to multi select point
		select curve
		add/remove point
	*/
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
		DELTA_X,
		DELTA_Y,
	};

	bool IsHovered(const ImVec2& v1, const ImVec2& v2, float radius)
	{
		ImVec2 diff;
		diff.x = v1.x - v2.x;
		diff.y = v1.y - v2.y;
		return diff.x * diff.x + diff.y * diff.y < radius * radius;
	}

	bool BeginFCurve()
	{
		if (!BeginChildFrame(1, ImVec2(200, 200), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			return false;
		}

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
		{
			return false;
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

		// get left button drag
		if (IsMouseDragging(0))
		{
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::DELTA_X, GetIO().MouseDelta.x);
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::DELTA_Y, GetIO().MouseDelta.y);
		}
		else
		{
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::DELTA_X, 0);
			window->StateStorage.SetFloat((ImGuiID)FCurveStorageValues::DELTA_Y, 0);
		}

		return true;
	}

	void EndFCurve()
	{
		EndChildFrame();
	}

	bool FCurve(
		float* keys, float* values,
		float* leftHandleKeys, float* leftHandleValues,
		float* rightHandleKeys, float* rightHandleValues,
		bool* kv_selected,
		int count,
		bool isLocked,
		ImU32 col,
		bool* selected,
		int* newCount,
		float* movedX,
		float* movedY,
		int* changedType)
	{
		ImGuiWindow* window = GetCurrentWindow();

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

		auto moveFWithS = [&](const ImVec2& p, const ImVec2& d) -> ImVec2
		{
			auto sp = transform_f2s(p);
			sp += d;
			return transform_s2f(sp);
		};

		if (isLocked || !(*selected))
		{

		}
		else
		{
			auto dx = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::DELTA_X, 0.0f);
			auto dy = window->StateStorage.GetFloat((ImGuiID)FCurveStorageValues::DELTA_Y, 0.0f);

			// move points
			int32_t movedIndex = -1;

			for (int i = 0; i < count; i++)
			{
				PushID(i + 1);

				auto isChanged = false;
				float pointSize = 4;

				auto pos = transform_f2s(ImVec2(keys[i], values[i]));
				auto cursorPos = GetCursorPos();

				SetCursorScreenPos(ImVec2(pos.x - pointSize, pos.y - pointSize));

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
					if (!GetIO().KeyShift)
					{
						for (int j = 0; j < count; j++)
						{
							kv_selected[j] = false;
						}
					}
					
					kv_selected[i] = !kv_selected[i];
				}

				if (IsItemActive() && IsMouseDragging(0))
				{
					movedIndex = i;
					isChanged = true;
				}

				PopID();
				SetCursorScreenPos(cursorPos);

				if (isChanged)
				{
					if (changedType != nullptr)
					{
						(*changedType) = 0;

						if (movedX != nullptr) (*movedX) = dx;
						if (movedY != nullptr) (*movedY) = dy;
					}

					break;
				}
			}

			// move points acctually
			if (movedIndex >= 0)
			{
				for (int i = 0; i < count; i++)
				{
					if (!kv_selected[i]) continue;

					{
						auto v = moveFWithS(ImVec2(keys[i], values[i]), ImVec2(dx, dy));
						keys[i] = v.x;
						values[i] = v.y;
					}
					
					{
						auto v = moveFWithS(ImVec2(leftHandleKeys[i], leftHandleValues[i]), ImVec2(dx, dy));
						leftHandleKeys[i] = v.x;
						leftHandleValues[i] = v.y;
					}

					{
						auto v = moveFWithS(ImVec2(rightHandleKeys[i], rightHandleValues[i]), ImVec2(dx, dy));
						rightHandleKeys[i] = v.x;
						rightHandleValues[i] = v.y;
					}
				}

				for (int i = 0; i < count; i++)
				{
					if (0 < i)
					{
						leftHandleKeys[i] = std::max(leftHandleKeys[i], keys[i - 1]);
					}

					leftHandleKeys[i] = std::min(leftHandleKeys[i], keys[i]);

					if (i < count - 1)
					{
						rightHandleKeys[i] = std::min(rightHandleKeys[i], keys[i + 1]);
					}

					rightHandleKeys[i] = std::max(rightHandleKeys[i], keys[i]);
				}

				// check values
				bool isChanged = true;

				while (isChanged)
				{
					isChanged = false;
					auto i = movedIndex;

					if (0 < i)
					{
						if (keys[i - 1] > keys[i])
						{
							std::swap(keys[i], keys[i - 1]);
							std::swap(values[i], values[i - 1]);
							std::swap(kv_selected[i], kv_selected[i - 1]);
							std::swap(leftHandleKeys[i], leftHandleKeys[i - 1]);
							std::swap(leftHandleValues[i], leftHandleValues[i - 1]);
							std::swap(rightHandleKeys[i], rightHandleKeys[i - 1]);
							std::swap(rightHandleValues[i], rightHandleValues[i - 1]);

							// To change button
							PushID(i - 1 + 1);
							auto id = GetID("");
							PopID();

							SetActiveID(id, window);

							movedIndex = i - 1;
							isChanged = true;
						}
					}

					i = movedIndex;

					if (i + 1 < count)
					{
						if (keys[i] > keys[i + 1])
						{
							std::swap(keys[i], keys[i + 1]);
							std::swap(values[i], values[i + 1]);
							std::swap(kv_selected[i], kv_selected[i + 1]);
							std::swap(leftHandleKeys[i], leftHandleKeys[i + 1]);
							std::swap(leftHandleValues[i], leftHandleValues[i + 1]);
							std::swap(rightHandleKeys[i], rightHandleKeys[i + 1]);
							std::swap(rightHandleValues[i], rightHandleValues[i + 1]);

							// To change button
							PushID(i + 1 + 1);
							auto id = GetID("");
							PopID();

							SetActiveID(id, window);

							movedIndex = i + 1;
							isChanged = true;
						}
					}
				}
			}

			// move left handles
			int32_t movedLIndex = -1;
			for (int i = 0; i < count; i++)
			{
				if (!kv_selected[i]) continue;

				auto isChanged = false;
				float pointSize = 4;

				auto centerPos = transform_f2s(ImVec2(keys[i], values[i]));
				auto pos = transform_f2s(ImVec2(leftHandleKeys[i], leftHandleValues[i]));
				auto cursorPos = GetCursorPos();

				SetCursorScreenPos(ImVec2(pos.x - pointSize, pos.y - pointSize));
				PushID(i + 0x1f0);

				InvisibleButton("", ImVec2(pointSize * 2, pointSize * 2));

				window->DrawList->AddLine(pos, centerPos, 0x55000000);

				if (IsItemHovered())
				{
					window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
					window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
					window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
					window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
				}

				if (IsItemActive() && IsMouseClicked(0))
				{
				}

				if (IsItemActive() && IsMouseDragging(0))
				{
					isChanged = true;
					movedLIndex = i;
				}

				PopID();
				SetCursorScreenPos(cursorPos);

				if (isChanged)
				{
					if (changedType != nullptr)
					{
						(*changedType) = 1;

						if (movedX != nullptr) (*movedX) = dx;
						if (movedY != nullptr) (*movedY) = dy;
					}

					break;
				}
			}

			if (movedLIndex >= 0)
			{
				for (int i = 0; i < count; i++)
				{
					if (!kv_selected[i]) continue;

					auto v = moveFWithS(ImVec2(leftHandleKeys[i], leftHandleValues[i]), ImVec2(dx, dy));
					leftHandleKeys[i] = v.x;
					leftHandleValues[i] = v.y;

					// movable area is limited
					if (0 < i)
					{
						leftHandleKeys[i] = std::max(leftHandleKeys[i], keys[i - 1]);
					}

					leftHandleKeys[i] = std::min(leftHandleKeys[i], keys[i]);
				}
			}

			// move right handles
			int32_t movedRIndex = -1;
			for (int i = 0; i < count; i++)
			{
				if (!kv_selected[i]) continue;

				auto isChanged = false;
				float pointSize = 4;

				auto centerPos = transform_f2s(ImVec2(keys[i], values[i]));
				auto pos = transform_f2s(ImVec2(rightHandleKeys[i], rightHandleValues[i]));
				auto cursorPos = GetCursorPos();

				SetCursorScreenPos(ImVec2(pos.x - pointSize, pos.y - pointSize));
				PushID(i + 0xaf0);

				InvisibleButton("", ImVec2(pointSize * 2, pointSize * 2));

				window->DrawList->AddLine(pos, centerPos, 0x55000000);

				if (IsItemHovered())
				{
					window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
					window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
					window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55000000);
					window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55000000);
				}

				if (IsItemActive() && IsMouseClicked(0))
				{
				}

				if (IsItemActive() && IsMouseDragging(0))
				{
					isChanged = true;
					movedRIndex = i;
				}

				PopID();
				SetCursorScreenPos(cursorPos);

				if (isChanged)
				{
					if (changedType != nullptr)
					{
						(*changedType) = 2;

						if (movedX != nullptr) (*movedX) = dx;
						if (movedY != nullptr) (*movedY) = dy;
					}

					break;
				}
			}

			if (movedRIndex >= 0)
			{
				for (int i = 0; i < count; i++)
				{
					if (!kv_selected[i]) continue;

					auto v = moveFWithS(ImVec2(rightHandleKeys[i], rightHandleValues[i]), ImVec2(dx, dy));
					rightHandleKeys[i] = v.x;
					rightHandleValues[i] = v.y;

					// movable area is limited
					if (i < count - 1)
					{
						rightHandleKeys[i] = std::min(rightHandleKeys[i], keys[i + 1]);
					}

					rightHandleKeys[i] = std::max(rightHandleKeys[i], keys[i]);
				}
			}
		}


		// render curve
		for (int i = 0; i < count - 1; i++)
		{
			auto v1 = ImVec2(keys[i + 0], values[i + 0]);
			auto v2 = ImVec2(keys[i + 1], values[i + 1]);
			window->DrawList->AddLine(
				transform_f2s(v1),
				transform_f2s(v2),
				col);
		}

		// render selected
		for (int i = 0; i < count; i++)
		{
			if (!kv_selected[i]) continue;

			int pointSize = 4;
			auto pos = transform_f2s(ImVec2(keys[i], values[i]));

			window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55FFFFFF);
			window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55FFFFFF);
			window->DrawList->AddLine(ImVec2(pos.x + pointSize, pos.y), ImVec2(pos.x, pos.y + pointSize), 0x55FFFFFF);
			window->DrawList->AddLine(ImVec2(pos.x - pointSize, pos.y), ImVec2(pos.x, pos.y - pointSize), 0x55FFFFFF);
		}

		return true;
	}
}