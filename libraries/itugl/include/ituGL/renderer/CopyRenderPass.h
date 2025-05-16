#pragma once

#include <ituGL/renderer/RenderPass.h>
#include <ituGL/texture/Texture2DObject.h>
#include <memory>

class Material;
class FramebufferObject;

class CopyRenderPass : public RenderPass
{
public:
    CopyRenderPass(std::shared_ptr<Texture2DObject> sourceTexture, std::shared_ptr<Texture2DObject> targetTexture);

    void Render() override;

private:
    void InitFramebuffer(std::shared_ptr<Texture2DObject> targetTexture);
    std::shared_ptr<Material> InitMaterial(std::shared_ptr<Texture2DObject> sourceTexture);

    std::shared_ptr<Material> m_material;
    std::shared_ptr<FramebufferObject> m_framebuffer;
};
