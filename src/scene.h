#pragma once

#include "graphics/graphics_manager.h"
#include "graphics/mesh.h"
#include <memory>
#include "camera.h"
#include <vector>
#include "glm_settings.h"
#include <glm/glm.hpp>
#include "graphics/image_wrapper.h"
#include "graphics/sampler_wrapper.h"

class Scene {
   private:
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<ImageWrapper> image;
    std::unique_ptr<SamplerWrapper> sampler;
    std::unique_ptr<Camera> camera;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> rotations;
    std::vector<glm::vec3> rot_speeds;
    std::vector<std::shared_ptr<UniformWrapper>> uniforms;
    float time = 0.0f;

   public:
    Scene() = default;

    void Init(GraphicsManager& graphics);
    void Deinit();

    void Update(float dt);
    void Draw(GraphicsManager& graphics);
};
