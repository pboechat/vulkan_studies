#include <vkfw/Application.h>

#if __linux__ && !__ANDROID__
#include <X11/Xutil.h>
#endif

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iostream>

namespace vkfw
{
	template <typename ListType, typename ElementType, typename ComparerType>
	bool contains(const ListType &list, const ElementType *value, const ComparerType &comparer)
	{
		return std::find_if(list.begin(), list.end(), [value, comparer](const ElementType *otherValue)
							{ return comparer(value, otherValue); }) != list.end();
	}

	bool strCmp(const char *a, const char *b)
	{
		return strcmp(a, b) == 0;
	}

	void printPhysicalDevicePropertiesAndFeatures(const std::vector<VkPhysicalDevice> &physicalDevices)
	{
		for (auto &physicalDevice : physicalDevices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			// TODO:
			std::cout << properties.deviceName << '\n';

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(physicalDevice, &features);

			// TODO:
		}
	}

	Application::~Application()
	{
		finalize();
	}

	Result Application::createInstance()
	{
		VkApplicationInfo applicationInfo;
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = m_name.c_str();
		applicationInfo.applicationVersion = VK_MAKE_VERSION(Application::MajorVersion, Application::MinorVersion, Application::PatchVersion);
		applicationInfo.pEngineName = "vkfw";
		applicationInfo.engineVersion = VK_MAKE_VERSION(vkfw::MajorVersion, vkfw::MinorVersion, vkfw::PatchVersion);
		applicationInfo.apiVersion = VK_API_VERSION_1_1;

		std::vector<const char *> layersNames;
#if _DEBUG
		if (!contains(layersNames, "VK_LAYER_KHRONOS_validation", strCmp))
		{
			layersNames.emplace_back("VK_LAYER_KHRONOS_validation");
		}
#endif
		std::vector<const char *> extensionNames;

		extensionNames.emplace_back("VK_KHR_surface");
#if defined _WIN32 || defined _WIN64
		if (!contains(extensionNames, "VK_KHR_win32_surface", strCmp))
		{
			extensionNames.emplace_back("VK_KHR_win32_surface");
		}
#elif __linux__ && !__ANDROID__
		if (!contains(extensionNames, VK_KHR_XLIB_SURFACE_EXTENSION_NAME, strCmp))
		{
			extensionNames.emplace_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
		}
#else
#error "don't know how to enable surfaces in the current platform"
#endif

		VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = (uint32_t)layersNames.size();
		instanceCreateInfo.ppEnabledLayerNames = layersNames.empty() ? nullptr : &layersNames[0];
		instanceCreateInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
		instanceCreateInfo.ppEnabledExtensionNames = extensionNames.empty() ? nullptr : &extensionNames[0];

		vkfwSafeVkCall(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

		return sc_success;
	}

	void Application::destroyInstance()
	{
		if (m_instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}
	}

	Result Application::createSurface()
	{
#if defined _WIN32 || defined _WIN64
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.flags = 0;
		surfaceCreateInfo.hinstance = m_hInstance;
		surfaceCreateInfo.hwnd = m_hWnd;
		vkfwSafeVkCall(vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface));
		return sc_success;
#elif __linux__ && !__ANDROID__
		VkXlibSurfaceCreateInfoKHR surfaceCreateInfo;
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.dpy = m_display;
		surfaceCreateInfo.window = m_window;
		vkfwSafeVkCall(vkCreateXlibSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface));
		return sc_success;
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

	Result Application::selectPhysicalDevice()
	{
		uint32_t numPhysicalDevices;
		vkfwSafeVkCall(vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, nullptr));

		std::vector<VkPhysicalDevice> physicalDevices;
		physicalDevices.resize(numPhysicalDevices);
		vkfwSafeVkCall(vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, &physicalDevices[0]));

#if _DEBUG
		printPhysicalDevicePropertiesAndFeatures(physicalDevices);
#endif

		if (physicalDevices.size() == 0)
		{
			return "no physical device found";
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
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
				{
					m_graphicsQueueIndex = queueFamilyIdx;
				}
				if (supportsPresentation(physicalDevice_, queueFamilyIdx))
				{
					m_presentationQueueIndex = queueFamilyIdx;
				}
				if (m_graphicsQueueIndex != sc_invalidQueueIndex && m_presentationQueueIndex != sc_invalidQueueIndex)
				{
					m_physicalDevice = physicalDevice_;
					return sc_success;
				}
			}
		}

		return "couldn't find a suitable physical device";
	}

	Result Application::createDeviceAndGetQueues()
	{
		static const float sc_queuePriorities[] = {1.f};

		uint32_t queueCount = 0;
		VkDeviceQueueCreateInfo deviceQueueCreateInfos[2];

		auto &deviceQueueCreateInfo = deviceQueueCreateInfos[queueCount++];
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfo.pNext = nullptr;
		deviceQueueCreateInfo.flags = 0;
		deviceQueueCreateInfo.queueFamilyIndex = m_graphicsQueueIndex;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.pQueuePriorities = sc_queuePriorities;

		if (m_graphicsQueueIndex != m_presentationQueueIndex)
		{
			auto &deviceQueueCreateInfo = deviceQueueCreateInfos[queueCount++];
			deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueCreateInfo.pNext = nullptr;
			deviceQueueCreateInfo.flags = 0;
			deviceQueueCreateInfo.queueFamilyIndex = m_presentationQueueIndex;
			deviceQueueCreateInfo.queueCount = 1;
			deviceQueueCreateInfo.pQueuePriorities = sc_queuePriorities;
		}

		std::vector<const char *> extensionNames;

		extensionNames.push_back("VK_KHR_swapchain");

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = queueCount;
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfos[0];
		deviceCreateInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
		deviceCreateInfo.ppEnabledExtensionNames = extensionNames.empty() ? nullptr : &extensionNames[0];
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
		deviceCreateInfo.pEnabledFeatures = nullptr;

		vkfwSafeVkCall(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));

		vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_graphicsQueue);
		if (m_graphicsQueueIndex != m_presentationQueueIndex)
		{
			vkGetDeviceQueue(m_device, m_presentationQueueIndex, 0, &m_presentationQueue);
		}
		else
		{
			m_presentationQueue = m_graphicsQueue;
		}

		return sc_success;
	}

	void Application::destroyDeviceAndClearQueues()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}
		m_graphicsQueue = VK_NULL_HANDLE;
		m_presentationQueue = VK_NULL_HANDLE;
		m_graphicsQueueIndex = sc_invalidQueueIndex;
		m_presentationQueueIndex = sc_invalidQueueIndex;
	}

	Result Application::createSwapChainAndGetImages()
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkfwSafeVkCall(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities));

		if (m_width < surfaceCapabilities.minImageExtent.width || m_width > surfaceCapabilities.maxImageExtent.width)
		{
			return "invalid width";
		}

		if (m_height < surfaceCapabilities.minImageExtent.height || m_height > surfaceCapabilities.maxImageExtent.height)
		{
			return "invalid height";
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

		auto swapChainCount = std::max(std::min(sc_maxSwapChainCount, surfaceCapabilities.maxImageCount), surfaceCapabilities.minImageCount);

		uint32_t surfaceFormatsCount = 0;
		vkfwSafeVkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatsCount, nullptr));

		std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
		vkfwSafeVkCall(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &surfaceFormatsCount, &surfaceFormats[0]));

		VkPresentModeKHR presentMode;
		{
			uint32_t presentModesCount;
			vkfwSafeVkCall(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModesCount, nullptr));
			if (presentModesCount == 0)
			{
				return "no present mode";
			}
			std::vector<VkPresentModeKHR> presentModes(presentModesCount);
			vkfwSafeVkCall(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModesCount, &presentModes[0]));
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
		swapChainCreateInfo.imageFormat = surfaceFormats[0].format;
		swapChainCreateInfo.imageColorSpace = surfaceFormats[0].colorSpace;
		swapChainCreateInfo.imageExtent = {m_width, m_height};
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		swapChainCreateInfo.preTransform = preTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		vkfwSafeVkCall(vkCreateSwapchainKHR(m_device, &swapChainCreateInfo, nullptr, &m_swapChain));

		vkfwSafeVkCall(vkGetSwapchainImagesKHR(m_device, m_swapChain, &swapChainCount, nullptr));
		m_swapChainImages.resize(swapChainCount);
		vkfwSafeVkCall(vkGetSwapchainImagesKHR(m_device, m_swapChain, &swapChainCount, &m_swapChainImages[0]));

		return sc_success;
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

	void Application::initialize(const char *name, uint32_t width, uint32_t height)
	{
		m_name = name;
		m_width = width;
		m_height = height;
		Result result;
		if (!(result = initializePresentationLayer()))
		{
			fail(result.failureReason);
		}
		if (!(result = createInstance()))
		{
			fail(result.failureReason);
		}
		if (!(result = createSurface()))
		{
			fail(result.failureReason);
		}
		if (!(result = selectPhysicalDevice()))
		{
			fail(result.failureReason);
		}
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);
		if (!(result = createDeviceAndGetQueues()))
		{
			fail(result.failureReason);
		}
		if (!(result = createSwapChainAndGetImages()))
		{
			fail(result.failureReason);
		}
	}

	void Application::run()
	{
		m_running = true;
#if defined _WIN32 || defined _WIN64
		while (m_running)
		{
			update();
			render();
			SwapBuffers(m_hDc);
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
			update();
			render();
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
	}

	bool Application::supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIdx) const
	{
#if defined _WIN32 || defined _WIN64
		return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIdx);
#elif __linux__ && !__ANDROID__
		return vkGetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIdx, m_display, m_visualId);
#else
#error "don't know how to check presentation support"
#endif
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

	Result Application::initializePresentationLayer()
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
			return 0;
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
			m_name.c_str(),
			dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			m_hInstance,
			nullptr);
		if (!m_hWnd)
		{
			return "failed to create window";
		}
		m_hDc = GetDC(m_hWnd);
		ShowWindow(m_hWnd, SW_SHOW);
		UpdateWindow(m_hWnd);
		return sc_success;
#elif __linux__ && !__ANDROID__
		m_display = XOpenDisplay(nullptr);
		if (m_display == nullptr)
		{
			return "cannot open display";
		}
		int defaultScreen = DefaultScreen(m_display);
		m_window = XCreateSimpleWindow(m_display, RootWindow(m_display, defaultScreen), 0, 0, m_width, m_height, 1, BlackPixel(m_display, defaultScreen), WhitePixel(m_display, defaultScreen));
		XSelectInput(m_display, m_window, ExposureMask | KeyPressMask);
		XMapWindow(m_display, m_window);
		XStoreName(m_display, m_window, m_name.c_str());
		m_visualId = XVisualIDFromVisual(DefaultVisual(m_display, defaultScreen));
		m_deleteWindowAtom = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(m_display, m_window, &m_deleteWindowAtom, 1);
		return sc_success;
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
		destroySwapChainAndClearImages();
		destroyDeviceAndClearQueues();
		destroySurface();
		destroyInstance();
		finalizePresentationLayer();
	}

	void Application::update()
	{
	}

	void Application::render()
	{
	}
}