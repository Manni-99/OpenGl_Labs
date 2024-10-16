#include "assignment5.hpp"
#include "parametric_shapes.hpp"
#include "parametric_shapes.cpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/node.hpp"
#include "core/helpers.hpp"
#include "core/ShaderProgramManager.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

// Member variables for storing objects and positions
struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

std::vector<Node> demo_coins;
std::vector<glm::vec3> positions;

edaf80::Assignment5::Assignment5(WindowManager &windowManager) : mCamera(0.5f * glm::half_pi<float>(),
																		 static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
																		 0.01f, 1000.0f),
																 inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr)
	{
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(-40.0f, 14.0f, 6.0f));
	mCamera.mWorld.LookAt(glm::vec3(0.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(9.0f); // 9 m/s => 31.2 km/h
	auto camera_position = mCamera.mWorld.GetTranslation();

	// Create the shader programs
	ShaderProgramManager program_manager;
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

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Phong Shading",
											 {{ShaderType::vertex, "EDAF80/phong.vert"},
											  {ShaderType::fragment, "EDAF80/phong.frag"}},
											 phong_shader);
	if (phong_shader == 0u)
		LogError("Failed to load phong shader");

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program)
	{
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};
	// ------------------------------------------------------------------------------
	float elapsed_time_s = 0.0f;
	bool use_normal_mapping = true;
	auto const water_set_uniforms = [&use_normal_mapping, &light_position, &camera_position, &elapsed_time_s](GLuint program)
	{
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1fv(glGetUniformLocation(program, "ellapsed_time_s"), 1, &elapsed_time_s);
	};
	// ------------------------------------------------------------------------------
	auto const phong_set_uniforms = [&use_normal_mapping, &light_position, &camera_position](GLuint program)
	{
		glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), use_normal_mapping ? 1 : 0);
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
	};

	// ------------------------------------------------------------------------------
	auto skybox_shape = parametric_shapes::createSphere(400.0f, 300u, 300u); // Increased value to give the illusion of an infinite skybox
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
	// ------------------------------------------------------------------------------
	auto demo_shape = parametric_shapes::createQuad(650.0f, 650.0f, 1150u, 1150u);
	if (demo_shape.vao == 0u)
	{
		LogError("Failed to retrieve the mesh for the demo sphere");
		return;
	}
	// ------------------------------------------------------------------------------
	auto demo_mesh_coin = parametric_shapes::createSphere(4.5f, 40u, 40u); // Coin mesh
	if (demo_mesh_coin.vao == 0u)
	{
		LogError("Failed to retrieve the mesh for the demo model");
		return;
	}

	GLuint demo_bump_texture_cat = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/models/Cat_v1_L3.123cb1b1943a-2f48-4e44-8f71-6bbe19a3ab64/Cat_bump.jpg", true);
	GLuint demo_diffuse_texture_cat = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/models/Cat_v1_L3.123cb1b1943a-2f48-4e44-8f71-6bbe19a3ab64/Cat_diffuse.jpg", true);

	GLuint demo_diffuse_texture = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/textures/Texturelabs_Metal_270M.jpg", true);
	GLuint demo_specular_texture = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/textures/leather_red_02_rough_2k.jpg", true);
	GLuint demo_normal_texture = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/textures/leather_red_02_nor_2k.jpg", true);
	// 
	if (demo_diffuse_texture == 0 || demo_specular_texture == 0 || demo_normal_texture == 0)
	{
		LogError("Failed to load texture onto coin.");
	}

	bonobo::material_data demo_material;
	demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	demo_material.diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
	demo_material.shininess = 10.0f;
	Log("We are going to add to the demo_quad node now!!");
	// ------------------------------------------------------------------------------
	GLuint demo_water_texture = bonobo::loadTexture2D("/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/textures/waves.png", true);
	Node demo_quad;
	demo_quad.set_geometry(demo_shape);
	demo_quad.set_material_constants(demo_material);
	demo_quad.set_program(&water_shader, water_set_uniforms);
	demo_quad.add_texture("skybox", cubeMap, GL_TEXTURE_CUBE_MAP);
	demo_quad.add_texture("water", demo_water_texture, GL_TEXTURE_2D);
	// ------------------------------------------------------------------------------
	struct Coin
	{
		Node node;
		glm::vec3 position;
		glm::vec3 size;
		bool collected;
	};

	std::vector<Coin> coins;
	// Seed the random number generator
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	float min_distance = 50.0f; // Minimum distance between objects

	// Generate positions for the objects
	for (int i = 0; i < 10; i++)
	{
		glm::vec3 position;
		bool isFarEnough;

		// Loop until a position is found that is far enough from all others
		do
		{
			// Generate random x, y, z coordinates in the range [-100, 100]
			float x = static_cast<float>((std::rand() % 201) - 100);
			float z = static_cast<float>((std::rand() % 201) - 100);
			float y = static_cast<float>(0); // y in [1, 40], above the quad surface
			position = glm::vec3(x, y, z);

			isFarEnough = true;
			for (const auto &pos : positions)
			{
				// Calculate distance between this position and all previously stored positions
				float distance = glm::length(pos - position);
				if (distance < min_distance)
				{
					isFarEnough = false;
					break;
				}
			}
		} while (!isFarEnough);

		// Store the valid position
		positions.push_back(position);

		// Create a new Node for each object
		Node demo_coin;
		demo_coin.set_geometry(demo_mesh_coin); // Use the mesh you already created
		demo_coin.set_material_constants(demo_material);
		demo_coin.set_program(&phong_shader, phong_set_uniforms);
		demo_coin.add_texture("demo_diffuse_texture", demo_diffuse_texture, GL_TEXTURE_2D);
		demo_coin.add_texture("demo_specular_texture", demo_specular_texture, GL_TEXTURE_2D);
		demo_coin.add_texture("demo_normal_texture", demo_normal_texture, GL_TEXTURE_2D);
		demo_coin.get_transform().SetTranslate(position);

		// Add the demo_model to the list
		Coin coin = {demo_coin, position, glm::vec3(45.5f, 45.5f, 45.5f), false};
		coins.push_back(coin);
	}
	// ------------------------------------------------------------------------------

	const std::string path = "/home/manni/LTH/Year3/Datorgrafik/AAA_Programing/CG_Labs/res/models/Cat_v1_L3.123cb1b1943a-2f48-4e44-8f71-6bbe19a3ab64/12221_Cat_v1_l3.obj";
	std::vector<bonobo::mesh_data> demo_mesh_model = bonobo::loadObjects(path);
	Node demo_cat;
	demo_cat.set_geometry(demo_mesh_model[0]);
	demo_cat.set_material_constants(demo_material);
	demo_cat.set_program(&phong_shader, phong_set_uniforms); // Assuming demo_material has the correct settings
	demo_cat.add_texture("demo_diffuse_texture_cat", demo_diffuse_texture, GL_TEXTURE_2D);
	demo_cat.add_texture("demo_specular_texture", demo_specular_texture, GL_TEXTURE_2D);
	demo_cat.add_texture("demo_bump_texture_cat", demo_normal_texture, GL_TEXTURE_2D);

	std::vector<std::unique_ptr<Node>> parts;
	parts.reserve(demo_mesh_model.size() - 1);

	for (int i = 1; i < demo_mesh_model.size(); i++)
	{
		parts.push_back(std::make_unique<Node>());
		parts[i - 1]->set_geometry(demo_mesh_model[i]);
		parts[i - 1]->set_material_constants(demo_material);
		parts[i - 1]->set_program(&phong_shader, phong_set_uniforms);
		parts[i - 1]->add_texture("my_diffuse", demo_diffuse_texture, GL_TEXTURE_2D);
		parts[i - 1]->add_texture("my_specular", demo_specular_texture, GL_TEXTURE_2D);
		parts[i - 1]->add_texture("my_normal", demo_normal_texture, GL_TEXTURE_2D);

		demo_cat.add_child(parts[i - 1].get());
	}
	// ------------------------------------------------------------------------------

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	std::int32_t demo_quad_program_index = 0;

	auto lastTime = std::chrono::high_resolution_clock::now();

	bool pause_animation = false;
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

	glm::vec3 cat_position = glm::vec3(0.0f, 0.0f, 0.0f); // Initial position
	float cat_rotation = 0.0f;							  // Rotation in degrees or radians, depending on your preference
	float cat_turn_speed = 90.0f;						  // Speed of turning in degrees per second
	float cat_speed = 50.0f;							  // Movement speed factor

	glm::vec3 cat_size(0.5f, 1.0f, 0.5f); // Assuming the cat is roughly 0.5 units wide, 1 unit tall, 0.5 units deep

	glm::vec3 camera_offset(0.0f, 30.0f, -100.0f); // Adjust this as necessary for your desired view

	glm::vec3 skybox_min(-200.0f, -200.0f, -200.0f); // Example values
	glm::vec3 skybox_max(200.0f, 200.0f, 200.0f);	 // Example values

	int coinsCollected = 0;
	int totalCoins = 10;
	int count_down = 10;
	float timeAccumulator = 0.0f; // Accumulator to track elapsed time

	bool showWinPopup = false;
	bool showLosePopup = false;
	bool gameWon = false;
	bool gameLost = false;

	while (!glfwWindowShouldClose(window))
	{
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		if (!pause_animation)
		{
			elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();
			timeAccumulator += std::chrono::duration<float>(deltaTimeUs).count();

			if (timeAccumulator >= 1.0f && count_down >= 0)
			{
				count_down -= 1;
				timeAccumulator = 0.0f; // Reset the accumulator
			}
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
		inputHandler.Advance();
		mCamera.Update(deltaTimeUs, inputHandler);
		if (use_orbit_camera)
		{
			mCamera.mWorld.LookAt(glm::vec3(0.0f));
		}
		camera_position = mCamera.mWorld.GetTranslation();

		// Update cat position based on WASD keys
		if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED)
		{
			cat_position.z -= cat_speed * std::chrono::duration<float>(deltaTimeUs).count(); // Move forward
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED)
		{
			cat_position.z += cat_speed * std::chrono::duration<float>(deltaTimeUs).count(); // Move backward
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED)
		{
			// cat_rotation -= cat_turn_speed * std::chrono::duration<float>(deltaTimeUs).count(); // Rotate right (clockwise)
			cat_position.x -= cat_speed * std::chrono::duration<float>(deltaTimeUs).count(); // Move left
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED)
		{
			// cat_rotation += cat_turn_speed * std::chrono::duration<float>(deltaTimeUs).count();
			cat_position.x += cat_speed * std::chrono::duration<float>(deltaTimeUs).count(); // Move right
		}

		// Update cat movement based on its rotation (forward/backward)
		/*if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED || inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED)
		{
			cat_position.x += cat_speed * cos(cat_rotation) * std::chrono::duration<float>(deltaTimeUs).count();
			cat_position.z += cat_speed * cos(cat_rotation) * std::chrono::duration<float>(deltaTimeUs).count();
		}*/
		//	glm::vec3 camera_offset = glm::vec3(-5.0f * cos(cat_rotation), 3.0f, -5.0f * sin(cat_rotation));

		// Update the camera's position to follow the cat
		glm::vec3 target_camera_position = cat_position + camera_offset;

		camera_position = glm::mix(camera_position, target_camera_position, 0.1f); // Adjust the alpha (0.1f) for smoothness

		// camera_position = target_camera_position; // Direct follow (uncomment the previous line for smooth follow)

		// Make the camera look at the cat
		mCamera.mWorld.LookAt(cat_position);

		// Set the camera position to the updated camera position
		mCamera.mWorld.SetTranslate(camera_position);

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

		for (Coin &coin : coins)
		{
			glm::vec3 cat_min = cat_position - cat_size * 0.5f;
			glm::vec3 cat_max = cat_position + cat_size * 0.5f;

			glm::vec3 coin_min = coin.position - coin.size * 0.5f;
			glm::vec3 coin_max = coin.position + coin.size * 0.5f;

			if (!coin.collected && checkCollisionCoins(cat_min, cat_max, coin_min, coin_max))
			{
				coin.collected = true;
				coinsCollected++;
				count_down++;
			}
		}

		for (const Coin &coin : coins)
		{
			if (!coin.collected)
			{
				coin.node.render(mCamera.GetWorldToClipMatrix());
			}
		}

		glm::mat4 cat_scaled = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
		glm::mat4 cat_translation = glm::translate(glm::mat4(1.0f), cat_position);

		//glm::mat4 cat_dynamic_rotation = glm::rotate(glm::mat4(1.0f), glm::radians(cat_rotation), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 cat_rotationUp = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 cat_result = cat_translation * cat_rotationUp * cat_scaled;

		demo_cat.render(mCamera.GetWorldToClipMatrix(), cat_result);

		glm::vec3 cat_min = cat_position - cat_size * 0.5f; // Min corner of cat's bounding box
		glm::vec3 cat_max = cat_position + cat_size * 0.5f; // Max corner of cat's bounding box

		if (checkCollisionSkybox(cat_min, cat_max, skybox_min, skybox_max))
		{

			if (cat_position.x < skybox_min.x)
			{
				cat_position.x = skybox_min.x + cat_size.x * 0.5f; // Snap to the left edge
			}
			else if (cat_position.x > skybox_max.x)
			{
				cat_position.x = skybox_max.x - cat_size.x * 0.5f; // Snap to the right edge
			}

			if (cat_position.y < skybox_min.y)
			{
				cat_position.y = skybox_min.y + cat_size.y * 0.5f; // Snap to the bottom edge
			}
			else if (cat_position.y > skybox_max.y)
			{
				cat_position.y = skybox_max.y - cat_size.y * 0.5f; // Snap to the top edge
			}

			if (cat_position.z < skybox_min.z)
			{
				cat_position.z = skybox_min.z + cat_size.z * 0.5f; // Snap to the back edge
			}
			else if (cat_position.z > skybox_max.z)
			{
				cat_position.z = skybox_max.z - cat_size.z * 0.5f; // Snap to the front edge
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		// Setup ImGui frame
		bool opened = ImGui::Begin("Score Board", nullptr, ImGuiWindowFlags_None);
		if (opened)
		{
			
			ImGui::Text("Coins Remaining: %d", totalCoins - coinsCollected);
			ImGui::Text("Time Remaining: %d", count_down);
			if (coinsCollected == 10 && !gameWon)
			{
				showWinPopup = true;
				gameWon = true;
				count_down = -1;
				ImGui::OpenPopup("You Won!");
			}
			if (ImGui::BeginPopupModal("You Won!", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Congratulations! You have collected all the coins!");
				if (ImGui::Button("OK"))
				{
					showWinPopup = false; // Close the popup when the "OK" button is pressed
					ImGui::CloseCurrentPopup();
					break;
				}
				ImGui::EndPopup();
			}

			if (count_down == 0 && !gameLost)
			{
				showLosePopup = true;
				gameLost = true;
				ImGui::OpenPopup("You Lost, Sorry try again");
			}
			if (ImGui::BeginPopupModal("You Lost, Sorry try again", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				count_down = 0;
				ImGui::Text("Damn bro you lost! Weird cause this is such an easy game, Try again!");
				if (ImGui::Button("OK"))
				{
					showLosePopup = false; // Close the popup when the "OK" button is pressed
					ImGui::CloseCurrentPopup();
					break;
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();
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
		edaf80::Assignment5 assignment5(framework.GetWindowManager());

		assignment5.run();
	}
	catch (std::runtime_error const &e)
	{
		LogError(e.what());
	}
}

bool edaf80::Assignment5::checkCollisionSkybox(const glm::vec3 &min1, const glm::vec3 &max1, const glm::vec3 &min2, const glm::vec3 &max2)
{
	return (min1.x < min2.x || max1.x > max2.x ||
			min1.y < min2.y || max1.y > max2.y ||
			min1.z < min2.z || max1.z > max2.z);
}

bool edaf80::Assignment5::checkCollisionCoins(const glm::vec3 &min1, const glm::vec3 &max1, const glm::vec3 &min2, const glm::vec3 &max2)
{
	return (min1.x < max2.x && max1.x > min2.x &&
			min1.y < max2.y && max1.y > min2.y &&
			min1.z < max2.z && max1.z > min2.z);
}