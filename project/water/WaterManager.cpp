#include "WaterManager.h"
#include <imgui.h>
#include <ituGL/geometry/Mesh.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/Texture2DLoader.h>
#include <ituGL/asset/ModelLoader.h>

WaterManager::WaterManager(Renderer& renderer, float& time)
    : m_colour(0.0f)
    , m_jump(0.0f, 0.0f)
    , m_tiling(3)
    , m_speed(0.5f)
    , m_flowStrength(0.25f)
    , m_flowOffset(0.0f)
{
    InitializeWaterMaterial(renderer, time);
    LoadModel();
}

void WaterManager::RenderGUI(DearImGui& imgui)
{
    if (ImGui::ColorEdit3("Colour", &m_colour[0]))
        m_waterMaterial->SetUniformValue("Color", m_colour);

    if (ImGui::DragFloat2("Jump", &m_jump[0], 0.01f, -0.25f, 0.25f))
        m_waterMaterial->SetUniformValue("Jump", m_jump);

    if (ImGui::DragInt("Tiling", &m_tiling, 1, 1, 100))
        m_waterMaterial->SetUniformValue("Tiling", m_tiling);

    if (ImGui::DragFloat("Speed", &m_speed, 0.1f, 0.0f, 100.f))
        m_waterMaterial->SetUniformValue("Speed", m_speed);

    if (ImGui::DragFloat("FlowStrength", &m_flowStrength, 0.1f, 0.0f, 100.f))
        m_waterMaterial->SetUniformValue("FlowStrength", m_flowStrength);

    if (ImGui::DragFloat("FlowOffset", &m_flowOffset, 0.05f, -1.0f, 1.f))
        m_waterMaterial->SetUniformValue("FlowOffset", m_flowOffset);
}

WaterManager::~WaterManager()
{
}

void WaterManager::InitializeWaterMaterial(Renderer& renderer, float& time)
{
    // Load and build shader
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/default.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/water.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Get transform related uniform locations
    ShaderProgram::Location worldViewMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewMatrix");
    ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");
    ShaderProgram::Location timeLocation = shaderProgramPtr->GetUniformLocation("ElapsedTime");

    // Register shader with renderer
    renderer.RegisterShaderProgram(shaderProgramPtr,
        [=, &time](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            shaderProgram.SetUniform(worldViewMatrixLocation, camera.GetViewMatrix() * worldMatrix);
            shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
            shaderProgram.SetUniform(timeLocation, time);
        },
        nullptr
    );

    // Filter out uniforms that are not material properties
    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("WorldViewMatrix");
    filteredUniforms.insert("WorldViewProjMatrix");
    filteredUniforms.insert("ElapsedTime");

    Texture2DLoader textureLoader;
    textureLoader.SetFlipVertical(true);
    std::shared_ptr<Texture2DObject> albedoMap = textureLoader.LoadTextureShared("models/water/water.png", TextureObject::FormatRGB, TextureObject::InternalFormat::InternalFormatRGB16F);
    std::shared_ptr<Texture2DObject> flowMap = textureLoader.LoadTextureShared("models/water/flowmap.png", TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA16F);
    std::shared_ptr<Texture2DObject> normalMap = textureLoader.LoadTextureShared("models/water/NormalMap.png", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);

    // Create material
    std::shared_ptr waterMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    waterMaterial->SetUniformValue("Color", m_colour);
    waterMaterial->SetUniformValue("ColorTexture", albedoMap);
    waterMaterial->SetUniformValue("NormalTexture", normalMap);
    waterMaterial->SetUniformValue("FlowTexture", flowMap);
    waterMaterial->SetUniformValue("Jump", m_jump);
    waterMaterial->SetUniformValue("Tiling", m_tiling);
    waterMaterial->SetUniformValue("Speed", m_speed);
    waterMaterial->SetUniformValue("FlowStrength", m_flowStrength);
    waterMaterial->SetUniformValue("FlowOffset", m_flowOffset);
    m_waterMaterial = waterMaterial;
}

void WaterManager::LoadModel()
{
    ModelLoader waterLoader(m_waterMaterial);
    //waterLoader.SetCreateMaterials(true);

    // Link vertex properties to attributes
    waterLoader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
    waterLoader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
    waterLoader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
    waterLoader.SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
    waterLoader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");

    // Link material properties to uniforms
    m_waterPlane = waterLoader.LoadShared("models/water/water_plane.obj");
    m_waterPlane->SetMaterial(0, m_waterMaterial);
}

const std::shared_ptr<Model> WaterManager::GetWaterPlane()
{
    return m_waterPlane;
}