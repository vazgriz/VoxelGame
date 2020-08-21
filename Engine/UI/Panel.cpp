#include <Engine/UI/Panel.h>
#include <Engine/UI/PanelRenderer.h>

using namespace VoxelEngine;
using namespace VoxelEngine::UI;

Panel::Panel(entt::registry& registry, entt::entity id, PanelRenderer* renderer)
    : Element(registry, id)
{
    m_renderer = renderer;
    m_color = {};
}

void Panel::render(Canvas& canvas, vk::CommandBuffer& commandBuffer) {
    m_renderer->render(commandBuffer, canvas, m_id);
}