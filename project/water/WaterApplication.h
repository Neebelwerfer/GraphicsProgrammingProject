#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/scene/Scene.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/renderer/Renderer.h>
#include <ituGL/camera/CameraController.h>
#include <ituGL/utils/DearImGui.h>
#include <ituGL/scene/SceneLight.h>
#include "WaterManager.h"

class Texture2DObject;
class TextureCubemapObject;
class Material;

class WaterApplication : public Application
{
public:
    WaterApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeCamera();
    void InitializeLights();
    void InitializeMaterials();
    void InitializeModels();
    void InitializeFramebuffers();
    void InitializeRenderer();

    std::shared_ptr<Material> CreatePostFXMaterial(const char* fragmentShaderPath, std::shared_ptr<Texture2DObject> sourceTexture = nullptr);
    std::shared_ptr<Material> CreateSSReflectionMaterial(std::shared_ptr<Texture2DObject> sourceTexture, std::shared_ptr<Texture2DObject> depthTexture, std::shared_ptr<Texture2DObject> normalTexture, std::shared_ptr<Texture2DObject> otherTexture);
    std::shared_ptr<Material> CreateSSRefractionMaterial(std::shared_ptr<Texture2DObject> sourceTexture, std::shared_ptr<Texture2DObject> depthFromTexture, std::shared_ptr<Texture2DObject> depthToTexture, std::shared_ptr<Texture2DObject> normalFromTexture, std::shared_ptr<Texture2DObject> otherTexture);
    std::shared_ptr<Material> CreateCompositeMaterial(std::shared_ptr<Texture2DObject> sourceTexture);

    Renderer::UpdateTransformsFunction GetFullscreenTransformFunction(std::shared_ptr<ShaderProgram> shaderProgramPtr) const;

    void RenderGUI();

private:
    // Helper object for debug GUI
    DearImGui m_imGui;

    // Camera controller
    CameraController m_cameraController;

    // Global scene
    Scene m_scene;

    // Renderer
    Renderer m_renderer;
    
    // Light
    std::shared_ptr<SceneLight> m_spotLight;
    float m_lightRotationSpeed;

    // Time
    float m_timeElapsed;
    bool m_play;

    // SSR Properties
    float m_maxDistance;
    float m_resolution;
    int   m_steps;
    float m_thickness;
    bool m_ssrEnabled;

    //SSRefration Properties
    float m_RefractionMaxDistance;
    float m_Refractionresolution;
    int   m_Refractionsteps;
    float m_Refractionthickness;
    float m_rior;
    float m_depthMax;
    bool m_ssRefractionEnabled;

    // Blur Properties
    int m_blurIterations;
    std::array<std::shared_ptr<FramebufferObject>, 2> m_tempFramebuffers;
    std::array<std::shared_ptr<Texture2DObject>, 2> m_tempTextures;

    // Skybox texture
    std::shared_ptr<TextureCubemapObject> m_skyboxTexture;
    float m_maxLod;

    std::shared_ptr<WaterManager> m_waterManager;

    // Materials
    std::shared_ptr<Material> m_defaultMaterial;
    std::shared_ptr<Material> m_deferredMaterial;
    std::shared_ptr<Material> m_ssrMaterial;
    std::shared_ptr<Material> m_ssRefractionMaterial;

    // Framebuffers
    std::shared_ptr<FramebufferObject> m_mainSceneFramebuffer;
    std::shared_ptr<Texture2DObject> m_sceneTexture;
    std::shared_ptr<Texture2DObject> m_depthTexture;
    std::shared_ptr<Texture2DObject> m_normalTexture;
    std::shared_ptr<Texture2DObject> m_otherTexture;


    // Refraction
    std::shared_ptr<FramebufferObject> m_backgroundTexturesBuffer;
    std::shared_ptr<Texture2DObject> m_backgroundTexture;
    std::shared_ptr<Texture2DObject> m_backgroundDepth;
    std::shared_ptr<FramebufferObject> m_refractionBuffer;
    std::shared_ptr<Texture2DObject> m_refractionColorTexture;


    //
    std::shared_ptr<FramebufferObject> m_fullSceneFramebuffer;
    std::array<std::shared_ptr<Texture2DObject>, 4> m_fullSceneTextures;

    //Reflection
    std::shared_ptr<FramebufferObject> m_reflectionBuffer;
    std::shared_ptr<Texture2DObject> m_reflectiveColorTexture;

};
