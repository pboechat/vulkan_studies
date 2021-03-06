#include <framework/framework.h>

#if defined _WIN32 || defined _WIN64
#include <WinUser.h>
#endif

#include <cstdlib>

namespace framework
{
	void fail(const char* msg)
	{
#if defined _WIN32 || defined _WIN64
		MessageBox(nullptr, msg, "FAILURE", MB_ICONERROR | MB_OK);
#endif
		exit(-1);
	}

	void warn(const char* msg)
	{
#if defined _WIN32 || defined _WIN64
		MessageBox(nullptr, msg, "WARNING", MB_ICONWARNING | MB_OK);
#endif
	}

	const char* getErrorString(VkResult errorCode)
	{
		switch (errorCode)
		{
#define STR(r) case VK_ ##r: return #r
			STR(NOT_READY);
			STR(TIMEOUT);
			STR(EVENT_SET);
			STR(EVENT_RESET);
			STR(INCOMPLETE);
			STR(ERROR_OUT_OF_HOST_MEMORY);
			STR(ERROR_OUT_OF_DEVICE_MEMORY);
			STR(ERROR_INITIALIZATION_FAILED);
			STR(ERROR_DEVICE_LOST);
			STR(ERROR_MEMORY_MAP_FAILED);
			STR(ERROR_LAYER_NOT_PRESENT);
			STR(ERROR_EXTENSION_NOT_PRESENT);
			STR(ERROR_FEATURE_NOT_PRESENT);
			STR(ERROR_INCOMPATIBLE_DRIVER);
			STR(ERROR_TOO_MANY_OBJECTS);
			STR(ERROR_FORMAT_NOT_SUPPORTED);
			STR(ERROR_SURFACE_LOST_KHR);
			STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
			STR(SUBOPTIMAL_KHR);
			STR(ERROR_OUT_OF_DATE_KHR);
			STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
			STR(ERROR_VALIDATION_FAILED_EXT);
			STR(ERROR_INVALID_SHADER_NV);
#undef STR
		default:
			return "UNKNOWN_ERROR";
		}
	}

	bool bitscan(unsigned long mask, unsigned long& index)
	{
#if defined _WIN32 || defined _WIN64
		return _BitScanForward64(&index, mask);
#else
# error don't know how to implement bitscan
#endif
	}

}