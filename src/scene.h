#pragma once

#include "render_thing/graphics_manager.h"
#include "render_thing/mesh.h"
#include <memory>
#include "camera.h"
#include <vector>
#include "render_thing/glm_settings.h"
#include <glm/glm.hpp>
#include "render_thing/image_wrapper.h"
#include "render_thing/sampler_wrapper.h"
#include "boid_system.h"

class Scene {
   private:
    std::shared_ptr<RenderThing::Mesh> mesh;
    std::shared_ptr<RenderThing::ImageWrapper> image;
    std::unique_ptr<RenderThing::SamplerWrapper> sampler;
    std::unique_ptr<Camera> camera;
    BoidSystem boid_system;
    std::vector<std::shared_ptr<RenderThing::UniformWrapper>> uniforms;
    float time = 0.0f;

   public:
    Scene() = default;

    void Init(RenderThing::GraphicsManager& graphics);
    void Deinit();

    void Update(float dt);
    void Draw(RenderThing::GraphicsManager& graphics);
};
