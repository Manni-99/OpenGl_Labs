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
    glm::mat4 m = glm::mat4(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                            glm::vec4(-t, 0.0f, t, 0.0f),
                            glm::vec4(2.0f*t, t-3.0f, 3.0f-2.0f*t, -t),
                            glm::vec4(-t, 2.0f-t, t-2.0f, t)); 

    // Control points in homogeneous coordinates
    glm::vec4 p0Vec = glm::vec4(p0, 1.0f);
    glm::vec4 p1Vec = glm::vec4(p1, 1.0f);
    glm::vec4 p2Vec = glm::vec4(p2, 1.0f);
    glm::vec4 p3Vec = glm::vec4(p3, 1.0f);

    // Create a matrix with control points as columns
    glm::mat4 controlPoints;
    controlPoints[0] = p0Vec;
    controlPoints[1] = p1Vec;
    controlPoints[2] = p2Vec;
    controlPoints[3] = p3Vec;

    // Compute the interpolated position
    glm::vec4 resultVec = xVec * (m * controlPoints);

    // Return the result as glm::vec3
    return glm::vec3(resultVec);
}
