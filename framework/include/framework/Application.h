#ifndef APPLICATION_H
#define APPLICATION_H

#include <vulkan/vulkan.h>

#include <exception>
#include <initializer_list>
#include <string>
#include <vector>

namespace framework
{
	using ApplicationException = std::exception;

	class Application
	{
	public:
		static constexpr int MajorVersion = 1;
		static constexpr int MinorVersion = 0;
		static constexpr int PatchVersion = 0;

		Application(const char* name);
		virtual ~Application();

		void initialize(std::initializer_list<const char*> layers = {},
			std::initializer_list<const char*> extensions = {});
		void finalize();

	private:
		std::string m_name;
		VkInstance m_instance{ 0 };
		bool m_isInstanceCreated{ false };
		VkPhysicalDevice m_physicalDevice{ 0 };

		void createInstance(std::initializer_list<const char*> layers,
			std::initializer_list<const char*> extensions);
		void destroyInstance();
		void selectPhysicalDevice();
		void queryPhysicalDeviceMemoryProperties();
		void printPhysicalDevicePropertiesAndFeatures(const std::vector<VkPhysicalDevice>& physicalDevices);

	};

}

#endif