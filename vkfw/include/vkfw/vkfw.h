#ifndef VKFW_H
#define VKFW_H

#if defined _WIN32 || defined _WIN64
#define VK_USE_PLATFORM_WIN32_KHR
#elif __linux__ && !__ANDROID__
#define VK_USE_PLATFORM_XLIB_KHR
#else
#error "unsupported plaform"
#endif
#include <vulkan/vulkan.h>
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#elif __linux__ && !__ANDROID__
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif

#include <string>

namespace vkfw
{
	constexpr int MajorVersion = 1;
	constexpr int MinorVersion = 0;
	constexpr int PatchVersion = 0;

	struct Result
	{
		const char *const failureReason;

		Result(const char *failureReason = nullptr) : failureReason(failureReason) {}
		Result(const Result &other) : failureReason(other.failureReason) {}
		inline operator bool() const
		{
			return failureReason == nullptr;
		}
		inline Result operator=(const Result &other)
		{
			return Result(other.failureReason);
		}
	};

	extern const Result sc_success;

	void fail(const char *msg);
	void warn(const char *msg);
	const char *getVkErrorString(VkResult errorCode);

}

#define vkfwSafeVkCall(vkCall)                         \
	{                                                  \
		VkResult __vkResult;                           \
		if ((__vkResult = vkCall) != VK_SUCCESS)       \
		{                                              \
			return vkfw::getVkErrorString(__vkResult); \
		}                                              \
	}

#endif