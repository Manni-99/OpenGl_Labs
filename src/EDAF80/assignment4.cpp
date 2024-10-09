#include "assignment4.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

edaf80::Assignment4::Assignment4(WindowManager &windowManager) : mCamera(0.5f * glm::half_pi<float>(),
																		 static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
																		 0.01f, 1000.0f),
																 inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 4", window_datum, config::msaa_rate);
	if (window == nullptr)
	{
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment4::~Assignment4()
{
	bonobo::deinit();
}

void edaf80::Assignment4::run()
{
	Log("TESTING RUN");
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(-40.0f, 14.0f, 6.0f));
	mCamera.mWorld.LookAt(glm::vec3(0.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h
	auto camera_position = mCamera.mWorld.GetTranslation();

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
											 {{ShaderType::vertex, "EDAF80/skybox.vert"},
											  {ShaderType::fragment, "EDAF80/skybox.frag"}},
											 skybox_shader);
	if (skybox_shader == 0u)
		LogError("Failed to load skybox shader");

	GLuint water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Water Shading",
											 {{ShaderType::vertex, "EDAF80/water.vert"},
											  {ShaderType::fragment, "EDAF80/water.frag"}},
											 water_shader);
	if (water_shader == 0u)
		LogError("Failed to load water shader");

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program)
	{
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	/*GLuint skyboxTextureID;
	glGenTextures(1, &skyboxTextureID);
	// Step 2: Bind the texture
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID);*/
	float elapsed_time_s = 0.0f;
	bool use_normal_mapping = false;
	auto const water_set_uniforms = [&use_normal_mapping, &light_position, &camera_position, &elapsed_time_s](GLuint program)
	{
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1fv(glGetUniformLocation(program, "ellapsed_time_s"), 1, &elapsed_time_s);
		//glUniform1i(glGetUniformLocation(program, "skybox"), 0); // Texture unit 0
	};

	auto skybox_shape = parametric_shapes::createSphere(200.0f, 1000u, 1000u); // Increased value to give the illusion of an infinite skybox
	if (skybox_shape.vao == 0u)
	{
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}
	GLuint cubeMap = bonobo::loadTextureCubeMap("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/cubemaps/NissiBeach2/posx.jpg", "/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/cubemaps/NissiBeach2/negx.jpg", "/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/cubemaps/NissiBeach2/posy.jpg",
												"/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/cubemaps/NissiBeach2/negy.jpg", "/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/cubemaps/NissiBeach2/posz.jpg", "/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/cubemaps/NissiBeach2/negz.jpg",
												true);

	Node skybox; // Add geometry and shader program to skybox node here!
	Log("Skybox_shape value %d\n", skybox_shape);
	Log("Skybox_shader value %d\n", skybox_shader);

	skybox.set_geometry(skybox_shape); // The skybox works but the current logic near the createQuad method is stopping the program from displaying the skybox
	skybox.set_program(&skybox_shader, set_uniforms);
	skybox.add_texture("cubemap", cubeMap, GL_TEXTURE_CUBE_MAP);

	auto demo_shape = parametric_shapes::createQuad(100.0f, 100.0f, 1000u, 1000u);
	if (demo_shape.vao == 0u)
	{
		LogError("Failed to retrieve the mesh for the demo sphere");
		return;
	}

	bonobo::material_data demo_material;
	// demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	// demo_material.diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	demo_material.shininess = 10.0f;
	Log("We are going to add to the demo_quad node now!!");

	 GLuint demo_water_texture = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/textures/waves.png", true);

	Node demo_quad;
	demo_quad.set_geometry(demo_shape);
	demo_quad.set_material_constants(demo_material);
	demo_quad.set_program(&water_shader, water_set_uniforms);
	demo_quad.add_texture("skybox", cubeMap, GL_TEXTURE_CUBE_MAP);
	demo_quad.add_texture("water", demo_water_texture, GL_TEXTURE_2D);

	// Here we should implement the smaller sphere to change its color, phong shading
	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	std::int32_t demo_quad_program_index = 0;
	//
	// Todo: Insert the creation of other shader programs.
	//       (Check how it was done in assignment 3.)
	//

	//
	// Todo: Load your geometry
	//

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::high_resolution_clock::now();
	bool pause_animation = true;
	bool use_orbit_camera = false;
	auto cull_mode = bonobo::cull_mode_t::disabled;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	changeCullMode(cull_mode);

	while (!glfwWindowShouldClose(window))
	{
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		if (!pause_animation)
		{
			elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
		}

		auto &io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		if (use_orbit_camera)
		{
			mCamera.mWorld.LookAt(glm::vec3(0.0f));
		}
		camera_position = mCamera.mWorld.GetTranslation();

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED)
		{
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
								   "An error occurred while reloading shader programs; see the logs for details.\n"
								   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
								   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		//
		// Todo: If you need to handle inputs, you can do it here
		//

		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);

		//
		// Todo: Render all your geometry here.
		//
		glDepthFunc(GL_LEQUAL);
		skybox.render(mCamera.GetWorldToClipMatrix());
		glDepthFunc(GL_LESS);

		demo_quad.render(mCamera.GetWorldToClipMatrix());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

		bool opened = ImGui::Begin("Scene Control", nullptr, ImGuiWindowFlags_None);
		if (opened)
		{
			ImGui::Checkbox("Pause animation", &pause_animation);
			ImGui::Checkbox("Use orbit camera", &use_orbit_camera);
			ImGui::Separator();
			auto const cull_mode_changed = bonobo::uiSelectCullMode("Cull mode", cull_mode);
			if (cull_mode_changed)
			{
				changeCullMode(cull_mode);
			}
			auto demo_quad_selection_result = program_manager.SelectProgram("Demo quad", demo_quad_program_index);
			if (demo_quad_selection_result.was_selection_changed)
			{
				demo_quad.set_program(demo_quad_selection_result.program, water_set_uniforms);
			}
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			ImGui::Separator();
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try
	{
		edaf80::Assignment4 assignment4(framework.GetWindowManager());
		assignment4.run();
	}
	catch (std::runtime_error const &e)
	{
		LogError(e.what());
	}
}
