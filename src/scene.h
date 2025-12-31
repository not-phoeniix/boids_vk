#pragma once

#include "graphics_manager.h"
#include "mesh.h"
#include <memory>
#include "camera.h"

class Scene {
   private:
    std::shared_ptr<Mesh> mesh;
    std::unique_ptr<Camera> camera;
    float time = 0.0f;

   public:
    Scene() = default;

    void Init(GraphicsManager& graphics);
    void Deinit();

    void Update(float dt);
    void Draw(GraphicsManager& graphics);
};
