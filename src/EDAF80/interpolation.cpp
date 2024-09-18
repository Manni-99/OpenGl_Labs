#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const &p0, glm::vec3 const &p1, float const x)
{
	return glm::mix(p0, p1, glm::clamp(x, 0.0f, 1.0f));
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const &p0, glm::vec3 const &p1,
                              glm::vec3 const &p2, glm::vec3 const &p3,
                              float const t, float const x)
{
    // Basis vector
    glm::vec4 xVec = glm::vec4(1.0f, x, x*x, x*x*x);

    // Catmull-Rom matrix
    glm::mat4 m = glm::transpose(glm::mat4(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                            glm::vec4(-t, 0.0f, t, 0.0f),
                            glm::vec4(2.0f*t, t-3.0f, 3.0f-2.0f*t, -t),
                            glm::vec4(-t, 2.0f-t, t-2.0f, t))); 


	glm::mat4x3 points = glm::mat4x3(p0, p1, p2, p3); 
   

    return  xVec * m * glm::transpose(points);
}
