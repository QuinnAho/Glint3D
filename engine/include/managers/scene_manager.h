// Machine Summary Block (ndjson)
// {"file":"engine/include/managers/scene_manager.h","purpose":"Manages scene objects, hierarchy, and material assignments for Glint3D","exports":["SceneObject","SceneManager"],"depends_on":["glint3d::RHI","MaterialCore","glm"],"notes":["Maintains object hierarchy and world transforms","Builds GPU buffers through the RHI","Maps materials by name for reuse"]}
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "../light.h"
#include "../objloader.h"
#include "../texture.h"
#include "../shader.h"
#include <glint3d/rhi.h>
#include <glint3d/rhi_types.h>
#include "../material_core.h"

/**
 * @file scene_manager.h
 * @brief Stores scene objects, maintains hierarchy transforms, and owns material assignments.
 *
 * SceneManager bridges asset loading into RHI buffers, tracks per-object transforms, and
 * provides lookup hooks for editor tooling and render passes.
 */

using glint3d::BufferHandle;
using glint3d::INVALID_HANDLE;
using glint3d::PipelineHandle;
using glint3d::RHI;

struct SceneObject
{
    std::string name;
    // rhi buffer handles
    BufferHandle rhiVboPositions = INVALID_HANDLE;
    BufferHandle rhiVboNormals = INVALID_HANDLE;
    BufferHandle rhiVboTexCoords = INVALID_HANDLE;
    BufferHandle rhiVboTangents = INVALID_HANDLE;   // for pbr tangent data
    BufferHandle rhiEbo = INVALID_HANDLE;
    PipelineHandle rhiPipelineBasic = INVALID_HANDLE;   // basic shader pipeline
    PipelineHandle rhiPipelinePbr = INVALID_HANDLE;     // pbr shader pipeline
    PipelineHandle rhiPipelineGBuffer = INVALID_HANDLE; // deferred g-buffer pipeline
    glm::mat4 modelMatrix{ 1.0f };        // world transform (computed from hierarchy)
    
    // hierarchy support
    int parentIndex = -1;  // -1 for root objects
    std::vector<int> childIndices;
    glm::mat4 localMatrix{ 1.0f };        // local transform relative to parent

    ObjLoader objLoader;
    Texture* texture = nullptr;       // legacy diffuse
    Texture* baseColorTex = nullptr;  // pbr
    Texture* normalTex = nullptr;     // pbr
    Texture* mrTex = nullptr;         // pbr (metallic-roughness)
    Shader* shader = nullptr;

    bool      isStatic = false;
    glm::vec3 color{ 1.0f };
    
    // unified material system - single source of truth (ACTIVE)
    MaterialCore materialCore;

};

class SceneManager 
{
public:
    SceneManager();
    ~SceneManager();

    // object management
    bool loadObject(const std::string& name, const std::string& path, 
                   const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));
    bool removeObject(const std::string& name);
    bool duplicateObject(const std::string& sourceName, const std::string& newName,
                        const glm::vec3* deltaPos = nullptr,
                        const glm::vec3* deltaScale = nullptr, 
                        const glm::vec3* deltaRotDeg = nullptr);
    bool duplicateObject(const std::string& sourceName, const std::string& newName, const glm::vec3& newPosition);
    bool moveObject(const std::string& name, const glm::vec3& delta);
    
    // hierarchy management
    bool reparentObject(int childIndex, int newParentIndex);
    bool reparentObject(const std::string& childName, const std::string& newParentName);
    std::vector<int> getParentIndices() const;
    
    // transform hierarchy
    void updateWorldTransforms();
    void updateWorldTransform(int objectIndex);
    glm::mat4 getWorldMatrix(int objectIndex) const;
    void setLocalMatrix(int objectIndex, const glm::mat4& localMatrix);
    void setLocalMatrix(const std::string& name, const glm::mat4& localMatrix);
    glm::mat4 getLocalMatrix(int objectIndex) const;
    
    // selection
    void setSelectedObjectIndex(int index) { m_selectedObjectIndex = index; }
    int getSelectedObjectIndex() const { return m_selectedObjectIndex; }
    std::string getSelectedObjectName() const;
    glm::vec3 getSelectedObjectCenterWorld() const;

    // material management (using MaterialCore)
    bool createMaterial(const std::string& name, const MaterialCore& material);
    bool assignMaterialToObject(const std::string& objectName, const std::string& materialName);

    // accessors
    const std::vector<SceneObject>& getObjects() const { return m_objects; }
    std::vector<SceneObject>& getObjects() { return m_objects; }
    SceneObject* findObjectByName(const std::string& name);
    const SceneObject* findObjectByName(const std::string& name) const;
    int findObjectIndex(const std::string& name) const;
    bool deleteObject(const std::string& name);

    // serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);

    // clear scene
    void clear();

private:
    RHI* m_rhi = nullptr; // not owned; provided by RenderSystem
    std::vector<SceneObject> m_objects;
    std::unordered_map<std::string, MaterialCore> m_materials;
    int m_selectedObjectIndex = -1;

    void setupObjectOpenGL(SceneObject& obj);
    void cleanupObjectOpenGL(SceneObject& obj);

public:
    // inject rhi (called from application once RenderSystem is initialized)
    void setRHI(RHI* rhi) { m_rhi = rhi; }
};


