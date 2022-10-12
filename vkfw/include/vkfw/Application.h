#ifndef VKFW_APPLICATION_H
#define VKFW_APPLICATION_H

#include <vkfw/vkfw.h>

#include <cstdint>
#include <memory>
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
#if defined vkfwWindows
		Application();
#elif defined vkfwLinux
		Application() = default;
#endif
		void initialize(const ApplicationSettings &settings);
		virtual ~Application();

		void run();

		inline const char *getName() const
		{
			return m_settings.name.c_str();
		}

	protected:
		virtual void postInitialize() {}
		virtual void update() {}
		virtual void record(VkCommandBuffer commandBuffer) {}
		virtual void onStop() {}
		virtual void onResize(uint32_t width, uint32_t height) {}

		inline VkDevice getDevice() const
		{
			return m_device;
		}

		inline uint32_t getWidth() const
		{
			return m_width;
		}

		inline uint32_t getHeight() const
		{
			return m_height;
		}

		inline VkSurfaceFormatKHR getSwapChainSurfaceFormat() const
		{
			return m_swapChainSurfaceFormat;
		}

		inline uint32_t getMaxSimultaneousFrames() const
		{
			return m_maxSimultaneousFrames;
		}

		inline size_t getSwapChainCount() const
		{
			return m_swapChainImages.size();
		}

		inline VkImage getSwapChainImage(size_t i) const
		{
			return m_swapChainImages[i];
		}

		inline uint32_t getSwapChainIndex() const
		{
			return m_swapChainIndex;
		}

		inline const VkAllocationCallbacks *getAllocationCallbacks() const
		{
			return m_allocationCallbacks.get();
		}

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
		void recreateSwapChainAndGetImages();
		void destroySwapChainAndClearImages();
		void createSynchronizationObjects();
		void destroySynchronizationObjects();
		void createCommandPoolAndCommandBuffers();
		void destroyCommandPoolAndCommandBuffers();
		void runOneFrame();
		void finalize();
		void render();
		void present();
		void resize(uint32_t width, uint32_t height);

#ifdef vkfwWindows
		friend LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);
#endif

		ApplicationSettings m_settings;
		uint32_t m_width{0};
		uint32_t m_height{0};
		uint32_t m_maxSimultaneousFrames{0};
		bool m_running{false};
#if defined vkfwWindows
		HINSTANCE m_hInstance{nullptr};
		HWND m_hWnd{nullptr};
#elif defined vkfwLinux
		Display *m_display{nullptr};
		Window m_window{None};
		VisualID m_visualId{None};
		Atom m_deleteWindowAtom{None};
#endif
		VkInstance m_instance{VK_NULL_HANDLE};
		VkSurfaceKHR m_surface{VK_NULL_HANDLE};
		VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
		VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
		std::unique_ptr<VkAllocationCallbacks> m_allocationCallbacks{nullptr};
		VkDevice m_device{VK_NULL_HANDLE};
		uint32_t m_graphicsAndPresentQueueIndex{gc_invalidQueueIndex};
		VkQueue m_graphicsAndPresentQueue{VK_NULL_HANDLE};
		VkSurfaceTransformFlagBitsKHR m_preTransform;
		VkPresentModeKHR m_presentMode;
		VkSwapchainKHR m_swapChain{VK_NULL_HANDLE};
		VkSurfaceFormatKHR m_swapChainSurfaceFormat;
		std::vector<VkImage> m_swapChainImages;
		uint32_t m_swapChainIndex{0};
		uint32_t m_currentFrame{0};
		std::vector<VkSemaphore> m_acquireSwapChainImageSemaphores;
		std::vector<VkSemaphore> m_submitFinishedSemaphores;
		std::vector<VkFence> m_frameFences;
		VkCommandPool m_commandPool{VK_NULL_HANDLE};
		std::vector<VkCommandBuffer> m_commandBuffers;
	};

}

#endif