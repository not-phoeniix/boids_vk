#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

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

void Camera::LookAt(const glm::vec3& target) {
    glm::vec3 delta = target - position;
    if (glm::length2(delta) > FLT_EPSILON) {
        delta = glm::normalize(delta);
    }

    float yaw = atan2f(delta.x, delta.z);
    float pitch = asinf(-delta.y);
    rotation = glm::vec3(pitch, yaw, 0);
}

glm::mat4 Camera::get_view() const {
    return glm::lookAt(
        position,
        position + get_forward(),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

glm::mat4 Camera::get_proj() const {
    glm::mat4 proj = glm::perspective(fov, aspect_ratio, near, far);

    // we gotta flip Y axis because GLM was originally designed
    //   for OpenGL which has its Y coordinate flipped <//3
    proj[1][1] *= -1;

    return proj;
}

glm::vec3 Camera::get_forward() const {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec4 forward = mat * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

    return glm::vec3(forward);
}

glm::vec3 Camera::get_up() const {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec4 up = mat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

    return glm::vec3(up);
}

glm::vec3 Camera::get_right() const {
    glm::mat4 mat = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec4 right = mat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

    return glm::vec3(right);
}
