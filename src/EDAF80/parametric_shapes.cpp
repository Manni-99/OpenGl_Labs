#include "parametric_shapes.hpp"
#include "core/Log.h"

#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include <stdio.h>

bonobo::mesh_data
parametric_shapes::createQuad(float const width, float const height,
							  unsigned int const horizontal_split_count,
							  unsigned int const vertical_split_count)
{
	Log("Start of create Quad");
	std::vector<glm::uvec3> index_sets;
	bonobo::mesh_data data;

	// Calculate the total number of vertices
	unsigned int const num_verts_x = horizontal_split_count + 1;
	unsigned int const num_verts_z = vertical_split_count + 1;
	unsigned int const total_vertices = num_verts_x * num_verts_z;

	// Create vectors with pre-allocated size (instead of reserve and emplace_back)
	std::vector<glm::vec3> vertices(total_vertices);
	std::vector<glm::vec3> texcoords(total_vertices);

	// Calculate step size in the X and Z directions (for the x-z plane)
	float const step_x = width / horizontal_split_count;
	float const step_z = height / vertical_split_count;

	// Fill vertices and texture coordinates using indexed access
	unsigned int index = 0;
	for (unsigned int z = 0; z <= horizontal_split_count; ++z)
	{
		for (unsigned int x = 0; x <= vertical_split_count; ++x)
		{
			glm::vec3 vertex = glm::vec3(x * step_x - width * 0.5f, 0.0f, z * step_z - height * 0.5f);
			float const u = static_cast<float>(x) / horizontal_split_count;
			float const v = static_cast<float>(z) / vertical_split_count;

			// Vertices in the x-z plane, y is always 0
			vertices[index] = vertex;				  // x-z plane
			texcoords[index] = glm::vec3(u, v, 0.0f); // Texture coordinates
			++index;
		}
	}
	index_sets = std::vector<glm::uvec3>(2u * horizontal_split_count * vertical_split_count);
	size_t index_sets_counter = 0u;
	for (unsigned int lat = 0u; lat < vertical_split_count; ++lat)
	{
		for (unsigned int lon = 0u; lon < horizontal_split_count; ++lon)
		{
			unsigned int first = lat * (horizontal_split_count + 1) + lon;
			unsigned int second = first + horizontal_split_count + 1;

			// First triangle of the quad (counter-clockwise winding)
			index_sets[index_sets_counter] = glm::uvec3(first, second, first + 1);
			++index_sets_counter;

			// Second triangle of the quad (counter-clockwise winding)
			index_sets[index_sets_counter] = glm::uvec3(second, second + 1, first + 1);
			++index_sets_counter;
		}
	}

	if (vertices.empty() || texcoords.empty() || index_sets.empty())
	{
		LogError("Vertices, texture coordinates, or indices are empty!");
		return data;
	}

	Log("Before offset");
	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const texcoords_offset = vertices_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size + texcoords_size);

	Log("Vertices size: %d\n", vertices_size);
	fflush(stdout);

	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	glGenBuffers(1, &data.bo);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	Log("Buffer size in createQuad:  %d\n", bo_size);

	// Allocate memory for both vertices and texcoords
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const *>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));
	
	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const *>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(texcoords_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const *>(index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
	Log("We are the end of the createQuad method \n");
	return data;
}

bonobo::mesh_data parametric_shapes::createSphere(float const radius,
												  unsigned int const longitude_split_count,
												  unsigned int const latitude_split_count)
{

	unsigned int vertex_count = (longitude_split_count + 1) * (latitude_split_count + 1);

	// Vectors for storing vertices, normals, tangents, binormals, uvs, and indices
	std::vector<glm::vec3> vertices(vertex_count);
	std::vector<glm::vec3> normals(vertex_count);
	std::vector<glm::vec3> tangents(vertex_count);	// Tangent vectors (∂p/∂θ)
	std::vector<glm::vec3> binormals(vertex_count); // Binormal vectors (∂p/∂φ)
	std::vector<glm::vec3> textcoords(vertex_count);
	std::vector<glm::uvec3> index_sets;
	// Create vertices, normals, tangents, and binormals
	for (unsigned int lat = 0; lat <= latitude_split_count; ++lat)
	{
		float phi = glm::pi<float>() * static_cast<float>(lat) / static_cast<float>(latitude_split_count); // Latitude (ϕ): [0, π]
		float sin_phi = glm::sin(phi);
		float cos_phi = glm::cos(phi);

		for (unsigned int lon = 0; lon <= longitude_split_count; ++lon)
		{
			float theta = 2.0f * glm::pi<float>() * static_cast<float>(lon) / static_cast<float>(longitude_split_count); // Longitude (θ): [0, 2π]
			float sin_theta = glm::sin(theta);
			float cos_theta = glm::cos(theta);

			// Vertex position (p(θ, ϕ))
			glm::vec3 vertex = glm::vec3(
				radius * sin_theta * sin_phi, // X: longitude (left/right)
				-radius * cos_phi,			  // Y: latitude (up/down)
				radius * cos_theta * sin_phi  // Z: longitude (forward/back)
			);

			// Tangent
			glm::vec3 tangent = glm::vec3(
				radius * cos_theta, // ∂p/∂θ X	//We need to simplify
				0.0f,				// ∂p/∂θ Y
				-radius * sin_theta // ∂p/∂θ Z
			);

			// Binormal
			glm::vec3 binormal = glm::vec3(
				radius * sin_theta * cos_phi, // ∂p/∂ϕ X
				radius * sin_phi,			  // ∂p/∂ϕ Y
				radius * cos_theta * cos_phi  // ∂p/∂ϕ Z
			);
			// glm::vec3 normal = glm::normalize(vertex);
			glm::vec3 normal = glm::vec3(
				glm::cross(tangent, binormal));

			// Texture coordinates (u, v)
			glm::vec3 texcoord = glm::vec3(static_cast<float>(lon) / static_cast<float>(longitude_split_count),
										   static_cast<float>(lat) / static_cast<float>(latitude_split_count),
										   0.0f);

			// Store vertex, normal, tangent, binormal, and uv
			unsigned int index = lat * (longitude_split_count + 1) + lon;
			vertices[index] = vertex;
			normals[index] = normal;
			tangents[index] = tangent;
			binormals[index] = binormal;
			textcoords[index] = texcoord;
		}
	}
	index_sets = std::vector<glm::uvec3>(2u * longitude_split_count * latitude_split_count);
	size_t index = 0u;
	for (unsigned int lat = 0u; lat < latitude_split_count; ++lat)
	{
		for (unsigned int lon = 0u; lon < longitude_split_count; ++lon)
		{
			unsigned int first = lat * (longitude_split_count + 1) + lon;
			unsigned int second = first + longitude_split_count + 1;

			// First triangle of the quad (counter-clockwise winding)
			index_sets[index] = glm::uvec3(first, second, first + 1);
			++index;

			// Second triangle of the quad (counter-clockwise winding)
			index_sets[index] = glm::uvec3(second, second + 1, first + 1);
			++index;
		}
	}
	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);
	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(textcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size + normals_size + texcoords_size + tangents_size + binormals_size);

	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const *>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const *>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const *>(textcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const *>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const *>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const *>(index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

bonobo::mesh_data
parametric_shapes::createTorus(float const major_radius,
							   float const minor_radius,
							   unsigned int const major_split_count,
							   unsigned int const minor_split_count)
{
	//! \todo (Optional) Implement this function
	return bonobo::mesh_data();
}

bonobo::mesh_data
parametric_shapes::createCircleRing(float const radius,
									float const spread_length,
									unsigned int const circle_split_count,
									unsigned int const spread_split_count)
{
	auto const circle_slice_edges_count = circle_split_count + 1u;
	auto const spread_slice_edges_count = spread_split_count + 1u;
	auto const circle_slice_vertices_count = circle_slice_edges_count + 1u;
	auto const spread_slice_vertices_count = spread_slice_edges_count + 1u;
	auto const vertices_nb = circle_slice_vertices_count * spread_slice_vertices_count;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float const spread_start = radius - 0.5f * spread_length;
	float const d_theta = glm::two_pi<float>() / (static_cast<float>(circle_slice_edges_count));
	float const d_spread = spread_length / (static_cast<float>(spread_slice_edges_count));

	// generate vertices iteratively
	size_t index = 0u;
	float theta = 0.0f;
	for (unsigned int i = 0u; i < circle_slice_vertices_count; ++i)
	{
		float const cos_theta = std::cos(theta);
		float const sin_theta = std::sin(theta);

		float distance_to_centre = spread_start;
		for (unsigned int j = 0u; j < spread_slice_vertices_count; ++j)
		{
			// vertex
			vertices[index] = glm::vec3(distance_to_centre * cos_theta,
										distance_to_centre * sin_theta,
										0.0f);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(spread_slice_vertices_count)),
										 static_cast<float>(i) / (static_cast<float>(circle_slice_vertices_count)),
										 0.0f);

			// tangent
			auto const t = glm::vec3(cos_theta, sin_theta, 0.0f);
			tangents[index] = t;

			// binormal
			auto const b = glm::vec3(-sin_theta, cos_theta, 0.0f);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			distance_to_centre += d_spread;
			++index;
		}

		theta += d_theta;
	}

	// create index array
	auto index_sets = std::vector<glm::uvec3>(2u * circle_slice_edges_count * spread_slice_edges_count);

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < circle_slice_edges_count; ++i)
	{
		for (unsigned int j = 0u; j < spread_slice_edges_count; ++j)
		{
			index_sets[index] = glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
										   spread_slice_vertices_count * (i + 0u) + (j + 1u),
										   spread_slice_vertices_count * (i + 1u) + (j + 1u));
			++index;

			index_sets[index] = glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
										   spread_slice_vertices_count * (i + 1u) + (j + 1u),
										   spread_slice_vertices_count * (i + 1u) + (j + 0u));
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size + normals_size + texcoords_size + tangents_size + binormals_size);
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const *>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const *>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const *>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const *>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const *>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const *>(index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}
