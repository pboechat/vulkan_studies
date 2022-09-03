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

		void initialize(const char* name, uint32_t width, uint32_t height);
		void run();

		inline const char* getName() const
		{
			return m_name.c_str();
		}

	protected:
		virtual void update() = 0;

	private:
		static constexpr uint32_t sc_invalidQueueIndex = ~0;
		static constexpr uint32_t sc_maxSwapChainCount = 3;

		void initializePresentationLayer();
		void finalizePresentationLayer();
		void createInstance();
		void destroyInstance();
		void createSurface();
		void destroySurface();
		void selectPhysicalDevice();
		void getPhysicalDeviceMemoryProperties();
		bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIdx) const;
		void createDeviceAndGetQueues();
		void destroyDeviceAndClearQueues();
		void createSwapChainAndGetImages();
		void destroySwapChainAndClearImages();
		void createSemaphores();
		void destroySemaphores();
		void createCommandPoolAndCommandBuffers();
		void destroyCommandPoolAndCommandBuffers();
		void finalize();
		void render();
		void present();

		std::string m_name;
		uint32_t m_width{ 0 };
		uint32_t m_height{ 0 };
		bool m_running{ false };
#if defined _WIN32 || defined _WIN64
		HINSTANCE m_hInstance{ nullptr };
		HWND m_hWnd{ nullptr };
#elif __linux__ && !__ANDROID__
		Display* m_display{ nullptr };
		Window m_window{ None };
		VisualID m_visualId{ None };
		Atom m_deleteWindowAtom{ None };
#endif
		VkInstance m_instance{ VK_NULL_HANDLE };
		VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
		VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
		VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
		VkDevice m_device{ VK_NULL_HANDLE };
		uint32_t m_graphicsAndPresentQueueIndex{ sc_invalidQueueIndex };
		VkQueue m_graphicsAndPresentQueue{ VK_NULL_HANDLE };
		VkSwapchainKHR m_swapChain;
		std::vector<VkImage> m_swapChainImages;
		uint32_t m_swapChainIndex{ 0 };
		VkSemaphore m_acquireSwapChainImageSemaphore{ VK_NULL_HANDLE };
		VkSemaphore m_renderFinishedSemaphore{ VK_NULL_HANDLE };
		VkCommandPool m_commandPool{ VK_NULL_HANDLE };
		std::vector<VkCommandBuffer> m_commandBuffers;
	};

}

#endif