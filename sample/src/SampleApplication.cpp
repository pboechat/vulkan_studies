#include "SampleApplication.h"

#include <vkfw/vkfw.h>

#include <cassert>

void createRenderPass(VkDevice device, const VkAllocationCallbacks *allocCb, VkFormat swapChainFormat, VkRenderPass &renderPass)
{
    VkAttachmentDescription colorAttachmentDescription;
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = swapChainFormat;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference;
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = nullptr;
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
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;

    vkfwCheckVkResult(vkCreateRenderPass(device, &renderPassCreateInfo, allocCb, &renderPass));
}

void createPipelineLayout(VkDevice device, const VkAllocationCallbacks *allocCb, VkPipelineLayout &pipelineLayout)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    vkfwCheckVkResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, allocCb, &pipelineLayout));
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

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
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

    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;

    vkfwCheckVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, allocCb, &pipeline));
}

void createImageView(VkDevice device, const VkAllocationCallbacks *allocCb, VkFormat format, VkImage image, VkImageView &imageView)
{
    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    vkfwCheckVkResult(vkCreateImageView(device, &imageViewCreateInfo, allocCb, &imageView));
}

void createFramebuffer(VkDevice device, const VkAllocationCallbacks *allocCb, VkRenderPass renderPass, VkImageView imageView, uint32_t width, uint32_t height, VkFramebuffer &framebuffer)
{
    VkFramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = nullptr;
    framebufferCreateInfo.flags = 0;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.pAttachments = &imageView;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    vkfwCheckVkResult(vkCreateFramebuffer(device, &framebufferCreateInfo, allocCb, &framebuffer));
}

void SampleApplication::postInitialize()
{
    createRenderPass(getDevice(), getAllocationCallbacks(), getSwapChainSurfaceFormat().format, m_renderPass);

    createPipelineLayout(getDevice(), getAllocationCallbacks(), m_pipelineLayout);

    m_vertModule = createShaderModule(vkfw::readFile("spirv/triangle.vert.spv"));
    m_fragModule = createShaderModule(vkfw::readFile("spirv/triangle.frag.spv"));
    createGraphicsPipeline(getDevice(), getAllocationCallbacks(), m_vertModule, m_fragModule, m_pipelineLayout, m_renderPass, m_pipeline);

    recreateSwapChainImageViewsAndFramebuffers();
}

void SampleApplication::onStop()
{
    destroySwapChainImageViewsAndFramebuffers();
    
    if (m_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(getDevice(), m_pipeline, getAllocationCallbacks());
    }
    destroyShaderModule(m_fragModule);
    destroyShaderModule(m_vertModule);
    if (m_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(getDevice(), m_pipelineLayout, getAllocationCallbacks());
    }
    if (m_renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(getDevice(), m_renderPass, getAllocationCallbacks());
    }
}

void SampleApplication::onResize(uint32_t width, uint32_t height)
{
    recreateSwapChainImageViewsAndFramebuffers();
}

void beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, uint32_t width, uint32_t height, VkFramebuffer framebuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea = {{0, 0}, {width, height}};
    renderPassBeginInfo.clearValueCount = 1;
    VkClearValue clearValue = {0, 0, 0, 1};
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void SampleApplication::record(VkCommandBuffer commandBuffer)
{
    beginRenderPass(commandBuffer, m_renderPass, getWidth(), getHeight(), m_framebuffers[getSwapChainIndex()]);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{0, 0, (float)getWidth(), (float)getHeight(), 0, 1};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissorRect{0, 0, getWidth(), getHeight()};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

VkShaderModule SampleApplication::createShaderModule(const std::vector<char> &code) const
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    vkfwCheckVkResult(vkCreateShaderModule(getDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule));
    return shaderModule;
}

void SampleApplication::destroyShaderModule(VkShaderModule shaderModule) const
{
    if (shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(getDevice(), shaderModule, nullptr);
    }
}

void SampleApplication::recreateSwapChainImageViewsAndFramebuffers()
{
    destroySwapChainImageViewsAndFramebuffers();

    m_swapChainImageViews.resize(getSwapChainCount());
    m_framebuffers.resize(getSwapChainCount());
    for (uint32_t i = 0; i < getSwapChainCount(); ++i)
    {
        createImageView(getDevice(), getAllocationCallbacks(), getSwapChainSurfaceFormat().format, getSwapChainImage(i), m_swapChainImageViews[i]);
        createFramebuffer(getDevice(), getAllocationCallbacks(), m_renderPass, m_swapChainImageViews[i], getWidth(), getHeight(), m_framebuffers[i]);
    }
}

void SampleApplication::destroySwapChainImageViewsAndFramebuffers()
{
    for (auto &framebuffer : m_framebuffers)
    {
        vkDestroyFramebuffer(getDevice(), framebuffer, getAllocationCallbacks());
    }

    for (auto &swapChainImageView : m_swapChainImageViews)
    {
        vkDestroyImageView(getDevice(), swapChainImageView, getAllocationCallbacks());
    }
}