#include <ituGL/renderer/PostFXRenderPass.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/shader/Material.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/camera/Camera.h>
#include <glm/gtx/transform.hpp>


PostFXRenderPass::PostFXRenderPass(std::shared_ptr<Material> material, std::shared_ptr<const FramebufferObject> framebuffer)
    : RenderPass(framebuffer), m_material(material)
{
}

void PostFXRenderPass::Render()
{
    Renderer& renderer = GetRenderer();

    assert(m_material);
    m_material->Use();
    const Mesh* mesh = &renderer.GetFullscreenMesh();

    //Lets make sure to update the camera projections TODO: Make another render pass class for this
    const Camera& camera = renderer.GetCurrentCamera();
    glm::mat4 fullscreenMatrix = glm::inverse(camera.GetViewProjectionMatrix());
    renderer.UpdateTransforms(m_material->GetShaderProgram(), fullscreenMatrix, true);

    mesh->DrawSubmesh(0);
}
