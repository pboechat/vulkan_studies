#include "ObjLoaderApplication.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

#include <iostream>
#include <functional>
#include <numeric>
#include <memory>
#include <vector>

struct Vertex
{
    float position[3];
    float normal[3];
    float uv[2];
};

struct Mesh
{
    Buffer vertexBuffer;
    Buffer stagingVertexBuffer;
    Buffer indexBuffer;
    Buffer stagingIndexBuffer;
    size_t indexCount;
};

struct Model
{
    std::vector<Mesh> meshes;
};

constexpr VkFormat gc_depthStencilFormat = VK_FORMAT_D32_SFLOAT;

namespace
{
    void createRenderPass(VkDevice device, const VkAllocationCallbacks *allocCb, VkFormat colorAttachmentFormat, VkFormat depthStencilAttachmentFormat, VkRenderPass &renderPass)
    {
        VkAttachmentDescription attachmentDescriptions[2];

        auto &colorAttachmentDescription = attachmentDescriptions[0];
        colorAttachmentDescription.flags = 0;
        colorAttachmentDescription.format = colorAttachmentFormat;
        colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        auto &depthStencilAttachmentDescription = attachmentDescriptions[1];
        depthStencilAttachmentDescription.flags = 0;
        depthStencilAttachmentDescription.format = depthStencilAttachmentFormat;
        depthStencilAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        depthStencilAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthStencilAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthStencilAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthStencilAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthStencilAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthStencilAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentReference;
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthStencilAttachmentReference;
        depthStencilAttachmentReference.attachment = 1;
        depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;

        VkSubpassDescription subpassDescription;
        subpassDescription.flags = 0;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkRenderPassCreateInfo renderPassCreateInfo;
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext = nullptr;
        renderPassCreateInfo.flags = 0;
        renderPassCreateInfo.attachmentCount = vkfwArraySize(attachmentDescriptions);
        renderPassCreateInfo.pAttachments = attachmentDescriptions;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = nullptr;

        vkfwCheckVkResult(vkCreateRenderPass(device, &renderPassCreateInfo, allocCb, &renderPass));
    }

    void beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, uint32_t width, uint32_t height, VkFramebuffer framebuffer)
    {
        const VkClearValue clearValues[] = {VkClearValue{0, 0, 0, 1},
                                            VkClearValue{1, 0, 0, 0}};

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffer;
        renderPassBeginInfo.renderArea = {{0, 0}, {width, height}};
        renderPassBeginInfo.clearValueCount = vkfwArraySize(clearValues);
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void createPipelineLayout(VkDevice device, const VkAllocationCallbacks *allocCb, PipelineLayout &pipelineLayout)
    {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pNext = nullptr;
        descriptorSetLayoutCreateInfo.flags = 0;
        descriptorSetLayoutCreateInfo.bindingCount = 1;
        descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

        vkfwCheckVkResult(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, allocCb, &pipelineLayout.descriptorSetLayout));

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = nullptr;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &pipelineLayout.descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        vkfwCheckVkResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, allocCb, &pipelineLayout.handle));
    }

    void initShaderStageCreateInfo(VkShaderModule module, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo &shaderStageCreateInfo)
    {
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.pNext = nullptr;
        shaderStageCreateInfo.flags = 0;
        shaderStageCreateInfo.module = module;
        shaderStageCreateInfo.stage = stage;
        shaderStageCreateInfo.pName = "main";
        shaderStageCreateInfo.pSpecializationInfo = nullptr;
    }

    void createGraphicsPipeline(VkDevice device, const VkAllocationCallbacks *allocCb, VkShaderModule vertModule, VkShaderModule fragModule, VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipeline &pipeline)
    {
        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.pNext = nullptr;
        graphicsPipelineCreateInfo.flags = 0;

        graphicsPipelineCreateInfo.layout = pipelineLayout;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        graphicsPipelineCreateInfo.basePipelineIndex = 0;
        graphicsPipelineCreateInfo.renderPass = renderPass;
        graphicsPipelineCreateInfo.subpass = 0;

        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
        initShaderStageCreateInfo(vertModule, VK_SHADER_STAGE_VERTEX_BIT, shaderStageCreateInfos[0]);
        initShaderStageCreateInfo(fragModule, VK_SHADER_STAGE_FRAGMENT_BIT, shaderStageCreateInfos[1]);
        graphicsPipelineCreateInfo.stageCount = vkfwArraySize(shaderStageCreateInfos);
        graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;

        const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.pNext = nullptr;
        dynamicStateCreateInfo.flags = 0;
        dynamicStateCreateInfo.dynamicStateCount = vkfwArraySize(dynamicStates);
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;
        graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        const VkVertexInputBindingDescription vertexInputBindingDescriptions[] = {VkVertexInputBindingDescription{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};

        const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] = {VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                                                                                      VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
                                                                                      VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}};

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.pNext = nullptr;
        vertexInputStateCreateInfo.flags = 0;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = vkfwArraySize(vertexInputBindingDescriptions);
        vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vkfwArraySize(vertexInputAttributeDescriptions);
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.pNext = nullptr;
        inputAssemblyStateCreateInfo.flags = 0;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

        VkViewport viewport{};
        VkRect2D scissorRect{};

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.pNext = nullptr;
        viewportStateCreateInfo.flags = 0;
        // even though we define viewport and scissor as dynamic states, we need to setup them
        // cause spec states that if the multiple viewports feature is not enabled, viewportCount/scissorCount must be 1
        // source: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkPipelineViewportStateCreateInfo-viewportCount-01216
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissorRect;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.pNext = nullptr;
        rasterizationStateCreateInfo.flags = 0;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0;
        rasterizationStateCreateInfo.depthBiasClamp = 0;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0;
        rasterizationStateCreateInfo.lineWidth = 1;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.pNext = nullptr;
        multisampleStateCreateInfo.flags = 0;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.minSampleShading = 0;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.pNext = nullptr;
        colorBlendStateCreateInfo.flags = 0;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_CLEAR;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
        colorBlendStateCreateInfo.blendConstants[0] = 0;
        colorBlendStateCreateInfo.blendConstants[1] = 0;
        colorBlendStateCreateInfo.blendConstants[2] = 0;
        colorBlendStateCreateInfo.blendConstants[3] = 0;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;

        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.pNext = nullptr;
        depthStencilStateCreateInfo.flags = 0;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.front = {};
        depthStencilStateCreateInfo.back = {};
        depthStencilStateCreateInfo.minDepthBounds = 0;
        depthStencilStateCreateInfo.maxDepthBounds = 0;
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;

        vkfwCheckVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, allocCb, &pipeline));
    }

    void createFramebuffer(VkDevice device, const VkAllocationCallbacks *allocCb, VkRenderPass renderPass, VkImageView colorAttachmentImageView, VkImageView depthStencilImageView, uint32_t width, uint32_t height, VkFramebuffer &framebuffer)
    {
        const VkImageView attachments[] = {colorAttachmentImageView, depthStencilImageView};

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = vkfwArraySize(attachments);
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = width;
        framebufferCreateInfo.height = height;
        framebufferCreateInfo.layers = 1;

        vkfwCheckVkResult(vkCreateFramebuffer(device, &framebufferCreateInfo, allocCb, &framebuffer));
    }

    void createDescriptorPool(VkDevice device, const VkAllocationCallbacks *allocCb, uint32_t descriptorCount, VkDescriptorPool &descriptorPool)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = descriptorCount;

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = 0;
        descriptorPoolCreateInfo.maxSets = descriptorCount;
        descriptorPoolCreateInfo.poolSizeCount = 1;
        descriptorPoolCreateInfo.pPoolSizes = &poolSize;

        vkfwCheckVkResult(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, allocCb, &descriptorPool));
    }

    void allocateDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout, std::vector<VkDescriptorSet> &descriptorSets)
    {
        std::vector<VkDescriptorSetLayout> layouts(descriptorSets.size(), layout);

        VkDescriptorSetAllocateInfo allocateInfo;
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = (uint32_t)descriptorSets.size();
        allocateInfo.pSetLayouts = &layouts[0];

        vkfwCheckVkResult(vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSets[0]))
    }

    VkShaderModule createShaderModule(VkDevice device, const VkAllocationCallbacks *allocCb, const vkfw::FileData &code)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo;
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.pNext = nullptr;
        shaderModuleCreateInfo.flags = 0;
        shaderModuleCreateInfo.codeSize = code.size;
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.value.get());
        VkShaderModule shaderModule;
        vkfwCheckVkResult(vkCreateShaderModule(device, &shaderModuleCreateInfo, allocCb, &shaderModule));
        return shaderModule;
    }

    using FindMemoryTypeCb = std::function<uint32_t(uint32_t, VkMemoryPropertyFlags)>;

    VkDeviceMemory allocateMemory(VkDevice device, const VkAllocationCallbacks *allocCb, const FindMemoryTypeCb &findMemoryTypeCb, VkMemoryPropertyFlags properties, VkMemoryRequirements memoryRequirements)
    {
        VkMemoryAllocateInfo memAllocInfo;
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllocInfo.pNext = nullptr;
        memAllocInfo.allocationSize = memoryRequirements.size;
        memAllocInfo.memoryTypeIndex = findMemoryTypeCb(memoryRequirements.memoryTypeBits, properties);

        VkDeviceMemory deviceMemory;
        vkfwCheckVkResult(vkAllocateMemory(device, &memAllocInfo, allocCb, &deviceMemory));

        return deviceMemory;
    }

    Buffer createBuffer(VkDevice device, const VkAllocationCallbacks *allocCb, const FindMemoryTypeCb &findMemoryTypeCb, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        VkBufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.flags = 0;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices = nullptr;

        Buffer buffer;

        vkfwCheckVkResult(vkCreateBuffer(device, &bufferCreateInfo, allocCb, &buffer.handle));

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer.handle, &memoryRequirements);

        buffer.backingMemory = allocateMemory(device, allocCb, findMemoryTypeCb, properties, memoryRequirements);

        vkfwCheckVkResult(vkBindBufferMemory(device, buffer.handle, buffer.backingMemory, 0));

        return buffer;
    }

    Image createImage(VkDevice device, const VkAllocationCallbacks *allocCb, const FindMemoryTypeCb &findMemoryTypeCb, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        VkImageCreateInfo imageCreateInfo;
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent = {width, height, 1};
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = usage;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        Image image;

        vkfwCheckVkResult(vkCreateImage(device, &imageCreateInfo, allocCb, &image.handle));

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device, image.handle, &memoryRequirements);

        image.backingMemory = allocateMemory(device, allocCb, findMemoryTypeCb, properties, memoryRequirements);

        vkfwCheckVkResult(vkBindImageMemory(device, image.handle, image.backingMemory, 0));

        return image;
    }

    void createImageView(VkDevice device, const VkAllocationCallbacks *allocCb, VkFormat format, VkImage image, VkImageAspectFlags aspectMask, VkImageView &imageView)
    {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        vkfwCheckVkResult(vkCreateImageView(device, &imageViewCreateInfo, allocCb, &imageView));
    }

    void destroyBuffer(VkDevice device, const VkAllocationCallbacks *allocCb, const Buffer &buffer)
    {
        vkDestroyBuffer(device, buffer.handle, allocCb);
        vkFreeMemory(device, buffer.backingMemory, allocCb);
    }

    template <typename data_t>
    void copyToMappedMemory(VkDevice device, const VkAllocationCallbacks *allocCb, VkDeviceMemory memory, const std::vector<data_t> &data)
    {
        data_t *mappedData;
        vkfwCheckVkResult(vkMapMemory(device, memory, 0, sizeof(data_t) * data.size(), 0, (void **)&mappedData));
        if (mappedData != nullptr)
        {
            memcpy(mappedData, &data[0], sizeof(data_t) * data.size());
            vkUnmapMemory(device, memory);
        }
    }

    using ImportContext = std::vector<decltype(vkfw::FileData::value)>;

    void readFileCallback(void *ctx, const char *filename, int is_mtl, const char *obj_filename, char **buf, size_t *len)
    {
        auto *importContext = static_cast<ImportContext *>(ctx);
        auto fileData = vkfw::readFile(filename);
        *buf = fileData.value.get();
        *len = fileData.size;
        importContext->emplace_back(std::move(fileData.value));
    }
    std::unique_ptr<Model> loadModel(VkDevice device, const VkAllocationCallbacks *allocCb, const FindMemoryTypeCb &findMemoryTypeCb, VkCommandBuffer commandBuffer, const std::string &modelPath)
    {
        std::unique_ptr<Model> model;

        tinyobj_attrib_t attributes;
        tinyobj_shape_t *shapes;
        tinyobj_material_t *materials;
        size_t shapeCount, materialCount;
        ImportContext importContext;

        if (tinyobj_parse_obj(&attributes,
                              &shapes,
                              &shapeCount,
                              &materials,
                              &materialCount,
                              modelPath.c_str(),
                              &readFileCallback,
                              (void *)&importContext,
                              TINYOBJ_FLAG_TRIANGULATE) == TINYOBJ_SUCCESS)
        {
            model = std::make_unique<Model>();

            for (size_t i = 0; i < shapeCount; ++i)
            {
                std::vector<Vertex> vertices;

                const auto &shape = shapes[i];

                uint32_t j = 0;
                uint32_t k = shape.face_offset;
                while (j < shape.length)
                {
                    auto vertexCount = attributes.face_num_verts[j++];
                    assert(vertexCount == 3);
                    int l = 0;
                    while (l++ < vertexCount)
                    {
                        Vertex vertex{};
                        auto &face = attributes.faces[k++];
                        if (face.v_idx != (int)0x80000000)
                        {
                            auto *vertices = attributes.vertices + (intptr_t)(face.v_idx * 3);
                            memcpy(vertex.position, vertices, sizeof(float) * 3);
                        }
                        if (face.vn_idx != (int)0x80000000)
                        {
                            auto *normals = attributes.normals + (intptr_t)(face.vn_idx * 3);
                            memcpy(vertex.normal, normals, sizeof(float) * 3);
                        }
                        if (face.vt_idx != (int)0x80000000)
                        {
                            auto *uvs = attributes.texcoords + (intptr_t)(face.vt_idx * 2);
                            memcpy(vertex.uv, uvs, sizeof(float) * 2);
                        }
                        vertices.emplace_back(vertex);
                    }
                }

                std::vector<uint32_t> indices;
                indices.resize(vertices.size());
                std::iota(indices.begin(), indices.end(), 0);

                auto vertexBufferSize = sizeof(Vertex) * vertices.size();
                auto indexBuffer = sizeof(uint32_t) * indices.size();

                Mesh mesh{createBuffer(device, allocCb, findMemoryTypeCb, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                          createBuffer(device, allocCb, findMemoryTypeCb, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                          createBuffer(device, allocCb, findMemoryTypeCb, indexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                          createBuffer(device, allocCb, findMemoryTypeCb, indexBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                          indices.size()};

                copyToMappedMemory(device, allocCb, mesh.stagingVertexBuffer.backingMemory, vertices);
                copyToMappedMemory(device, allocCb, mesh.stagingIndexBuffer.backingMemory, indices);

                VkBufferCopy copyRegion{0, 0, vertexBufferSize};
                vkCmdCopyBuffer(commandBuffer, mesh.stagingVertexBuffer.handle, mesh.vertexBuffer.handle, 1, &copyRegion);

                copyRegion.size = indexBuffer;
                vkCmdCopyBuffer(commandBuffer, mesh.stagingIndexBuffer.handle, mesh.indexBuffer.handle, 1, &copyRegion);

                model->meshes.emplace_back(mesh);
            }
        }

        tinyobj_attrib_free(&attributes);
        tinyobj_shapes_free(shapes, shapeCount);
        tinyobj_materials_free(materials, materialCount);

        return std::move(model);
    }

    void setTranslation(float matrix[16], float vector[3])
    {
        matrix[12] = vector[0];
        matrix[13] = vector[1];
        matrix[14] = vector[2];
    }

    void setPerspective(float matrix[16], float fovY, float aspect, float nearClip, float farClip)
    {
        float bottom = nearClip * tanf((fovY * vkfwDegToRad) * 0.5f);
        float top = -bottom;
        float right = top * aspect;
        float left = -right;

        matrix[0] = 2 * nearClip / (right - left);
        matrix[1] = 0;
        matrix[2] = 0;
        matrix[3] = 0;

        matrix[4] = 0;
        matrix[5] = 2 * nearClip / (top - bottom);
        matrix[6] = 0;
        matrix[7] = 0;

        matrix[8] = 0;
        matrix[9] = 0;
        matrix[10] = -(farClip + nearClip) / (farClip - nearClip);
        matrix[11] = -1;

        matrix[12] = -nearClip * (right + left) / (right - left);
        matrix[13] = -nearClip * (top + bottom) / (top - bottom);
        matrix[14] = 2 * farClip * nearClip / (nearClip - farClip);
        matrix[15] = 0;
    }

    void printUsage()
    {
        std::cout << "obj_loader <path to obj>" << std::endl;
    }

}

ObjLoaderApplication::ObjLoaderApplication() = default;

ObjLoaderApplication::~ObjLoaderApplication() = default;

void ObjLoaderApplication::postInitialize()
{
    createRenderPass(getDevice(), getAllocationCallbacks(), getSwapChainSurfaceFormat().format, gc_depthStencilFormat, m_renderPass);

    createPipelineLayout(getDevice(), getAllocationCallbacks(), m_pipelineLayout);

    m_vertModule = createShaderModule(getDevice(), getAllocationCallbacks(), vkfw::readFile("spirv/lambert.vert.spv"));
    m_fragModule = createShaderModule(getDevice(), getAllocationCallbacks(), vkfw::readFile("spirv/lambert.frag.spv"));
    createGraphicsPipeline(getDevice(), getAllocationCallbacks(), m_vertModule, m_fragModule, m_pipelineLayout.handle, m_renderPass, m_pipeline);

    recreateDepthStencilImageSwapChainImageViewsAndFramebuffers();

    const auto findMemoryTypeCb = std::bind(&ObjLoaderApplication::findMemoryType, this, std::placeholders::_1, std::placeholders::_2);

    m_sceneConstantBuffers.resize(getMaxSimultaneousFrames());
    for (auto &sceneConstantBuffer : m_sceneConstantBuffers)
    {
        sceneConstantBuffer = createBuffer(getDevice(), getAllocationCallbacks(), findMemoryTypeCb, sizeof(SceneConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    createDescriptorPool(getDevice(), getAllocationCallbacks(), getMaxSimultaneousFrames(), m_descriptorPool);
    m_descriptorSets.resize(getMaxSimultaneousFrames());
    allocateDescriptorSets(getDevice(), m_descriptorPool, m_pipelineLayout.descriptorSetLayout, m_descriptorSets);

    for (uint32_t i = 0; i < getMaxSimultaneousFrames(); ++i)
    {
        VkDescriptorBufferInfo descriptorBufferInfo;
        descriptorBufferInfo.buffer = m_sceneConstantBuffers[i].handle;
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(SceneConstants);

        VkWriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.pNext = nullptr;
        writeDescriptorSet.dstSet = m_descriptorSets[i];
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
        writeDescriptorSet.pImageInfo = nullptr;
        writeDescriptorSet.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(getDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }
}

bool ObjLoaderApplication::preRun(int argc, char **argv)
{
#ifdef _DEBUG
    if (argc < 2)
    {
        m_modelPath = "models/bunny.obj";
    }
    else
#else
    if (argc < 2)
    {
        printUsage();
        return false;
    }
#endif
    {
        m_modelPath = argv[1];
    }

    return true;
}

void ObjLoaderApplication::postRun()
{
    if (m_model != nullptr)
    {
        for (auto &mesh : m_model->meshes)
        {
            destroyBuffer(getDevice(), getAllocationCallbacks(), mesh.vertexBuffer);
            destroyBuffer(getDevice(), getAllocationCallbacks(), mesh.stagingVertexBuffer);
            destroyBuffer(getDevice(), getAllocationCallbacks(), mesh.indexBuffer);
            destroyBuffer(getDevice(), getAllocationCallbacks(), mesh.stagingIndexBuffer);
        }
        m_model->meshes.clear();
    }

    for (auto &sceneConstantBuffer : m_sceneConstantBuffers)
    {
        destroyBuffer(getDevice(), getAllocationCallbacks(), sceneConstantBuffer);
    }
    m_sceneConstantBuffers.clear();

    destroyDepthStencilImageSwapChainImageViewsAndFramebuffers();

    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(getDevice(), m_pipeline, getAllocationCallbacks());
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_fragModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(getDevice(), m_fragModule, getAllocationCallbacks());
        m_fragModule = VK_NULL_HANDLE;
    }
    if (m_vertModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(getDevice(), m_vertModule, getAllocationCallbacks());
        m_vertModule = VK_NULL_HANDLE;
    }
    if (m_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(getDevice(), m_descriptorPool, getAllocationCallbacks());
    }
    vkDestroyPipelineLayout(getDevice(), m_pipelineLayout.handle, getAllocationCallbacks());
    vkDestroyDescriptorSetLayout(getDevice(), m_pipelineLayout.descriptorSetLayout, getAllocationCallbacks());
    m_pipelineLayout = {};
    if (m_renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(getDevice(), m_renderPass, getAllocationCallbacks());
        m_renderPass = VK_NULL_HANDLE;
    }
}

void ObjLoaderApplication::postResize(uint32_t width, uint32_t height)
{
    recreateDepthStencilImageSwapChainImageViewsAndFramebuffers();
}

void ObjLoaderApplication::update()
{
    setTranslation(m_sceneConstants.view, m_cameraPosition);
    setPerspective(m_sceneConstants.projection, 60, getWidth() / (float)getHeight(), 0.1f, 100);
}

void ObjLoaderApplication::record(VkCommandBuffer commandBuffer)
{
    copyToMappedMemory<SceneConstants>(getDevice(), getAllocationCallbacks(), m_sceneConstantBuffers[getCurrentFrame()].backingMemory, {m_sceneConstants});

    if (m_model == nullptr)
    {
        const auto findMemoryTypeCb = std::bind(&ObjLoaderApplication::findMemoryType, this, std::placeholders::_1, std::placeholders::_2);

        if ((m_model = loadModel(getDevice(), getAllocationCallbacks(), findMemoryTypeCb, commandBuffer, m_modelPath)) == nullptr)
        {
            vkfw::fail("failed to load %s", m_modelPath.c_str());
        }

        VkImageMemoryBarrier imageMemoryBarrier;
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = nullptr;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier.srcQueueFamilyIndex = getGraphicsQueueFamilyIndex();
        imageMemoryBarrier.dstQueueFamilyIndex = getPresentQueueFamilyIndex();
        imageMemoryBarrier.image = getSwapChainImage(getSwapChainIndex());
        imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }
    else
    {
        const VkDeviceSize offsets[] = {0};

        beginRenderPass(commandBuffer, m_renderPass, getWidth(), getHeight(), m_framebuffers[getSwapChainIndex()]);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        VkViewport viewport{0, 0, (float)getWidth(), (float)getHeight(), 0, 1};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissorRect{0, 0, getWidth(), getHeight()};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.handle, 0, 1, &m_descriptorSets[getCurrentFrame()], 0, nullptr);

        for (const auto &mesh : m_model->meshes)
        {
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.vertexBuffer.handle, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, (uint32_t)mesh.indexCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);
    }
}

void ObjLoaderApplication::recreateDepthStencilImageSwapChainImageViewsAndFramebuffers()
{
    destroyDepthStencilImageSwapChainImageViewsAndFramebuffers();

    const auto findMemoryTypeCb = std::bind(&ObjLoaderApplication::findMemoryType, this, std::placeholders::_1, std::placeholders::_2);

    m_depthStencilImages.resize(getSwapChainCount());
    m_depthStencilImageViews.resize(getSwapChainCount());
    m_swapChainImageViews.resize(getSwapChainCount());
    m_framebuffers.resize(getSwapChainCount());
    for (uint32_t i = 0; i < getSwapChainCount(); ++i)
    {
        m_depthStencilImages[i] = createImage(getDevice(), getAllocationCallbacks(), findMemoryTypeCb, gc_depthStencilFormat, getWidth(), getHeight(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        createImageView(getDevice(), getAllocationCallbacks(), gc_depthStencilFormat, m_depthStencilImages[i].handle, VK_IMAGE_ASPECT_DEPTH_BIT, m_depthStencilImageViews[i]);
        createImageView(getDevice(), getAllocationCallbacks(), getSwapChainSurfaceFormat().format, getSwapChainImage(i), VK_IMAGE_ASPECT_COLOR_BIT, m_swapChainImageViews[i]);
        createFramebuffer(getDevice(), getAllocationCallbacks(), m_renderPass, m_swapChainImageViews[i], m_depthStencilImageViews[i], getWidth(), getHeight(), m_framebuffers[i]);
    }
}

void ObjLoaderApplication::destroyDepthStencilImageSwapChainImageViewsAndFramebuffers()
{
    for (auto &framebuffer : m_framebuffers)
    {
        vkDestroyFramebuffer(getDevice(), framebuffer, getAllocationCallbacks());
    }
    m_framebuffers.clear();

    for (auto &swapChainImageView : m_swapChainImageViews)
    {
        vkDestroyImageView(getDevice(), swapChainImageView, getAllocationCallbacks());
    }
    m_swapChainImageViews.clear();

    for (auto &depthStencilImage : m_depthStencilImages)
    {
        vkDestroyImage(getDevice(), depthStencilImage.handle, getAllocationCallbacks());
        vkFreeMemory(getDevice(), depthStencilImage.backingMemory, getAllocationCallbacks());
    }
    m_depthStencilImages.clear();

    for (auto &depthStencilImageView : m_depthStencilImageViews)
    {
        vkDestroyImageView(getDevice(), depthStencilImageView, getAllocationCallbacks());
    }
    m_depthStencilImageViews.clear();
}

void ObjLoaderApplication::keyDown(uint32_t keyCode)
{
    const float c_speed = 0.01f;

    if (keyCode == vkfwKeyUp)
    {
        m_cameraPosition[2] += c_speed;
    }
    else if (keyCode == vkfwKeyDown)
    {
        m_cameraPosition[2] -= c_speed;
    }
    else if (keyCode == vkfwKeyLeft)
    {
        m_cameraPosition[0] -= c_speed;
    }
    else if (keyCode == vkfwKeyRight)
    {
        m_cameraPosition[0] += c_speed;
    }
}