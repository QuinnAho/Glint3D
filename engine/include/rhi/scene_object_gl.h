#pragma once

struct SceneObject;

namespace rhi {
void setupSceneObjectGL(SceneObject& obj);
void cleanupSceneObjectGL(SceneObject& obj);
}
