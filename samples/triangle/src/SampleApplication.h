#include <vkfw/Application.h>

#include <vector>

class SampleApplication : public vkfw::Application
{
public:
    virtual ~SampleApplication() = default;

protected:
    void postInitialize() override;
    void record(VkCommandBuffer commandBuffer) override;
    void postRun() override;
    void postResize(uint32_t width, uint32_t height) override;

private:
    void recreateSwapChainImageViewsAndFramebuffers();
    void destroySwapChainImageViewsAndFramebuffers();

    VkShaderModule m_vertModule{VK_NULL_HANDLE};
    VkShaderModule m_fragModule{VK_NULL_HANDLE};
    VkRenderPass m_renderPass{VK_NULL_HANDLE};
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
};