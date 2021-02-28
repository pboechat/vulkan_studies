#include <framework/Application.h>
#include <framework/framework.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iostream>

namespace framework
{
	template <typename ListType, typename ElementType, typename ComparerType>
	bool contains(const ListType& list, const ElementType* value, const ComparerType& comparer)
	{
		return std::find_if(list.begin(), list.end(), [value, comparer](const ElementType* otherValue) { return comparer(value, otherValue); }) != list.end();
	}

	bool strCmp(const char* a, const char* b)
	{
		return strcmp(a, b) == 0;
	}

	bool createInstance(QueueCapabilitiesMask requiredQueueCapabilities,
		const std::string& applicationName,
		const std::initializer_list<const char*>& layers,
		const std::initializer_list<const char*>& extensions,
		VkInstance& instance,
		const char*& failureReason)
	{
		VkApplicationInfo applicationInfo;
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = &applicationName[0];
		applicationInfo.applicationVersion = VK_MAKE_VERSION(Application::MajorVersion, Application::MinorVersion, Application::PatchVersion);
		applicationInfo.pEngineName = "framework";
		applicationInfo.engineVersion = VK_MAKE_VERSION(framework::MajorVersion, framework::MinorVersion, framework::PatchVersion);
		applicationInfo.apiVersion = VK_API_VERSION_1_1;

		std::vector<const char*> layersNames(layers.begin(), layers.end());
#if _DEBUG
		if (!contains(layersNames, "VK_LAYER_KHRONOS_validation", strCmp))
		{
			layersNames.emplace_back("VK_LAYER_KHRONOS_validation");
		}
#endif
		std::vector<const char*> extensionNames(extensions.begin(), extensions.end());

		if ((requiredQueueCapabilities & eQueueCap_Presentation) != 0)
		{
			if (!contains(extensionNames, "VK_KHR_surface", strCmp))
			{
				warn("application requires presentation queue capability but doesn't enable VK_KHR_surface. auto-enabling it.");
				extensionNames.emplace_back("VK_KHR_surface");
			}
#if defined _WIN32 || defined _WIN64
			if (!contains(extensionNames, "VK_KHR_win32_surface", strCmp))
			{
				extensionNames.emplace_back("VK_KHR_win32_surface");
			}
#endif
		}

		VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = (uint32_t)layersNames.size();
		instanceCreateInfo.ppEnabledLayerNames = layersNames.empty() ? nullptr : &layersNames[0];
		instanceCreateInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
		instanceCreateInfo.ppEnabledExtensionNames = extensionNames.empty() ? nullptr : &extensionNames[0];

		VkResult result;
		if ((result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance)) != VK_SUCCESS)
		{
			failureReason = getErrorString(result);
			return false;
		}

		return true;
	}

	bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIdx)
	{
#if defined _WIN32 || defined _WIN64
		auto result = vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIdx);
		return result;
#else
#error "Don't know how to check presentation support for this platform"
#endif
	}

	void printPhysicalDevicePropertiesAndFeatures(const std::vector<VkPhysicalDevice>& physicalDevices)
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

	template <typename ValueType, typename MaskType>
	ValueType getFirstMatch(MaskType requiredValues, MaskType values)
	{
		static_assert(std::is_integral_v<MaskType>, "MaskType must be an integral type");
		static_assert(std::is_integral_v<ValueType>, "ValueType must be an integral type");
		static_assert(sizeof(MaskType) >= sizeof(ValueType) * 8, "sizeof(MaskType) must be at least 8 times bigger than sizeof(ValueType)");
		unsigned long flippedBit;
		if (bitscan((unsigned long)(requiredValues & values), flippedBit))
		{
			return (ValueType)(1 << flippedBit);
		}
		else
		{
			return (ValueType)0;
		}
	}

	void getQueueFamilyCapabilitiesMask(const VkQueueFamilyProperties& queueFamilyProperties,
		QueueCapabilitiesMask& queueFamilyCapabililies)
	{
		queueFamilyCapabililies = 0;
		queueFamilyCapabililies |= ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 ? eQueueCap_Graphics : 0);
		queueFamilyCapabililies |= ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 ? eQueueCap_Compute : 0);
		queueFamilyCapabililies |= ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 ? eQueueCap_Transfer : 0);
	}

	bool selectPhysicalDevice(VkInstance instance, 
		QueueCapabilitiesMask requiredQueueCapabilities,
		uint32_t& queueDescriptorsCount,
		QueueDescriptorArray& queueDescriptors,
		VkPhysicalDevice& physicalDevice, 
		const char*& failureReason)
	{
		uint32_t numPhysicalDevices;
		VkResult result;
		if ((result = vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr)) != VK_SUCCESS)
		{
			failureReason = getErrorString(result);
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDevices;
		physicalDevices.resize(numPhysicalDevices);
		if ((result = vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, &physicalDevices[0])) != VK_SUCCESS)
		{
			failureReason = getErrorString(result);
			return false;
		}

#if _DEBUG
		printPhysicalDevicePropertiesAndFeatures(physicalDevices);
#endif

		if (physicalDevices.size() == 0)
		{
			failureReason = "no physical device found";
			return false;
		}

		if (requiredQueueCapabilities == eQueueCap_None)
		{
			queueDescriptorsCount = 0;
			physicalDevice = physicalDevices[0];
		}

		std::vector<VkQueueFamilyProperties> queueFamiliesProperties;
		for (auto& physicalDevice_ : physicalDevices)
		{
			auto devicesRequiredQueueCapabilities = requiredQueueCapabilities;
			uint32_t queueFamiliesCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamiliesCount, nullptr);
			queueFamiliesProperties.resize(queueFamiliesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamiliesCount, &queueFamiliesProperties[0]);

			queueDescriptorsCount = 0;
			for (uint32_t queueFamilyIdx = 0; queueFamilyIdx < queueFamiliesCount; ++queueFamilyIdx)
			{
				auto& queueFamilyProperties = queueFamiliesProperties[queueFamilyIdx];
				QueueCapabilitiesMask queueFamilyCapabilities;
				getQueueFamilyCapabilitiesMask(queueFamilyProperties, queueFamilyCapabilities);
				auto& queueDescriptor = queueDescriptors[queueDescriptorsCount];
				queueDescriptor.queueCapabilities = eQueueCap_None;
				QueueCapability queueCapability;
				while ((queueCapability = (QueueCapability)getFirstMatch<uint8_t>(devicesRequiredQueueCapabilities, queueFamilyCapabilities)) != eQueueCap_None)
				{
					queueDescriptor.queueCapabilities |= queueCapability;
					devicesRequiredQueueCapabilities &= ~queueCapability;
				}

				if ((devicesRequiredQueueCapabilities & eQueueCap_Presentation) != 0
					&& supportsPresentation(physicalDevice_, queueFamilyIdx))
				{
					queueDescriptor.queueCapabilities |= eQueueCap_Presentation;
					devicesRequiredQueueCapabilities &= ~eQueueCap_Presentation;
				}

				if (queueDescriptor.queueCapabilities == eQueueCap_None)
				{
					continue;
				}

				queueDescriptor.queueFamilyIdx = queueFamilyIdx;
				queueDescriptorsCount++;

				if (devicesRequiredQueueCapabilities == eQueueCap_None)
				{
					physicalDevice = physicalDevice_;
					return true;
				}
			}
		}

		failureReason = "couldn't find a suitable physical device";
		return false;
	}

	bool createDeviceAndGetQueues(VkPhysicalDevice physicalDevice,
		const std::initializer_list<const char*>& extensions,
		uint32_t queueCount,
		QueueDescriptorArray& queueDescriptors,
		VkDevice& device,
		VkQueueArray& queues,
		const char*& failureReason)
	{
		static const float c_queuePriorities[] = { 1.f };

		std::array<VkDeviceQueueCreateInfo, MaxQueueCount> deviceQueueCreateInfos;
		for (uint32_t queueDescIdx = 0; queueDescIdx < queueCount; ++queueDescIdx)
		{
			auto& queueDesc = queueDescriptors[queueDescIdx];
			auto& deviceQueueCreateInfo = deviceQueueCreateInfos[queueDescIdx];
			deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueCreateInfo.pNext = nullptr;
			deviceQueueCreateInfo.flags = 0;
			deviceQueueCreateInfo.queueFamilyIndex = queueDesc.queueFamilyIdx;
			deviceQueueCreateInfo.queueCount = 1;
			deviceQueueCreateInfo.pQueuePriorities = c_queuePriorities;
		}

		std::vector<const char*> extensionNames(extensions.begin(), extensions.end());

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

		VkResult result;
		if ((result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) != VK_SUCCESS)
		{
			failureReason = getErrorString(result);
			return false;
		}

		for (uint32_t queueIdx = 0; queueIdx < queueCount; ++queueIdx)
		{
			auto& queueDesc = queueDescriptors[queueIdx];
			vkGetDeviceQueue(device, queueDesc.queueFamilyIdx, 0, &queues[queueIdx]);
		}

		return true;
	}

	Application::Application(const char* name)
		: m_name(name)
	{

	}
	Application::~Application()
	{
		finalize();
	}

	void Application::initialize(QueueCapabilitiesMask requiredQueueCapabilities,
		const std::initializer_list<const char*>& layers,
		const std::initializer_list<const char*>& instanceExtensions,
		const std::initializer_list<const char*>& deviceExtensions)
	{
		const char* failureReason = nullptr;
		if (!createInstance(requiredQueueCapabilities, m_name, layers, instanceExtensions, m_instance, failureReason))
		{
			fail(failureReason);
		}
		if (!selectPhysicalDevice(m_instance, requiredQueueCapabilities, m_queueCount, m_queueDescriptors, m_physicalDevice, failureReason))
		{
			fail(failureReason);
		}
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);
		if (!createDeviceAndGetQueues(m_physicalDevice, deviceExtensions, m_queueCount, m_queueDescriptors, m_device, m_queues, failureReason))
		{
			fail(failureReason);
		}
	}

	void Application::finalize()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}

		if (m_instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}
	}

}