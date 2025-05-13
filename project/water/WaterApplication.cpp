#include "WaterApplication.h"

#include <ituGL/asset/TextureCubemapLoader.h>
#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/ModelLoader.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/scene/SceneCamera.h>

#include <ituGL/lighting/DirectionalLight.h>
#include <ituGL/lighting/PointLight.h>
#include <ituGL/lighting/SpotLight.h>
#include <ituGL/scene/SceneLight.h>

#include <ituGL/shader/ShaderUniformCollection.h>
#include <ituGL/shader/Material.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/scene/SceneModel.h>
#include <ituGL/scene/Transform.h>

#include <ituGL/renderer/SkyboxRenderPass.h>
#include <ituGL/renderer/GBufferRenderPass.h>
#include <ituGL/renderer/DeferredRenderPass.h>
#include <ituGL/renderer/GBufferCopyPass.h>
#include <ituGL/renderer/PostFXRenderPass.h>
#include <ituGL/renderer/TransparencyPass.h>
#include <ituGL/scene/RendererSceneVisitor.h>

#include <ituGL/scene/ImGuiSceneVisitor.h>
#include <imgui.h>

const float _MaxPlaytime = 60;

WaterApplication::WaterApplication()
    : Application(1248, 1248, "Water Scene")
    , m_renderer(GetDevice())
    , m_mainSceneFramebuffer(std::make_shared<FramebufferObject>())
    , m_reflectionBuffer(std::make_shared<FramebufferObject>())
    , m_fullSceneFramebuffer(std::make_shared<FramebufferObject>())
    , m_play(false)
    , m_timeElapsed(0)
    , m_showType(0)
    , m_maxDistance(20)
    , m_resolution(0.7)
    , m_steps(15)
    , m_thickness(0.85)
    , m_blurIterations(5)
{
}

void WaterApplication::Initialize()
{
    Application::Initialize();

    // Initialize DearImGUI
    m_imGui.Initialize(GetMainWindow());
    m_waterManager = std::make_shared<WaterManager>(m_renderer, m_timeElapsed);

    InitializeCamera();
    InitializeLights();
    InitializeMaterials();
    InitializeModels();
    InitializeRenderer();

}

void WaterApplication::Update()
{
    Application::Update();

    // Update camera controller
    m_cameraController.Update(GetMainWindow(), GetDeltaTime());

    // Add the scene nodes to the renderer
    RendererSceneVisitor rendererSceneVisitor(m_renderer);
    m_scene.AcceptVisitor(rendererSceneVisitor);

    if(m_play)
        m_timeElapsed += GetDeltaTime();

    if (m_timeElapsed > _MaxPlaytime)
        m_timeElapsed = 0.0f;
    
}

void WaterApplication::Render()
{
    Application::Render();

    GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

    // Render the scene
    m_renderer.Render();

    // Render the debug user interface
    RenderGUI();
}

void WaterApplication::Cleanup()
{
    // Cleanup DearImGUI
    m_imGui.Cleanup();

    Application::Cleanup();
}

void WaterApplication::InitializeCamera()
{
    // Create the main camera
    std::shared_ptr<Camera> camera = std::make_shared<Camera>();
    camera->SetViewMatrix(glm::vec3(-2, 1, -2), glm::vec3(0, 0.5f, 0), glm::vec3(0, 1, 0));
    camera->SetPerspectiveProjectionMatrix(1.0f, 1.0f, 0.1f, 100.0f);

    // Create a scene node for the camera
    std::shared_ptr<SceneCamera> sceneCamera = std::make_shared<SceneCamera>("camera", camera);
    std::shared_ptr<Transform> cameraTransform = sceneCamera->GetTransform();
    cameraTransform->SetRotation(glm::vec3(-0.46, -1.6, 0));
    cameraTransform->SetTranslation(glm::vec3(-18, 9, -2));

    // Add the camera node to the scene
    m_scene.AddSceneNode(sceneCamera);

    // Set the camera scene node to be controlled by the camera controller
    m_cameraController.SetCamera(sceneCamera);
}

void WaterApplication::InitializeLights()
{
    // Create a directional light and add it to the scene
    std::shared_ptr<DirectionalLight> directionalLight = std::make_shared<DirectionalLight>();
    directionalLight->SetDirection(glm::vec3(0.0f, -1.0f, -0.3f)); // It will be normalized inside the function
    directionalLight->SetIntensity(3.0f);
    m_scene.AddSceneNode(std::make_shared<SceneLight>("directional light", directionalLight));


    std::shared_ptr<SpotLight> spotLight = std::make_shared<SpotLight>();
    spotLight->SetPosition(glm::vec3(0, 5, 0));
    spotLight->SetDirection(glm::vec3(0.0f, -1.0f, 0.0f));
    spotLight->SetIntensity(5);
    spotLight->SetDistanceAttenuation(glm::vec2(5.0f, 10.0f));
    spotLight->SetAngleAttenuation(glm::vec2(0.1f, 0.4f));
    m_scene.AddSceneNode(std::make_shared<SceneLight>("spot light", spotLight));

    // Create a point light and add it to the scene
    //std::shared_ptr<PointLight> pointLight = std::make_shared<PointLight>();
    //pointLight->SetPosition(glm::vec3(0, 0, 0));
    //pointLight->SetDistanceAttenuation(glm::vec2(5.0f, 10.0f));
    //m_scene.AddSceneNode(std::make_shared<SceneLight>("point light", pointLight));
}

void WaterApplication::InitializeMaterials()
{
    // G-buffer material
    {
        // Load and build shader
        std::vector<const char*> vertexShaderPaths;
        vertexShaderPaths.push_back("shaders/version330.glsl");
        vertexShaderPaths.push_back("shaders/default.vert");
        Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

        std::vector<const char*> fragmentShaderPaths;
        fragmentShaderPaths.push_back("shaders/version330.glsl");
        fragmentShaderPaths.push_back("shaders/utils.glsl");
        fragmentShaderPaths.push_back("shaders/default.frag");
        Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

        std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
        shaderProgramPtr->Build(vertexShader, fragmentShader);

        // Get transform related uniform locations
        ShaderProgram::Location worldViewMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewMatrix");
        ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

        // Register shader with renderer
        m_renderer.RegisterShaderProgram(shaderProgramPtr,
            [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
            {
                shaderProgram.SetUniform(worldViewMatrixLocation, camera.GetViewMatrix() * worldMatrix);
                shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
            },
            nullptr
        );

        // Filter out uniforms that are not material properties
        ShaderUniformCollection::NameSet filteredUniforms;
        filteredUniforms.insert("WorldViewMatrix");
        filteredUniforms.insert("WorldViewProjMatrix");

        // Create material
        m_defaultMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
        m_defaultMaterial->SetUniformValue("Color", glm::vec3(1.0f));
        m_defaultMaterial->SetUniformValue("HaveTextures", glm::vec3(0));
    }

    // Deferred material
    {
        std::vector<const char*> vertexShaderPaths;
        vertexShaderPaths.push_back("shaders/version330.glsl");
        vertexShaderPaths.push_back("shaders/renderer/deferred.vert");
        Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

        std::vector<const char*> fragmentShaderPaths;
        fragmentShaderPaths.push_back("shaders/version330.glsl");
        fragmentShaderPaths.push_back("shaders/utils.glsl");
        fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
        fragmentShaderPaths.push_back("shaders/lighting.glsl");
        fragmentShaderPaths.push_back("shaders/renderer/deferred.frag");
        Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

        std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
        shaderProgramPtr->Build(vertexShader, fragmentShader);

        // Filter out uniforms that are not material properties
        ShaderUniformCollection::NameSet filteredUniforms;
        filteredUniforms.insert("InvViewMatrix");
        filteredUniforms.insert("InvProjMatrix");
        filteredUniforms.insert("WorldViewProjMatrix");
        filteredUniforms.insert("LightIndirect");
        filteredUniforms.insert("LightColor");
        filteredUniforms.insert("LightPosition");
        filteredUniforms.insert("LightDirection"); 
        filteredUniforms.insert("LightAttenuation");

        // Get transform related uniform locations
        ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
        ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
        ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

        // Register shader with renderer
        m_renderer.RegisterShaderProgram(shaderProgramPtr,
            [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
            {
                if (cameraChanged)
                {
                    shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
                    shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
                }
                shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
            },
            m_renderer.GetDefaultUpdateLightsFunction(*shaderProgramPtr)
        );

        // Create material
        m_deferredMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    }
}

void WaterApplication::InitializeModels()
{
    m_skyboxTexture = TextureCubemapLoader::LoadTextureShared("models/skybox/puresky.hdr", TextureObject::FormatRGB, TextureObject::InternalFormatRGB16F);

    m_skyboxTexture->Bind();
    float maxLod;
    m_skyboxTexture->GetParameter(TextureObject::ParameterFloat::MaxLod, maxLod);
    TextureCubemapObject::Unbind();

    // Set the environment texture on the deferred material
    m_deferredMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
    m_deferredMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

    // Configure loader
    ModelLoader loader(m_defaultMaterial);

    // Create a new material copy for each submaterial
    loader.SetCreateMaterials(true);

    // Flip vertically textures loaded by the model loader
    loader.GetTexture2DLoader().SetFlipVertical(true);

    // Link vertex properties to attributes
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Bitangent, "VertexBitangent");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");

    // Link material properties to uniforms
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseColor, "Color");
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::DiffuseTexture, "ColorTexture");
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::NormalTexture, "NormalTexture");
    loader.SetMaterialProperty(ModelLoader::MaterialProperty::SpecularTexture, "SpecularTexture");

    // Load models
    std::shared_ptr<Model> lightHouse = loader.LoadShared("models/Lighthouse/LighthouseScaled.obj");
    auto lightHouseSceneModel = std::make_shared<SceneModel>("LightHouse", lightHouse);
    lightHouseSceneModel->GetTransform()->SetTranslation(glm::vec3(0.0, -0.5, 0.0));

    std::shared_ptr<Model> underwaterModel = loader.LoadShared("models/UnderwaterScene/underwater.obj");

    m_scene.AddSceneNode(lightHouseSceneModel);
    m_scene.AddSceneNode(std::make_shared<SceneModel>("Under Water", underwaterModel));

    auto waterPlane = std::make_shared<SceneModel>("Water", m_waterManager->GetWaterPlane());
    auto trans = waterPlane->GetTransform();
    trans->SetScale(glm::vec3(10, 1, 10));
    m_scene.AddSceneNode(waterPlane);
}

void WaterApplication::InitializeFramebuffers()
{
    int width, height;
    GetMainWindow().GetDimensions(width, height);

    // Scene Texture
    m_sceneTexture = std::make_shared<Texture2DObject>();
    m_sceneTexture->Bind();
    m_sceneTexture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatSRGBA8);
    m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
    m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);

    // Scene framebuffer
    m_mainSceneFramebuffer->Bind();
    m_mainSceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Depth, *m_depthTexture);
    m_mainSceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_sceneTexture);
    m_mainSceneFramebuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));

    m_reflectiveColorTexture = std::make_shared<Texture2DObject>();
    m_reflectiveColorTexture->Bind();
    m_reflectiveColorTexture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatSRGBA8);
    m_reflectiveColorTexture->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
    m_reflectiveColorTexture->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);

    m_reflectionBuffer->Bind();
    m_reflectionBuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_reflectiveColorTexture);
    m_reflectionBuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));

    for (int i = 0; i < m_tempFramebuffers.size(); ++i)
    {
        m_tempTextures[i] = std::make_shared<Texture2DObject>();
        m_tempTextures[i]->Bind();
        m_tempTextures[i]->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatSRGBA8);
        m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::WrapS, GL_CLAMP_TO_EDGE);
        m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::WrapT, GL_CLAMP_TO_EDGE);
        m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
        m_tempTextures[i]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);

        m_tempFramebuffers[i] = std::make_shared<FramebufferObject>();
        m_tempFramebuffers[i]->Bind();
        m_tempFramebuffers[i]->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_tempTextures[i]);
        m_tempFramebuffers[i]->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));
    }

    // Depth: Set the min and magfilter as nearest
    m_fullSceneTextures[0] = std::make_shared<Texture2DObject>();
    m_fullSceneTextures[0]->Bind();
    m_fullSceneTextures[0]->SetImage(0, width, height, TextureObject::FormatDepth, TextureObject::InternalFormatDepth);
    m_fullSceneTextures[0]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_NEAREST);
    m_fullSceneTextures[0]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_NEAREST);

    // Albedo: Bind the newly created texture, set the image, and the min and magfilter as nearest
    m_fullSceneTextures[1] = std::make_shared<Texture2DObject>();
    m_fullSceneTextures[1]->Bind();
    m_fullSceneTextures[1]->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormatSRGBA8);
    m_fullSceneTextures[1]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_NEAREST);
    m_fullSceneTextures[1]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_NEAREST);

    // Normal: Bind the newly created texture, set the image and the min and magfilter as nearest
    m_fullSceneTextures[2] = std::make_shared<Texture2DObject>();
    m_fullSceneTextures[2]->Bind();
    m_fullSceneTextures[2]->SetImage(0, width, height, TextureObject::FormatRG, TextureObject::InternalFormatRG16F);
    m_fullSceneTextures[2]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_NEAREST);
    m_fullSceneTextures[2]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_NEAREST);

    // Others: Bind the newly created texture, set the image and the min and magfilter as nearest
    m_fullSceneTextures[3] = std::make_shared<Texture2DObject>();
    m_fullSceneTextures[3]->Bind();
    m_fullSceneTextures[3]->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormatSRGBA8);
    m_fullSceneTextures[3]->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_NEAREST);
    m_fullSceneTextures[3]->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_NEAREST);

    m_fullSceneFramebuffer->Bind();
    m_fullSceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Depth, *m_fullSceneTextures[0]);
    m_fullSceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_fullSceneTextures[1]);
    m_fullSceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color1, *m_fullSceneTextures[2]);
    m_fullSceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color2, *m_fullSceneTextures[3]);

    // Set the draw buffers used by the framebuffer (all attachments except depth)
    m_fullSceneFramebuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 4>(
        {
            FramebufferObject::Attachment::Color0,
            FramebufferObject::Attachment::Color1,
            FramebufferObject::Attachment::Color2
        }));
    FramebufferObject::Unbind();
    Texture2DObject::Unbind();


}

void WaterApplication::InitializeRenderer()
{
    int width, height;
    GetMainWindow().GetDimensions(width, height);

    // Set up deferred passes for opaque items
    {
        std::unique_ptr<GBufferRenderPass> gbufferRenderPass(std::make_unique<GBufferRenderPass>(width, height));

        // Set the g-buffer textures as properties of the deferred material
        m_deferredMaterial->SetUniformValue("DepthTexture", gbufferRenderPass->GetDepthTexture());
        m_deferredMaterial->SetUniformValue("AlbedoTexture", gbufferRenderPass->GetAlbedoTexture());
        m_deferredMaterial->SetUniformValue("NormalTexture", gbufferRenderPass->GetNormalTexture());
        m_deferredMaterial->SetUniformValue("OthersTexture", gbufferRenderPass->GetOthersTexture());
        m_deferredMaterial->SetUniformValue("ShowType", m_showType);
    

        // Save some gbuffer textures for later
        m_depthTexture = gbufferRenderPass->GetDepthTexture();
        m_normalTexture = gbufferRenderPass->GetNormalTexture();
        m_otherTexture = gbufferRenderPass->GetOthersTexture();

        // Add the render passes
        m_renderer.AddRenderPass(std::move(gbufferRenderPass));
        m_renderer.AddRenderPass(std::make_unique<DeferredRenderPass>(m_deferredMaterial, m_mainSceneFramebuffer));
    }

    // Initialize the framebuffers and the textures they use
    InitializeFramebuffers();

    // Skybox pass
    // We can do this after the opaque pass since we know there is not going to any other opaque thing infront
    m_renderer.AddRenderPass(std::make_unique<SkyboxRenderPass>(m_skyboxTexture, m_mainSceneFramebuffer));

    // Transparency Passes
    // The goal is to have opaque g-buffer for the forward rendering
    // and then the full scene g-buffer for the rest of the passes going forward
    {
        // Copy the opaque gbuffer textures into our fullscene framebruffer
        std::shared_ptr<Material> copyGbuffer = CreatePostFXMaterial("shaders/postfx/copyGBuffer.frag", m_sceneTexture);
        copyGbuffer->SetUniformValue("DepthTexture", m_depthTexture);
        copyGbuffer->SetUniformValue("NormalTexture", m_normalTexture);
        copyGbuffer->SetUniformValue("OtherTexture", m_otherTexture);
        m_renderer.AddRenderPass(std::make_unique<GBufferCopyPass>(copyGbuffer, m_fullSceneFramebuffer));

        // Update the fullscene g-buffer with the transparent data
        m_renderer.AddRenderPass(std::make_unique<GBufferRenderPass>(m_fullSceneFramebuffer, true));

        // Run the forward rendering pass on the opaque data only
        m_renderer.AddRenderPass(std::make_unique<TransparencyPass>(m_mainSceneFramebuffer));
    }
    // SSR passes
    {
        // Get the reflection texture
        m_ssrMaterial = CreateSSRMaterial(m_sceneTexture, m_fullSceneTextures[0], m_fullSceneTextures[2], m_fullSceneTextures[3]);
        m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(m_ssrMaterial, m_reflectionBuffer));

        // Copy the reflections into temp buffers for blurring
        std::shared_ptr<Material> copyMaterial = CreatePostFXMaterial("shaders/postfx/copy.frag", m_reflectiveColorTexture);
        m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(copyMaterial, m_tempFramebuffers[0]));

        // Blur the copied reflection texture
        std::shared_ptr<Material> blurHorizontalMaterial = CreatePostFXMaterial("shaders/postfx/blur.frag", m_tempTextures[0]);
        std::shared_ptr<Material> blurVerticalMaterial = CreatePostFXMaterial("shaders/postfx/blur.frag", m_tempTextures[1]);

        blurHorizontalMaterial->SetUniformValue("Scale", glm::vec2(6.0f / width, 0.0f));
        blurVerticalMaterial->SetUniformValue("Scale", glm::vec2(0.0f, 6.0f / height));

        for (int i = 0; i < m_blurIterations; ++i)
        {
            m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(blurHorizontalMaterial, m_tempFramebuffers[1]));
            m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(blurVerticalMaterial, m_tempFramebuffers[0]));
        }
    }

    // Final composite pass
    std::shared_ptr<Material> composeMaterial = CreateCompositeMaterial(m_sceneTexture);
    composeMaterial->SetUniformValue("ReflectiveTexture", m_reflectiveColorTexture);
    composeMaterial->SetUniformValue("BlurReflectiveTexture", m_tempTextures[0]);
    composeMaterial->SetUniformValue("SpecularTexture", m_fullSceneTextures[3]);
    composeMaterial->SetUniformValue("EnvironmentTexture", m_skyboxTexture);
    composeMaterial->SetUniformValue("DepthTexture", m_fullSceneTextures[0]);
    composeMaterial->SetUniformValue("NormalTexture", m_fullSceneTextures[2]);

    m_skyboxTexture->Bind();
    float maxLod;
    m_skyboxTexture->GetParameter(TextureObject::ParameterFloat::MaxLod, maxLod);
    TextureCubemapObject::Unbind();
    composeMaterial->SetUniformValue("EnvironmentMaxLod", maxLod);

    m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(composeMaterial, m_renderer.GetDefaultFramebuffer()));
}

// Material for Screen-Space Reflections
std::shared_ptr<Material> WaterApplication::CreateSSRMaterial(std::shared_ptr<Texture2DObject> sourceTexture, std::shared_ptr<Texture2DObject> depthTexture, std::shared_ptr<Texture2DObject> normalTexture, std::shared_ptr<Texture2DObject> otherTexture)
{
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/ssr.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Get transform related uniform locations
    ShaderProgram::Location projMatrixLocation = shaderProgramPtr->GetUniformLocation("ProjectionMatrix");
    ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
    ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");

    // Register shader with renderer
    m_renderer.RegisterShaderProgram(shaderProgramPtr,
        [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            shaderProgram.SetUniform(projMatrixLocation, camera.GetProjectionMatrix());
            shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
        },
        nullptr
    );

    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("InvProjMatrix");
    filteredUniforms.insert("InvViewMatrix");
    filteredUniforms.insert("ProjectionMatrix");

    // Create material
    std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    material->SetUniformValue("SourceTexture", sourceTexture);
    material->SetUniformValue("DepthTexture", depthTexture);
    material->SetUniformValue("NormalTexture", normalTexture);
    material->SetUniformValue("SpecularTexture", otherTexture);

    material->SetUniformValue("MaxDistance", m_maxDistance);
    material->SetUniformValue("Resolution", m_resolution);
    material->SetUniformValue("Steps", m_steps);
    material->SetUniformValue("Thickness", m_thickness);
    return material;
}

// Material to combine and blend indirect lighting into the scene
std::shared_ptr<Material> WaterApplication::CreateCompositeMaterial(std::shared_ptr<Texture2DObject> sourceTexture)
{
    // We could keep this vertex shader and reuse it, but it looks simpler this way
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/lambert-ggx.glsl");
    fragmentShaderPaths.push_back("shaders/lighting.glsl");
    fragmentShaderPaths.push_back("shaders/postfx/compose.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
    ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
    m_renderer.RegisterShaderProgram(shaderProgramPtr,
        [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
            shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
        },
        nullptr
    );

    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("InvProjMatrix");
    filteredUniforms.insert("InvViewMatrix");

    // Create material
    std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    material->SetUniformValue("SourceTexture", sourceTexture);

    return material;
}

std::shared_ptr<Material> WaterApplication::CreatePostFXMaterial(const char* fragmentShaderPath, std::shared_ptr<Texture2DObject> sourceTexture)
{
    // We could keep this vertex shader and reuse it, but it looks simpler this way
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back(fragmentShaderPath);
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Create material
    std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr);
    material->SetUniformValue("SourceTexture", sourceTexture);
    
    return material;
}

Renderer::UpdateTransformsFunction WaterApplication::GetFullscreenTransformFunction(std::shared_ptr<ShaderProgram> shaderProgramPtr) const
{
    // Get transform related uniform locations
    ShaderProgram::Location invViewMatrixLocation = shaderProgramPtr->GetUniformLocation("InvViewMatrix");
    ShaderProgram::Location invProjMatrixLocation = shaderProgramPtr->GetUniformLocation("InvProjMatrix");
    ShaderProgram::Location worldViewProjMatrixLocation = shaderProgramPtr->GetUniformLocation("WorldViewProjMatrix");

    // Return transform function
    return [=](const ShaderProgram& shaderProgram, const glm::mat4& worldMatrix, const Camera& camera, bool cameraChanged)
        {
            if (cameraChanged)
            {
                shaderProgram.SetUniform(invViewMatrixLocation, glm::inverse(camera.GetViewMatrix()));
                shaderProgram.SetUniform(invProjMatrixLocation, glm::inverse(camera.GetProjectionMatrix()));
            }
            shaderProgram.SetUniform(worldViewProjMatrixLocation, camera.GetViewProjectionMatrix() * worldMatrix);
        };
}

void WaterApplication::RenderGUI()
{
    m_imGui.BeginFrame();

    // Draw GUI for scene nodes, using the visitor pattern
    ImGuiSceneVisitor imGuiVisitor(m_imGui, "Scene");
    m_scene.AcceptVisitor(imGuiVisitor);

    // Draw GUI for camera controller
    m_cameraController.DrawGUI(m_imGui);

    if (auto window = m_imGui.UseWindow("Water"))
    {
        ImGui::Checkbox("Play", &m_play);
        ImGui::SliderFloat("Time", &m_timeElapsed, 0, _MaxPlaytime);
        if (ImGui::CollapsingHeader("Debug Settings"))
        {
            ImGui::Indent();
            if (ImGui::BeginListBox("Show Type"))
            {
            
                if (ImGui::Selectable("Lighting", m_showType == 0))
                {
                    m_showType = 0;
                    m_deferredMaterial->SetUniformValue("ShowType", m_showType);
                }
                if (ImGui::Selectable("Albedo", m_showType == 1))
                {
                    m_showType = 1;
                    m_deferredMaterial->SetUniformValue("ShowType", m_showType);
                }
                if (ImGui::Selectable("Position", m_showType == 2))
                {
                    m_showType = 2;
                    m_deferredMaterial->SetUniformValue("ShowType", m_showType);
                }
                if (ImGui::Selectable("Depth", m_showType == 3))
                {
                    m_showType = 3;
                    m_deferredMaterial->SetUniformValue("ShowType", m_showType);
                }
                if (ImGui::Selectable("WorldNormal", m_showType == 4))
                {
                    m_showType = 4;
                    m_deferredMaterial->SetUniformValue("ShowType", m_showType);
                }
                if (ImGui::Selectable("ViewNormal", m_showType == 5))
                {
                    m_showType = 5;
                    m_deferredMaterial->SetUniformValue("ShowType", m_showType);
                }
                ImGui::EndListBox();
            }
            ImGui::Unindent();
        }
        m_waterManager->RenderGUI(m_imGui);

        if (ImGui::CollapsingHeader("SSR Settings"))
        {
            ImGui::Indent();
            if (ImGui::DragFloat("Max distance", &m_maxDistance, 1.0f, 0.0f, 100))
            {
                m_ssrMaterial->SetUniformValue("MaxDistance", m_maxDistance);
            }
            if (ImGui::DragFloat("Resolution", &m_resolution, 0.1f, 0.0f, 1.0f))
            {
                m_ssrMaterial->SetUniformValue("Resolution", m_resolution);
            }
            if (ImGui::DragInt("Steps", &m_steps, 1.0f, 0.0, 100))
            {
                m_ssrMaterial->SetUniformValue("Steps", m_steps);
            }
            if (ImGui::DragFloat("Thickness", &m_thickness, 0.05f, 0.0f, 10.0))
            {
                m_ssrMaterial->SetUniformValue("Thickness", m_thickness);
            }
            ImGui::Unindent();
        }
    }

    m_imGui.EndFrame();
}
