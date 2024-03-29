#ifndef VKFW_H
#define VKFW_H

#if defined _WIN32 || defined _WIN64
#define vkfwWindows
#elif __linux__ && !__ANDROID__
#define vkfwLinux
#endif

#ifdef vkfwWindows
#define NOMINMAX
#include <windows.h>
#elif defined vkfwLinux
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined vkfwWindows
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined vkfwLinux
#define VK_USE_PLATFORM_XLIB_KHR
#else
#error "unsupported plaform"
#endif

#include <vulkan/vulkan.h>

#if defined vkfwWindows
#define NOMINMAX
#include <vulkan/vulkan_win32.h>
#elif defined vkfwLinux
#include <vulkan/vulkan_xlib.h>
#endif

#include <cstdint>
#include <cassert>
#include <fstream>
#include <memory>
#include <vector>
#include <string>

namespace vkfw
{
	constexpr uint32_t gc_majorVersion = 1;
	constexpr uint32_t gc_minorVersion = 0;
	constexpr uint32_t gc_patchVersion = 0;

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

	inline void fail(const char *msg)
	{
#ifdef vkfwWindows
		MessageBox(nullptr, msg, "FAILURE", MB_ICONERROR | MB_OK);
#else
		printf("%s", msg);
#endif
		exit(-1);
	}

	template <typename... args_t>
	inline void fail(const char *fmt, args_t... args)
	{
		char msg[1024];
#ifdef vkfwWindows
		sprintf_s(msg, fmt, args...);
#else
		sprintf(msg, fmt, args...);
#endif
		fail(msg);
	}

	inline void warn(const char *msg)
	{
#ifdef vkfwWindows
		MessageBox(nullptr, msg, "WARNING", MB_ICONWARNING | MB_OK);
#else
		printf("%s", msg);
#endif
	}

	template <typename... args_t>
	inline void warn(const char *fmt, args_t... args)
	{
		char msg[1024];
#ifdef vkfwWindows
		sprintf_s(msg, fmt, args...);
#else
		sprintf(msg, fmt, args...);
#endif
		warn(msg);
	}

	inline const char *getVkErrorString(VkResult errorCode)
	{
		switch (errorCode)
		{
#define STR(r)   \
	case VK_##r: \
		return #r
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
			assert(false);
			return "UNKNOWN_ERROR";
		}
	}

	struct FileData
	{
		std::unique_ptr<char[]> value;
		size_t size;
	};

	inline FileData readFile(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			return {};
		}

		auto bufferSize = file.tellg();
		auto buffer = std::make_unique<char[]>(bufferSize);
		file.seekg(0);
		file.read(buffer.get(), bufferSize);
		file.close();

		return {std::move(buffer), (size_t)bufferSize};
	}

}

#define vkfwSafeVkCall(vkCall)                         \
	{                                                  \
		VkResult __vkResult;                           \
		if ((__vkResult = vkCall) != VK_SUCCESS)       \
		{                                              \
			return vkfw::getVkErrorString(__vkResult); \
		}                                              \
	}

#define vkfwCheckResult(call)                 \
	{                                         \
		Result __result;                      \
		if (!(result = call))                 \
		{                                     \
			vkfw::fail(result.failureReason); \
		}                                     \
	}

#define vkfwCheckVkResult(vkCall)                           \
	{                                                       \
		VkResult __vkResult;                                \
		if ((__vkResult = vkCall) != VK_SUCCESS)            \
		{                                                   \
			vkfw::fail(vkfw::getVkErrorString(__vkResult)); \
		}                                                   \
	}

#define vkfwArraySize(arr) sizeof(arr) / sizeof(arr[0])

#define vkfwDegToRad 0.0174532925f

#if defined vkfwWindows
#define vkfwKeyLeft VK_LEFT
#define vkfwKeyRight VK_RIGHT
#define vkfwKeyUp VK_UP
#define vkfwKeyDown VK_DOWN
#elif defined vkfwLinux
#define vkfwKeyLeft XK_Left
#define vkfwKeyRight XK_Right
#define vkfwKeyUp XK_Up
#define vkfwKeyDown XK_Down
#endif

#endif