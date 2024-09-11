#include "CelestialBody.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "core/helpers.hpp"
#include "core/Log.h"

CelestialBody::CelestialBody(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id)
{
	_body.node.set_geometry(shape);
	_body.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	_body.node.set_program(program);
}

glm::mat4 CelestialBody::render(std::chrono::microseconds elapsed_time,
                                glm::mat4 const& view_projection,
                                glm::mat4 const& parent_transform,
                                bool show_basis)
{
    // Convert the duration from microseconds to seconds.
    auto const elapsed_time_s = std::chrono::duration<float>(elapsed_time).count();

    // Update spin and orbit angles based on elapsed time
    _body.spin.rotation_angle += _body.spin.speed * elapsed_time_s;
    _body.orbit.rotation_angle += _body.orbit.speed * elapsed_time_s;
 

    // Initialize the world matrix with the parent transform
    glm::mat4 world = parent_transform;

    // Tilt the orbit plane around the z-axis by the orbit inclination (R2,o)
    glm::mat4 R2_o = glm::rotate(glm::mat4(1.0f), _body.orbit.inclination, glm::vec3(0.0f, 0.0f, 1.0f));

    // Orbit rotation: rotate around the y-axis (R1,o) using the updated orbit angle
    glm::mat4 R1_o = glm::rotate(glm::mat4(1.0f), _body.orbit.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));

    // Translation: move the celestial body along its orbit radius
    glm::mat4 T_orbit = glm::translate(glm::mat4(1.0f), glm::vec3(_body.orbit.radius, 0.0f, 0.0f));	//Changed the y and z value so that the moon wouldn't crash into the earth

    // Combine orbit R2_o, R1_o and translation (orbit transformation)
    glm::mat4 orbit_transform = R2_o * R1_o * T_orbit;

    // Apply the orbit transformations: rotate first, then translate
    world *= orbit_transform;

    // Apply any scaling / transformations to the globe (if needed)
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(_body.scale));
    world *= scale;

    // First rotation: spin around the y-axis (R1,s) using the updated spin angle
    glm::mat4 R1_s = glm::rotate(glm::mat4(1.0f), _body.spin.rotation_angle, glm::vec3(0.0f, 1.0f, 0.0f));

    // Second rotation: tilt the spin plane around the z-axis (R2,s)
    glm::mat4 R2_s = glm::rotate(glm::mat4(1.0f), _body.spin.axial_tilt, glm::vec3(0.0f, 0.0f, 1.0f));

    // Combine the spin tilt and rotation
    world *= R2_s * R1_s;
	
	//glm::mat4 scale_child = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

	// Compute the matrix for the children (excluding scale and spin rotation)
	glm::mat4 child_transform = parent_transform * orbit_transform;
	// Adjustment of the scale of child node
	//child_transform *= scale_child;
    
  //  glm::mat4 child_transform = R2_o * R1_o * T_orbit;

    // Render the basis if needed
    if (show_basis) {
        bonobo::renderBasis(1.0f, 2.0f, view_projection, world);
    }
	 //_body.node.render(view_projection, world);
    // Render the body using the updated world matrix
    _body.node.render(view_projection, world);

    // Return the matrix for the children (computed without scale and spin rotation)
    return child_transform;
}

void CelestialBody::add_child(CelestialBody* child)
{
	_children.push_back(child);
}

std::vector<CelestialBody*> const& CelestialBody::get_children() const
{
	return _children;
}

void CelestialBody::set_orbit(OrbitConfiguration const& configuration)
{
	_body.orbit.radius = configuration.radius;
	_body.orbit.inclination = configuration.inclination;
	_body.orbit.speed = configuration.speed;
	_body.orbit.rotation_angle = 0.0f;
}

void CelestialBody::set_scale(glm::vec3 const& scale)
{
	_body.scale = scale;
}

void CelestialBody::set_spin(SpinConfiguration const& configuration)
{
	_body.spin.axial_tilt = configuration.axial_tilt;
	_body.spin.speed = configuration.speed;
	_body.spin.rotation_angle = 0.0f;
}

void CelestialBody::set_ring(bonobo::mesh_data const& shape,
                             GLuint const* program,
                             GLuint diffuse_texture_id,
                             glm::vec2 const& scale)
{
	_ring.node.set_geometry(shape);
	_ring.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	_ring.node.set_program(program);

	_ring.scale = scale;

	_ring.is_set = true;
}
/*if (!get_children().empty()) {	// Dont think this is necessary since at the moment we call the render function a couple of time in the main class 
    std::vector<CelestialBody*> const& children = get_children();
    for (size_t i = 0; i < children.size(); ++i) {
        CelestialBody* child = children[i];

        // Apply the child_transform matrix to the child
        if (child != nullptr) {
            child->render(elapsed_time, view_projection, child_transform, show_basis);
        }
    }
}*/