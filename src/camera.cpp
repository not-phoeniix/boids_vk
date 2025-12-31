#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float aspect_ratio, float fov, float near, float far)
  : position(0.0f),
    rotation(0.0f),
    aspect_ratio(aspect_ratio),
    fov(fov),
    near(near),
    far(far) {
}

Camera::Camera(glm::vec3 position, float aspect_ratio, float fov, float near, float far)
  : position(position),
    rotation(0.0f),
    aspect_ratio(aspect_ratio),
    fov(fov),
    near(near),
    far(far) {
}

glm::mat4 Camera::get_view() const {
    // glm::vec3 forward(1, 0, 0);
    return glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::get_proj() const {
    glm::mat4 proj = glm::perspective(fov, aspect_ratio, near, far);

    // we gotta flip Y axis because GLM was originally designed
    //   for OpenGL which has its Y coordinate flipped <//3
    proj[1][1] *= -1;

    return proj;
}
