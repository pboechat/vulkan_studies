#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#if defined _WIN32 || defined _WIN64
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <string>

namespace framework
{
	constexpr int MajorVersion = 1;
	constexpr int MinorVersion = 0;
	constexpr int PatchVersion = 0;

	void fail(const char* msg);
	void warn(const char* msg);

	const char* getErrorString(VkResult errorCode);

#if defined _WIN32 || defined _WIN64
	bool bitscan(unsigned long mask, unsigned long& index);
#endif

}

#endif