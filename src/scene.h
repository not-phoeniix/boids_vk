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
    std::unique_ptr<RenderThing::Sampler> sampler;
    std::unique_ptr<Camera> camera;
    BoidSystem boid_system;
    std::vector<std::shared_ptr<RenderThing::Uniform>> uniforms;
    float time = 0.0f;

   public:
    Scene() = default;

    void Init(RenderThing::GraphicsManager& graphics);
    void Deinit();

    void Update(float dt);
    void Draw(RenderThing::GraphicsManager& graphics);
};
