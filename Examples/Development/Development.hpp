#pragma once

#include "Framework/Core/Application.hpp"
//
//
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/Renderer.hpp"
#include "BindlessVk/Renderer/Rendergraph.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"
#include "Framework/Common/Common.hpp"
#include "Framework/Core/Window.hpp"
#include "Framework/Scene/CameraController.hpp"
#include "Framework/Scene/Scene.hpp"
#include "Framework/Utils/Timer.hpp"
#include "Rendergraphs/Graph.hpp"
#include "Rendergraphs/Passes/Forward.hpp"
#include "Rendergraphs/Passes/UserInterface.hpp"

#include <imgui.h>

// @todo: load stuff from files
class DevelopmentExampleApplication: public Application
{
public:
	DevelopmentExampleApplication();
	~DevelopmentExampleApplication();

	void on_tick(f64 delta_time) final;

	void on_swapchain_recreate() final
	{
		assert_fail("Swapchain recreation not supported (yet)");
	}

private:
	void load_shaders();

	void load_shader_effects();

	void load_pipeline_configuration();

	void load_materials();

	void load_models();

	void load_entities();

	auto create_forward_pass_blueprint() -> bvk::RenderpassBlueprint;

	auto create_ui_pass_blueprint() -> bvk::RenderpassBlueprint;

	void create_render_graph();
};
