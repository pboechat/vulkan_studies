#define _CRT_SECURE_NO_WARNINGS
#include <vkfw/Application.h>

#include <memory>

struct Model;

struct Buffer
{
    VkBuffer handle{VK_NULL_HANDLE};
    VkDeviceMemory backingMemory{VK_NULL_HANDLE};
};

struct Image
{
    VkImage handle{VK_NULL_HANDLE};
    VkDeviceMemory backingMemory{VK_NULL_HANDLE};
};

struct PipelineLayout
{
    VkPipelineLayout handle{VK_NULL_HANDLE};
    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
};

struct SceneConstants
{
    float view[16]{1, 0, 0, 0,
                   0, 1, 0, 0,
                   0, 0, 1, 0,
                   0, 0, 0, 1};
    float projection[16];
};

class ObjLoaderApplication : public vkfw::Application
{
public:
    ObjLoaderApplication();
    virtual ~ObjLoaderApplication();

protected:
    void postInitialize() override;
    bool preRun(int argc, char **argv) override;
    void postRun() override;
    void postResize(uint32_t width, uint32_t height) override;
    void update() override;
    void record(VkCommandBuffer commandBuffer) override;
    void keyDown(uint32_t keyCode) override;

private:
    void recreateDepthStencilImageSwapChainImageViewsAndFramebuffers();
    void destroyDepthStencilImageSwapChainImageViewsAndFramebuffers();

    VkShaderModule m_vertModule{VK_NULL_HANDLE};
    VkShaderModule m_fragModule{VK_NULL_HANDLE};
    VkRenderPass m_renderPass{VK_NULL_HANDLE};
    PipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<Image> m_depthStencilImages;
    std::vector<VkImageView> m_depthStencilImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<Buffer> m_sceneConstantBuffers;
    SceneConstants m_sceneConstants{};
    VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::string m_modelPath;
    std::unique_ptr<Model> m_model{nullptr};
    float m_cameraPosition[3]{0, 0, -1};
};