#pragma once 
#include <ituGL/utils/DearImGui.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/shader/Material.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/renderer/Renderer.h>
#include <ituGL/geometry/Model.h>


class WaterManager
{
public:
	WaterManager(Renderer& renderer, float& time);
	~WaterManager();

	void RenderGUI(DearImGui& imgui);
	const std::shared_ptr<Model> GetWaterPlane();

private:
	void InitializeWaterMaterial(Renderer& renderer, float& time);
	void LoadModel();

	std::shared_ptr<Material> m_waterMaterial;
	std::shared_ptr<Model> m_waterPlane;

	//Material Traits
	glm::vec3 m_colour;
	glm::vec2 m_jump;
	int m_tiling;
	float m_speed;
	float m_flowStrength;
	float m_flowOffset;
	float m_heightScale;
	float m_heightScaleModulated;

	//Visual Properties
	float m_ambientOcclusion;
	float m_roughness;
	float m_metalness;
	float m_alpha;
};