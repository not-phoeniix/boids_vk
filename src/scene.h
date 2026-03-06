#pragma once

#include "render_thing/render_thing.h"
#include <memory>
#include "camera.h"
#include <vector>
#include <glm/glm.hpp>
#include "boid_system.h"
#include "light.h"

class Scene {
   private:
    std::shared_ptr<rt::Mesh> mesh;
    std::shared_ptr<rt::Image> image;
    std::unique_ptr<Camera> camera;
    std::vector<Light> lights;
    BoidSystem boid_system;
    float time = 0.0f;
    
    void RandomizeLights();

   public:
    Scene();
    ~Scene();

    void Update(float dt);
    void Draw();
};
