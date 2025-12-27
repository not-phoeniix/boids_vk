#pragma once

#include "graphics_manager.h"
#include "mesh.h"
#include <memory>

class Scene {
   private:
    std::shared_ptr<Mesh> mesh;

   public:
    Scene() = default;

    void Init(const GraphicsManager& graphics);
    void Deinit();

    void Update(float dt);
    void Draw(const GraphicsManager& graphics);
};
