#include <ituGL/renderer/CopyRenderPass.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/shader/Material.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/asset/ShaderLoader.h>
#include <glm/gtx/transform.hpp>



CopyRenderPass::CopyRenderPass(std::shared_ptr<Texture2DObject> sourceTexture, std::shared_ptr<Texture2DObject> targetTexture)
    : m_material(InitMaterial(sourceTexture))
{
    InitFramebuffer(targetTexture);
}



void CopyRenderPass::Render()
{
    Renderer& renderer = GetRenderer();

    assert(m_material);
    m_material->Use();
    const Mesh* mesh = &renderer.GetFullscreenMesh();

    mesh->DrawSubmesh(0);
}


void CopyRenderPass::InitFramebuffer(std::shared_ptr<Texture2DObject> targetTexture)
{
    std::shared_ptr<FramebufferObject> framebuffer = std::make_shared<FramebufferObject>();
    framebuffer->Bind();
    framebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *targetTexture);
    framebuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));

    m_targetFramebuffer = framebuffer;
    FramebufferObject::Unbind();
}

std::shared_ptr<Material> CopyRenderPass::InitMaterial(std::shared_ptr<Texture2DObject> sourceTexture)
{
    // We could keep this vertex shader and reuse it, but it looks simpler this way
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/postfx/copy.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Create material
    std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr);
    material->SetUniformValue("SourceTexture", sourceTexture);

    return material;
}
