#include <ituGL/renderer/GBufferCopyPass.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/shader/Material.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/camera/Camera.h>
#include <glm/gtx/transform.hpp>


GBufferCopyPass::GBufferCopyPass(std::shared_ptr<Material> material, std::shared_ptr<const FramebufferObject> framebuffer)
    : RenderPass(framebuffer), m_material(material)
{
}

void GBufferCopyPass::Render()
{
    Renderer& renderer = GetRenderer();

    assert(m_material);
    m_material->Use();
    const Mesh* mesh = &renderer.GetFullscreenMesh();

    // Set to always so we write directly to depth buffer
    glDepthFunc(GL_ALWAYS);
    
    mesh->DrawSubmesh(0);

    // Reset depth function to default
    glDepthFunc(GL_LESS);
}
