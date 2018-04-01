
#pragma once

#include"imgui.h"

namespace ImGui
{
	enum class ImInterporationType
	{
		Linear,
		Cubic,
	};

	bool BeginFCurve();

	void EndFCurve();

	bool FCurve(
		int fcurve_id,
		float* keys, float* values,
		float* leftHandleKeys, float* leftHandleValues,
		float* rightHandleKeys, float* rightHandleValues,
		bool* kv_selected,
		int count,
		bool isLocked,
		ImU32 col,
		bool selected,
		int* newCount,
		bool* newSelected,
		float* movedX,
		float* movedY,
		int* changedType);
}