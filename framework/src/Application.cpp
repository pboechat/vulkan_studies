#include <framework/Application.h>
#include <framework/framework.h>

#include <algorithm>
#include <cstring>
#include <iostream>

namespace framework
{
	Application::Application(const char* name)
		: m_name(name)
	{
	}

	Application::~Application()
	{
		destroyInstance();
	}

	void Application::initialize(std::initializer_list<const char*> layers,
		std::initializer_list<const char*> extensions)
	{
		createInstance(layers, extensions);
		selectPhysicalDevice();
		queryPhysicalDeviceMemoryProperties();
	}

	void Application::finalize()
	{
		destroyInstance();
	}

	void Application::createInstance(std::initializer_list<const char*> layers,
		std::initializer_list<const char*> extensions)
	{
		VkApplicationInfo applicationInfo;
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = &m_name[0];
		applicationInfo.applicationVersion = VK_MAKE_VERSION(Application::MajorVersion, Application::MinorVersion, Application::PatchVersion);
		applicationInfo.pEngineName = "framework";
		applicationInfo.engineVersion = VK_MAKE_VERSION(framework::MajorVersion, framework::MinorVersion, framework::PatchVersion);
		applicationInfo.apiVersion = VK_API_VERSION_1_1;

		std::vector<const char*> layersNames(layers.begin(), layers.end());
#if _DEBUG
		auto it = std::find_if(layersNames.begin(), layersNames.end(), [](const char* layerName) { return strcmp(layerName, "VK_LAYER_KHRONOS_validation") == 0; });
		if (it == layersNames.end())
		{
			layersNames.emplace_back("VK_LAYER_KHRONOS_validation");
		}
#endif
		std::vector<const char*> extensionNames(extensions.begin(), extensions.end());

		VkInstanceCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.pApplicationInfo = &applicationInfo;
		createInfo.enabledLayerCount = (uint32_t)layersNames.size();
		createInfo.ppEnabledLayerNames = layersNames.empty() ? nullptr : &layersNames[0];
		createInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
		createInfo.ppEnabledExtensionNames = extensionNames.empty() ? nullptr : &extensionNames[0];

		VkResult result;
		if ((result = vkCreateInstance(&createInfo, nullptr, &m_instance)) != VK_SUCCESS)
		{
			throw ApplicationException(getErrorString(result));
		}

		m_isInstanceCreated = true;
	}

	void Application::destroyInstance()
	{
		if (!m_isInstanceCreated)
		{
			return;
		}
		vkDestroyInstance(m_instance, nullptr);
		m_isInstanceCreated = false;
	}

	void Application::selectPhysicalDevice()
	{
		uint32_t numPhysicalDevices;
		VkResult result;
		if ((result = vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, nullptr)) != VK_SUCCESS)
		{
			throw ApplicationException(getErrorString(result));
		}

		std::vector<VkPhysicalDevice> physicalDevices;
		physicalDevices.resize(numPhysicalDevices);
		if ((result = vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, &physicalDevices[0])) != VK_SUCCESS)
		{
			throw ApplicationException(getErrorString(result));
		}

#if _DEBUG
		printPhysicalDevicePropertiesAndFeatures(physicalDevices);
#endif

		m_physicalDevice = physicalDevices[0];
	}

	void Application::printPhysicalDevicePropertiesAndFeatures(const std::vector<VkPhysicalDevice>& physicalDevices)
	{
		for (auto& physicalDevice : physicalDevices)
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

	void Application::queryPhysicalDeviceMemoryProperties()
	{
	}

}