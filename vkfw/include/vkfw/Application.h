#ifndef VKFW_APPLICATION_H
#define VKFW_APPLICATION_H

#include <vkfw/vkfw.h>

#include <cstdint>
#include <string>
#include <vector>

namespace vkfw
{
	struct ApplicationSettings
	{
		std::string name{"application"};
		uint32_t width{1024};
		uint32_t height{768};
		uint32_t maxSimultaneousFrames{3};
		uint32_t majorVersion{1};
		uint32_t minorVersion{0};
		uint32_t patchVersion{0};
	};

	constexpr uint32_t gc_invalidQueueIndex = ~0;

	class Application
	{
	public:
		Application() = default;
		void initialize(const ApplicationSettings &settings);
		virtual ~Application();

		void run();

		inline const char *getName() const
		{
			return m_settings.name.c_str();
		}

	protected:
		virtual void update() = 0;

	private:
		void initializePresentationLayer();
		void finalizePresentationLayer();
		void createInstance();
		void destroyInstance();
		void createSurface();
		void destroySurface();
		void selectPhysicalDevice();
		void getPhysicalDeviceMemoryProperties();
		void createDeviceAndGetQueues();
		void destroyDeviceAndClearQueues();
		void createSwapChainAndGetImages();
		void destroySwapChainAndClearImages();
		void createSynchronizationObjects();
		void destroySynchronizationObjects();
		void createCommandPoolAndCommandBuffers();
		void destroyCommandPoolAndCommandBuffers();
		void finalize();
		void render();
		void present();

		ApplicationSettings m_settings;
		uint32_t m_width{0};
		uint32_t m_height{0};
		bool m_running{false};
#if defined _WIN32 || defined _WIN64
		HINSTANCE m_hInstance{nullptr};
		HWND m_hWnd{nullptr};
#elif __linux__ && !__ANDROID__
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
		uint32_t m_graphicsAndPresentQueueIndex{gc_invalidQueueIndex};
		VkQueue m_graphicsAndPresentQueue{VK_NULL_HANDLE};
		VkSwapchainKHR m_swapChain;
		std::vector<VkImage> m_swapChainImages;
		uint32_t m_swapChainIndex{0};
		uint32_t m_currentFrame{0};
		std::vector<VkSemaphore> m_acquireSwapChainImageSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_frameFences;
		VkCommandPool m_commandPool{VK_NULL_HANDLE};
		std::vector<VkCommandBuffer> m_commandBuffers;
	};

}

#endif