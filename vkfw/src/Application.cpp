#include <vkfw/Application.h>

#if __linux__ && !__ANDROID__
#include <X11/Xutil.h>
#endif

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iostream>

namespace
{
	template <typename AType>
	struct _StrComparer;

	template <>
	struct _StrComparer<VkLayerProperties>
	{
		static bool compare(const VkLayerProperties &a, const char *b)
		{
			return strcmp(a.layerName, b) == 0;
		}
	};

	template <>
	struct _StrComparer<VkExtensionProperties>
	{
		static bool compare(const VkExtensionProperties &a, const char *b)
		{
			return strcmp(a.extensionName, b) == 0;
		}
	};

	template <typename ElementType>
	bool contains(const std::vector<ElementType> &vector, const char *value)
	{
		return std::find_if(vector.begin(), vector.end(), [&value](const auto &element)
							{ return _StrComparer<ElementType>::compare(element, value); }) != vector.end();
	}

	bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIdx, VkSurfaceKHR surface
#if __linux__ && !__ANDROID__
							  ,
							  Display *display, VisualID visualId
#endif
	)
	{
		VkBool32 supportsPresentation_;
#if defined _WIN32 || defined _WIN64
		supportsPresentation_ = vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIdx);
#elif __linux__ && !__ANDROID__
		supportsPresentation_ = vkGetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIdx, display, visualId);
#else
#error "don't know how to check presentation support"
#endif
		if (!supportsPresentation_)
		{
			return false;
		}
		VkBool32 supportsPresentationToSurface;
		vkfwCheckVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIdx, surface, &supportsPresentationToSurface));
		return supportsPresentationToSurface;
	}

}

namespace vkfw
{
	Application::~Application()
	{
		finalize();
	}

	void Application::initialize(const ApplicationSettings &settings)
	{
		m_settings = settings;
		m_width = settings.width;
		m_height = settings.height;

		initializePresentationLayer();
		createInstance();
		createSurface();
		selectPhysicalDevice();
		getPhysicalDeviceMemoryProperties();

		createDeviceAndGetQueues();
		createSwapChainAndGetImages();
		createSynchronizationObjects();
		createCommandPoolAndCommandBuffers();

		postInitialize();
	}

	void Application::createInstance()
	{
		VkApplicationInfo applicationInfo;
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = m_settings.name.c_str();
		applicationInfo.applicationVersion = VK_MAKE_VERSION(m_settings.majorVersion, m_settings.minorVersion, m_settings.patchVersion);
		applicationInfo.pEngineName = "vkfw";
		applicationInfo.engineVersion = VK_MAKE_VERSION(vkfw::gc_majorVersion, vkfw::gc_minorVersion, vkfw::gc_patchVersion);
		applicationInfo.apiVersion = VK_API_VERSION_1_1;

#if _DEBUG
		uint32_t availableLayerCount;
		vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(availableLayerCount);
		vkEnumerateInstanceLayerProperties(&availableLayerCount, &availableLayers[0]);

		std::cout << "Available layers:" << std::endl;
		for (const auto &layer : availableLayers)
		{
			std::cout << layer.layerName << std::endl;
		}
#endif

		std::vector<const char *> usedLayers;
#if _DEBUG
		if (contains(availableLayers, "VK_LAYER_KHRONOS_validation"))
		{
			usedLayers.emplace_back("VK_LAYER_KHRONOS_validation");
		}
		else
		{
			std::cout << "VK_LAYER_KHRONOS_validation not available, ignoring it" << std::endl;
		}
#endif

		uint32_t availableExtensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, &availableExtensions[0]);

#if _DEBUG
		std::cout << "Available extensions:" << std::endl;
		for (const auto &extension : availableExtensions)
		{
			std::cout << extension.extensionName << std::endl;
		}
#endif

		std::vector<const char *> extensions;

		if (!contains(availableExtensions, "VK_KHR_surface"))
		{
			fail("couldn't find VK_KHR_surface extension");
		}
		extensions.emplace_back("VK_KHR_surface");

		const char *platformSurfaceExtName =
#if defined _WIN32 || defined _WIN64
			"VK_KHR_win32_surface"
#elif __linux__ && !__ANDROID__
			VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#else
#error "don't know how to enable surfaces in the current platform"
#endif
			;
		if (!contains(availableExtensions, platformSurfaceExtName))
		{
			fail("couldn't find platform surface extension");
		}
		extensions.emplace_back(platformSurfaceExtName);

		VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = (uint32_t)usedLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = usedLayers.empty() ? nullptr : &usedLayers[0];
		instanceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : &extensions[0];

		vkfwCheckVkResult(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));
	}

	void Application::destroyInstance()
	{
		if (m_instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}
	}

	void Application::createSurface()
	{
#if defined _WIN32 || defined _WIN64
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.flags = 0;
		surfaceCreateInfo.hinstance = m_hInstance;
		surfaceCreateInfo.hwnd = m_hWnd;
		vkfwCheckVkResult(vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface));
#elif __linux__ && !__ANDROID__
		VkXlibSurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.flags = 0;
		surfaceCreateInfo.dpy = m_display;
		surfaceCreateInfo.window = m_window;
		vkfwCheckVkResult(vkCreateXlibSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface));
#else
#error "don't know how to create presentation surface"
#endif
	}

	void Application::destroySurface()
	{
		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			m_surface = VK_NULL_HANDLE;
		}
	}

	void Application::selectPhysicalDevice()
	{
		uint32_t numPhysicalDevices;
		vkfwCheckVkResult(vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, nullptr));

		std::vector<VkPhysicalDevice> physicalDevices;
		physicalDevices.resize(numPhysicalDevices);
		vkfwCheckVkResult(vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, &physicalDevices[0]));

#if _DEBUG
		std::cout << "Devices:" << std::endl;
		for (auto &physicalDevice : physicalDevices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			std::cout << properties.deviceName << std::endl;
		}
#endif

		if (physicalDevices.size() == 0)
		{
			fail("no physical device found");
		}

		std::vector<VkQueueFamilyProperties> queueFamiliesProperties;
		for (auto &physicalDevice_ : physicalDevices)
		{
			uint32_t queueFamiliesCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamiliesCount, nullptr);
			queueFamiliesProperties.resize(queueFamiliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamiliesCount, &queueFamiliesProperties[0]);

			for (uint32_t queueFamilyIdx = 0; queueFamilyIdx < queueFamiliesCount; ++queueFamilyIdx)
			{
				auto &queueFamilyProperties = queueFamiliesProperties[queueFamilyIdx];
				// not supporting separate graphics and present queues at the moment
				// see: https://github.com/KhronosGroup/Vulkan-Docs/issues/1234
				if (
					(queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
					supportsPresentation(physicalDevice_, queueFamilyIdx, m_surface
#if __linux__ && !__ANDROID__
										 ,
										 m_display, m_visualId
#endif
										 ))
				{
					m_graphicsAndPresentQueueIndex = queueFamilyIdx;
				}
				if (m_graphicsAndPresentQueueIndex != gc_invalidQueueIndex)
				{
					m_physicalDevice = physicalDevice_;
					return;
				}
			}
		}

		fail("couldn't find a suitable physical device");
	}

	void Application::createDeviceAndGetQueues()
	{
		static const float sc_queuePriorities[] = {1.f};

		uint32_t queueCount = 0;
		VkDeviceQueueCreateInfo deviceQueueCreateInfos[2];

		auto &deviceQueueCreateInfo = deviceQueueCreateInfos[queueCount++];
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.pNext = nullptr;
		deviceQueueCreateInfo.flags = 0;
		deviceQueueCreateInfo.queueFamilyIndex = m_graphicsAndPresentQueueIndex;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.pQueuePriorities = sc_queuePriorities;

		std::vector<const char *> extensions;

		extensions.push_back("VK_KHR_swapchain");

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = queueCount;
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfos[0];
		deviceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : &extensions[0];
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
		deviceCreateInfo.pEnabledFeatures = nullptr;

		vkfwCheckVkResult(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));

		vkGetDeviceQueue(m_device, m_graphicsAndPresentQueueIndex, 0, &m_graphicsAndPresentQueue);
	}

	void Application::destroyDeviceAndClearQueues()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}
		m_graphicsAndPresentQueue = VK_NULL_HANDLE;
		m_graphicsAndPresentQueueIndex = gc_invalidQueueIndex;
	}

	void Application::createSwapChainAndGetImages()
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkfwCheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities));

		if (m_width < surfaceCapabilities.minImageExtent.width || m_width > surfaceCapabilities.maxImageExtent.width)
		{
			fail("invalid width");
		}

		if (m_height < surfaceCapabilities.minImageExtent.height || m_height > surfaceCapabilities.maxImageExtent.height)
		{
			fail("invalid height");
		}

		VkSurfaceTransformFlagBitsKHR preTransform;
		if ((surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) != 0)
		{
			preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
		}
		else
		{
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}

		m_maxSimultaneousFrames = std::max(std::min(m_settings.maxSimultaneousFrames, surfaceCapabilities.maxImageCount), surfaceCapabilities.minImageCount);
		auto swapChainCount = m_maxSimultaneousFrames;

		uint32_t surfaceFormatsCount = 0;
		vkfwCheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatsCount, nullptr));

		std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
		vkfwCheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatsCount, &surfaceFormats[0]));

		m_swapChainSurfaceFormat = surfaceFormats[0];

		VkPresentModeKHR presentMode;
		{
			uint32_t presentModesCount;
			vkfwCheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModesCount, nullptr));
			if (presentModesCount == 0)
			{
				fail("no present mode");
			}
			std::vector<VkPresentModeKHR> presentModes(presentModesCount);
			vkfwCheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModesCount, &presentModes[0]));
			auto it = std::find_if(presentModes.begin(), presentModes.end(), [](const auto &presentMode)
								   { return presentMode == VK_PRESENT_MODE_MAILBOX_KHR; });
			if (it != presentModes.end())
			{
				presentMode = VK_PRESENT_MODE_MAILBOX_KHR; // supposedly, > swapchain image count
			}
			else
			{
				presentMode = VK_PRESENT_MODE_FIFO_KHR;
			}
		}

		VkSwapchainCreateInfoKHR swapChainCreateInfo;
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.pNext = nullptr;
		swapChainCreateInfo.flags = 0;
		swapChainCreateInfo.surface = m_surface;
		swapChainCreateInfo.minImageCount = swapChainCount;
		swapChainCreateInfo.imageFormat = m_swapChainSurfaceFormat.format;
		swapChainCreateInfo.imageColorSpace = m_swapChainSurfaceFormat.colorSpace;
		swapChainCreateInfo.imageExtent = {m_width, m_height};
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		swapChainCreateInfo.preTransform = preTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		vkfwCheckVkResult(vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain));

		vkfwCheckVkResult(vkGetSwapchainImagesKHR(m_device, m_swapChain, &swapChainCount, nullptr));
		m_swapChainImages.resize(swapChainCount);
		vkfwCheckVkResult(vkGetSwapchainImagesKHR(m_device, m_swapChain, &swapChainCount, &m_swapChainImages[0]));
	}

	void Application::destroySwapChainAndClearImages()
	{
		if (m_swapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
			m_swapChain = VK_NULL_HANDLE;
		}
		m_swapChainImages.clear();
	}

	void Application::createSynchronizationObjects()
	{
		m_frameFences.resize(m_maxSimultaneousFrames);
		m_acquireSwapChainImageSemaphores.resize(m_maxSimultaneousFrames);
		m_submitFinishedSemaphores.resize(m_maxSimultaneousFrames);

		VkFenceCreateInfo fenceInfo;
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = nullptr;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;
		for (uint32_t i = 0; i < m_maxSimultaneousFrames; ++i)
		{
			vkfwCheckVkResult(vkCreateFence(m_device, &fenceInfo, nullptr, &m_frameFences[i]));
			vkfwCheckVkResult(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_acquireSwapChainImageSemaphores[i]));
			vkfwCheckVkResult(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_submitFinishedSemaphores[i]));
		}
	}

	void Application::destroySynchronizationObjects()
	{
		for (uint32_t i = 0; i < m_maxSimultaneousFrames; ++i)
		{
			vkDestroySemaphore(m_device, m_acquireSwapChainImageSemaphores[i], nullptr);
			vkDestroySemaphore(m_device, m_submitFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_device, m_frameFences[i], nullptr);
		}
	}

	void Application::createCommandPoolAndCommandBuffers()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = nullptr;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = m_graphicsAndPresentQueueIndex;
		vkfwCheckVkResult(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
		commandBufferAllocateInfo.commandPool = m_commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = m_maxSimultaneousFrames;
		m_commandBuffers.resize(m_maxSimultaneousFrames);
		vkfwCheckVkResult(vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &m_commandBuffers[0]));
	}

	void Application::destroyCommandPoolAndCommandBuffers()
	{
		if (m_commandPool != VK_NULL_HANDLE)
		{
			if (!m_commandBuffers.empty())
			{
				vkFreeCommandBuffers(m_device, m_commandPool, (uint32_t)m_commandBuffers.size(), &m_commandBuffers[0]);
				m_commandBuffers.clear();
			}
			vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
		}
	}

	void Application::getPhysicalDeviceMemoryProperties()
	{
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);
	}

	void Application::run()
	{
		m_running = true;
#if defined _WIN32 || defined _WIN64
		while (m_running)
		{
			runOneFrame();
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if (!GetMessage(&msg, NULL, 0, 0))
				{
					m_running = false;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
#elif __linux__ && !__ANDROID__
		XEvent event;
		while (m_running)
		{
			runOneFrame();
			XNextEvent(m_display, &event);
			if (event.type == KeyPress)
			{
				char buf[128] = {0};
				KeySym keySym;
				XLookupString(&event.xkey, buf, sizeof buf, &keySym, NULL);
				if (keySym == XK_Escape)
				{
					m_running = false;
				}
			}
			else if (event.type == ClientMessage)
			{
				if (static_cast<Atom>(event.xclient.data.l[0]) == m_deleteWindowAtom)
				{
					m_running = false;
				}
			}
		}
#else
#error "don't know how to run"
#endif
		vkDeviceWaitIdle(m_device);

		onStop();
	}

	void Application::runOneFrame()
	{
		update();
		render();
		present();
	}

#if defined _WIN32 || defined _WIN64
	LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_KEYDOWN:
			int fwKeys;
			LPARAM keyData;
			fwKeys = (int)wParam;
			keyData = lParam;
			switch (fwKeys)
			{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
#endif

	void Application::initializePresentationLayer()
	{
#if defined _WIN32 || defined _WIN64
		constexpr char *const c_className = "VkfwClass";
		m_hInstance = GetModuleHandle(nullptr);
		WNDCLASSEX windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = wndProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = m_hInstance;
		windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName = nullptr;
		windowClass.lpszClassName = c_className;
		windowClass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);
		if (!RegisterClassEx(&windowClass))
		{
			fail("couldn't register window class");
		}
		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;
		RECT windowRect;
		windowRect.left = (long)0;
		windowRect.right = (long)m_width;
		windowRect.top = (long)0;
		windowRect.bottom = (long)m_height;
		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
		m_hWnd = CreateWindowEx(
			0,
			c_className,
			m_settings.name.c_str(),
			dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			m_hInstance,
			nullptr);
		if (m_hWnd == nullptr)
		{
			fail("couldn't create window");
		}
		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);
#elif __linux__ && !__ANDROID__
		m_display = XOpenDisplay(nullptr);
		if (m_display == nullptr)
		{
			fail("couldn't open display");
		}
		int defaultScreen = DefaultScreen(m_display);
		m_window = XCreateSimpleWindow(m_display, RootWindow(m_display, defaultScreen), 0, 0, m_width, m_height, 1, BlackPixel(m_display, defaultScreen), WhitePixel(m_display, defaultScreen));
		XSelectInput(m_display, m_window, ExposureMask | KeyPressMask);
		XMapWindow(m_display, m_window);
		XStoreName(m_display, m_window, m_settings.name.c_str());
		m_visualId = XVisualIDFromVisual(DefaultVisual(m_display, defaultScreen));
		m_deleteWindowAtom = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(m_display, m_window, &m_deleteWindowAtom, 1);
#else
#error "don't know how to initialize presentation layer"
#endif
	}

	void Application::finalizePresentationLayer()
	{
#if defined _WIN32 || defined _WIN64
		if (m_hWnd != nullptr)
		{
			DestroyWindow(m_hWnd);
			m_hWnd = nullptr;
		}
#elif __linux__ && !__ANDROID__
		if (m_display == nullptr)
		{
			return;
		}
		if (m_window != None)
		{
			XDestroyWindow(m_display, m_window);
			m_window = None;
		}
		XCloseDisplay(m_display);
		m_display = nullptr;
#else
#error "don't know how to finalize presentation layer"
#endif
	}

	void Application::finalize()
	{
		destroyCommandPoolAndCommandBuffers();
		destroySynchronizationObjects();
		destroySwapChainAndClearImages();
		destroyDeviceAndClearQueues();
		destroySurface();
		destroyInstance();
		finalizePresentationLayer();
	}

	void Application::render()
	{
		vkfwCheckVkResult(vkWaitForFences(m_device, 1, &m_frameFences[m_currentFrame], VK_TRUE, UINT64_MAX));
		vkfwCheckVkResult(vkResetFences(m_device, 1, &m_frameFences[m_currentFrame]));

		auto result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_acquireSwapChainImageSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_swapChainIndex);
		switch (result)
		{
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			fail("outdated swapchain");
			break;
		default:
			fail("couldn't acquire new swapchain image");
			break;
		}

		vkfwCheckVkResult(vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0));

		VkCommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = 0;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		vkfwCheckVkResult(vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &commandBufferBeginInfo));

		record(m_commandBuffers[m_currentFrame]);

		vkfwCheckVkResult(vkEndCommandBuffer(m_commandBuffers[m_currentFrame]))
	}

	void Application::present()
	{
		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_acquireSwapChainImageSemaphores[m_currentFrame];
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_submitFinishedSemaphores[m_currentFrame];
		if (vkQueueSubmit(m_graphicsAndPresentQueue, 1, &submitInfo, m_frameFences[m_currentFrame]) != VK_SUCCESS)
		{
			fail("couldn't submit commands");
		}

		VkPresentInfoKHR presentInfo;
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_submitFinishedSemaphores[m_currentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapChain;
		presentInfo.pImageIndices = &m_swapChainIndex;
		presentInfo.pResults = nullptr;

		auto result = vkQueuePresentKHR(m_graphicsAndPresentQueue, &presentInfo);
		switch (result)
		{
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			fail("outdated swapchain");
			break;
		default:
			fail("couldn't present");
			break;
		}

		m_currentFrame = (m_currentFrame + 1) % m_maxSimultaneousFrames;
	}
}