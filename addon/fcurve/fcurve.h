
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
		int* changedType);

	/**
		@brief	FCurve
	*/
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
		float* newCount2);
}