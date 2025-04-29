#pragma once 
#include <ituGL/utils/DearImGui.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/shader/Material.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/renderer/Renderer.h>

class Water : Model
{
public:
	Water(Material waterMaterial);
	~Water();

	static void RenderGUI(DearImGui imgui);
	static std::shared_ptr<Material> InitializeWaterMaterial(Renderer& renderer, const float& time);

private:
	static std::shared_ptr<Material> _waterMaterial;
};