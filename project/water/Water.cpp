#include "Water.h"
#include <imgui.h>
#include <ituGL/geometry/Mesh.h>
#include <ituGL/asset/ShaderLoader.h>


Water::Water(Material waterMaterial)
{
	std::array<glm::vec3, sizeof(glm::vec3) * 4> vertices{ };
	vertices[0] = glm::vec3(0, 0, 0);
	vertices[1] = glm::vec3(1, 0, 0);
	vertices[2] = glm::vec3(0, 0, 1);
	vertices[3] = glm::vec3(1, 0, 1);
	Mesh mesh;
	mesh.AddVertexData<glm::vec3>(std::span(vertices));
}

Water::~Water()
{

}

void Water::RenderGUI(DearImGui imgui)
{

}

std::shared_ptr<Material> Water::InitializeWaterMaterial(Renderer& renderer)
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

    // Register shader with renderer
    renderer.RegisterShaderProgram(shaderProgramPtr,
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
    std::shared_ptr waterMaterial = std::make_shared<Material>(shaderProgramPtr, filteredUniforms);
    waterMaterial->SetUniformValue("Color", glm::vec3(1.0f));
    return waterMaterial;
}
