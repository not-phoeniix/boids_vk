#pragma once

#include <glm/glm.hpp>

class Camera {
   private:
    glm::vec3 position;
    glm::vec3 rotation;
    float aspect_ratio;
    float fov;
    float near;
    float far;

   public:
    Camera(float aspect_ratio, float fov, float near, float far);
    Camera(glm::vec3 position, float aspect_ratio, float fov, float near, float far);

    void LookAt(glm::vec3 position);

    glm::mat4 get_view() const;
    glm::mat4 get_proj() const;

    glm::vec3 get_forward() const;
    glm::vec3 get_up() const;
    glm::vec3 get_right() const;

    float get_fov() const { return fov; }
    float get_aspect() const { return aspect_ratio; }
    void set_fov(float fov) { this->fov = fov; }
    void set_aspect(float aspect_ratio) { this->aspect_ratio = aspect_ratio; }
};
