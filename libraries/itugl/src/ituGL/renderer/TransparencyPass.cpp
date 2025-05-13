#include <ituGL/renderer/TransparencyPass.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/VertexArrayObject.h>
#include <ituGL/renderer/Renderer.h>

TransparencyPass::TransparencyPass(std::shared_ptr<const FramebufferObject> framebuffer)
    : TransparencyPass(framebuffer, 0)
{
}

TransparencyPass::TransparencyPass(std::shared_ptr<const FramebufferObject> framebuffer, int drawcallCollectionIndex)
    : m_drawcallCollectionIndex(drawcallCollectionIndex)
{
    m_targetFramebuffer = framebuffer;
}

void TransparencyPass::Render()
{
    Renderer& renderer = GetRenderer();

    const Camera& camera = renderer.GetCurrentCamera();
    const auto& lights = renderer.GetLights();
    const auto& drawcallCollection = renderer.GetDrawcalls(m_drawcallCollectionIndex);
    const bool blendEnabled = renderer.GetDevice().IsFeatureEnabled(GL_BLEND);

    // for all drawcalls
    for (const Renderer::DrawcallInfo& drawcallInfo : drawcallCollection)
    {
        if (!drawcallInfo.material.GetTransparency())
            continue;

        // Prepare drawcall states
        renderer.PrepareDrawcall(drawcallInfo);

        std::shared_ptr<const ShaderProgram> shaderProgram = drawcallInfo.material.GetShaderProgram();


        auto forwardPass = shaderProgram->GetUniformLocation("ForwardPass");
        shaderProgram->SetUniform(forwardPass, 1);

        //for all lights
        bool first = true;
        unsigned int lightIndex = 0;
 
        // Hacky way to ensure that we have blend enabled
        // but can allow that since all objects getting rendered this pass is transparent
        renderer.GetDevice().EnableFeature(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        while (renderer.UpdateLights(shaderProgram, lights, lightIndex))
        {

            // Draw
            drawcallInfo.drawcall.Draw();

            first = false;
        }
        shaderProgram->SetUniform(forwardPass, 0);
    }
    renderer.GetDevice().SetFeatureEnabled(GL_BLEND, blendEnabled);
}
