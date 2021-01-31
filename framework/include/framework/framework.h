#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <vulkan/vulkan.h>

#include <string>

namespace framework
{
	constexpr int MajorVersion = 1;
	constexpr int MinorVersion = 0;
	constexpr int PatchVersion = 0;

	const char* getErrorString(VkResult errorCode);

}

#endif