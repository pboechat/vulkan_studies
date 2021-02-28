#ifndef APPLICATION_H
#define APPLICATION_H

#include <vulkan/vulkan.h>

#include <array>
#include <exception>
#include <initializer_list>
#include <string>
#include <tuple>
#include <vector>

namespace framework
{
	using ApplicationException = std::exception;

	enum QueueCapability : uint8_t
	{
		eQueueCap_None = 0,
		eQueueCap_Compute = 1,
		eQueueCap_Graphics = (1 << 1),
		eQueueCap_Presentation = (1 << 2),
		eQueueCap_Transfer = (1 << 3)

	};
	using QueueCapabilitiesMask = uint64_t;

	struct QueueDescriptor
	{
		uint32_t queueFamilyIdx{ 0 };
		QueueCapabilitiesMask queueCapabilities{ eQueueCap_None };

	};

	constexpr size_t MaxQueueCount = 8;

	using QueueDescriptorArray = std::array<QueueDescriptor, MaxQueueCount>;
	using VkQueueArray = std::array<VkQueue, MaxQueueCount>;

	class Application
	{
	public:
		static constexpr int MajorVersion = 1;
		static constexpr int MinorVersion = 0;
		static constexpr int PatchVersion = 0;

		Application(const char* name);
		virtual ~Application();

		void initialize(QueueCapabilitiesMask requiredQueueCapabilities,
			const std::initializer_list<const char*>& layers = {},
			const std::initializer_list<const char*>& instanceExtensions = {},
			const std::initializer_list<const char*>& deviceExtensions = {});
		void finalize();

	private:
		std::string m_name;
		VkInstance m_instance{ VK_NULL_HANDLE };
		VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
		VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
		VkDevice m_device{ VK_NULL_HANDLE };
		uint32_t m_queueCount{ 0 };
		QueueDescriptorArray m_queueDescriptors;
		VkQueueArray m_queues;

	};

}

#endif