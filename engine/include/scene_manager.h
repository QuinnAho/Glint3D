#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "material.h"
#include "light.h"
#include "objloader.h"
#include "texture.h"
#include "shader.h"
#include <glint3d/rhi.h>
#include <glint3d/rhi_types.h>
#include "material_core.h"

using namespace glint3d;

struct SceneObject
{
    std::string name;
    unsigned int VAO = 0, VBO_positions = 0, VBO_normals = 0, VBO_uvs = 0, VBO_tangents = 0, EBO = 0;
    // RHI buffer handles (migration)
    BufferHandle rhiVboPositions = INVALID_HANDLE;
    BufferHandle rhiVboNormals = INVALID_HANDLE;
    BufferHandle rhiVboUVs = INVALID_HANDLE;
    BufferHandle rhiEbo = INVALID_HANDLE;
    PipelineHandle rhiPipelineBasic = INVALID_HANDLE;
    PipelineHandle rhiPipelinePbr = INVALID_HANDLE;
    glm::mat4 modelMatrix{ 1.0f };        // World transform (computed from hierarchy)
    
    // Hierarchy support
    int parentIndex = -1;  // -1 for root objects
    std::vector<int> childIndices;
    glm::mat4 localMatrix{ 1.0f };        // Local transform relative to parent

    ObjLoader objLoader;
    Texture* texture = nullptr;       // legacy diffuse
    Texture* baseColorTex = nullptr;  // PBR
    Texture* normalTex = nullptr;     // PBR
    Texture* mrTex = nullptr;         // PBR (metallic-roughness)
    Shader* shader = nullptr;

    bool      isStatic = false;
    glm::vec3 color{ 1.0f };
    
    // Unified material system - eliminates dual storage problem
    MaterialCore materialCore;
    
    // Legacy material - deprecated, kept temporarily for migration compatibility
    // TODO: Remove after full raytracer migration to MaterialCore (FEAT-0241 PR4)
    Material  material;
};

class SceneManager 
{
public:
    SceneManager();
    ~SceneManager();

    // Object management
    bool loadObject(const std::string& name, const std::string& path, 
                   const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f));
    bool removeObject(const std::string& name);
    bool duplicateObject(const std::string& sourceName, const std::string& newName,
                        const glm::vec3* deltaPos = nullptr,
                        const glm::vec3* deltaScale = nullptr, 
                        const glm::vec3* deltaRotDeg = nullptr);
    bool duplicateObject(const std::string& sourceName, const std::string& newName, const glm::vec3& newPosition);
    bool moveObject(const std::string& name, const glm::vec3& delta);
    
    // Hierarchy management
    bool reparentObject(int childIndex, int newParentIndex);
    bool reparentObject(const std::string& childName, const std::string& newParentName);
    std::vector<int> getParentIndices() const;
    
    // Transform hierarchy
    void updateWorldTransforms();
    void updateWorldTransform(int objectIndex);
    glm::mat4 getWorldMatrix(int objectIndex) const;
    void setLocalMatrix(int objectIndex, const glm::mat4& localMatrix);
    void setLocalMatrix(const std::string& name, const glm::mat4& localMatrix);
    glm::mat4 getLocalMatrix(int objectIndex) const;
    
    // Selection
    void setSelectedObjectIndex(int index) { m_selectedObjectIndex = index; }
    int getSelectedObjectIndex() const { return m_selectedObjectIndex; }
    std::string getSelectedObjectName() const;
    glm::vec3 getSelectedObjectCenterWorld() const;

    // Material management  
    bool createMaterial(const std::string& name, const Material& material);
    bool assignMaterialToObject(const std::string& objectName, const std::string& materialName);

    // Accessors
    const std::vector<SceneObject>& getObjects() const { return m_objects; }
    std::vector<SceneObject>& getObjects() { return m_objects; }
    SceneObject* findObjectByName(const std::string& name);
    const SceneObject* findObjectByName(const std::string& name) const;
    int findObjectIndex(const std::string& name) const;
    bool deleteObject(const std::string& name);

    // Serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);

    // Clear scene
    void clear();

private:
    RHI* m_rhi = nullptr; // not owned; provided by RenderSystem
    std::vector<SceneObject> m_objects;
    std::unordered_map<std::string, Material> m_materials;
    int m_selectedObjectIndex = -1;

    void setupObjectOpenGL(SceneObject& obj);
    void cleanupObjectOpenGL(SceneObject& obj);

public:
    // Inject RHI (called from Application once RenderSystem is initialized)
    void setRHI(RHI* rhi) { m_rhi = rhi; }
};
