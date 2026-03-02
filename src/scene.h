#pragma once

#include "render_thing/render_thing.h"
#include <memory>
#include "camera.h"
#include <vector>
#include <glm/glm.hpp>
#include "boid_system.h"

class Scene {
   private:
    std::shared_ptr<RenderThing::Mesh> mesh;
    std::shared_ptr<RenderThing::Image> image;
    std::unique_ptr<Camera> camera;
    BoidSystem boid_system;
    float time = 0.0f;

   public:
    Scene();
    ~Scene();

    void Update(float dt);
    void Draw();
};
