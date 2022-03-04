#ifndef VKFW_APPLICATION_H
#define VKFW_APPLICATION_H

#include <vkfw/vkfw.h>

#include <string>
#include <vector>

namespace vkfw
{
	using ApplicationException = std::exception;

	class Application
	{
	public:
		static constexpr int MajorVersion = 1;
		static constexpr int MinorVersion = 0;
		static constexpr int PatchVersion = 0;

		Application() = default;
		virtual ~Application();

		void initialize(const char *name, uint32_t width, uint32_t height);
		void run();

		inline const char *getName() const
		{
			return m_name.c_str();
		}

	protected:
		virtual void update();

	private:
		constexpr static uint32_t sc_invalidQueueIndex = ~0u;
		constexpr static uint32_t sc_maxSwapChainCount = 3;

		Result initializePresentationLayer();
		void finalizePresentationLayer();
		Result createInstance();
		void destroyInstance();
		Result createSurface();
		void destroySurface();
		Result selectPhysicalDevice();
		bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIdx) const;
		Result createDeviceAndGetQueues();
		void destroyDeviceAndClearQueues();
		Result createSwapChainAndGetImages();
		void destroySwapChainAndClearImages();
		void finalize();
		void render();

		std::string m_name;
		uint32_t m_width{0};
		uint32_t m_height{0};
		bool m_running{false};
#if __linux__ && !__ANDROID__
		Display *m_display{nullptr};
		Window m_window{None};
		VisualID m_visualId{None};
		Atom m_deleteWindowAtom{None};
#endif
		VkInstance m_instance{VK_NULL_HANDLE};
		VkSurfaceKHR m_surface{VK_NULL_HANDLE};
		VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
		VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
		VkDevice m_device{VK_NULL_HANDLE};
		uint32_t m_graphicsQueueIndex{sc_invalidQueueIndex};
		uint32_t m_presentationQueueIndex{sc_invalidQueueIndex};
		VkQueue m_graphicsQueue{VK_NULL_HANDLE};
		VkQueue m_presentationQueue{VK_NULL_HANDLE};
		VkSwapchainKHR m_swapChain;
		std::vector<VkImage> m_swapChainImages;
	};

}

#endif