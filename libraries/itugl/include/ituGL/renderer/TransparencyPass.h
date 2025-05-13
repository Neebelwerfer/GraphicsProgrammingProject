#pragma once

#include <ituGL/renderer/RenderPass.h>

class TransparencyPass : public RenderPass
{
public:
    TransparencyPass(std::shared_ptr<const FramebufferObject> framebuffer);
    TransparencyPass(std::shared_ptr<const FramebufferObject> framebuffer, int drawcallCollectionIndex);

    void Render() override;

private:
    int m_drawcallCollectionIndex;
};
