#include "Engine/RenderGraph/AcquireNode.h"

using namespace VoxelEngine;

AcquireNode::AcquireNode(VoxelEngine::Engine& engine, RenderGraph& graph)
    : RenderGraph::Node(graph, *engine.getGraphics().graphicsQueue(), vk::PipelineStageFlags::TopOfPipe) {
    m_swapchain = &engine.getGraphics().swapchain();

    vk::SemaphoreCreateInfo info = {};
    m_semaphore = std::make_unique<vk::Semaphore>(m_swapchain->device(), info);

    addExternalWait(*m_semaphore, vk::PipelineStageFlags::ColorAttachmentOutput);
}

void AcquireNode::preRender(uint32_t currentFrame) {
    m_swapchain->acquireNextImage(-1, m_semaphore.get(), nullptr, m_swapchainIndex);
}