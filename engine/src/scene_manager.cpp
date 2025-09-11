#include "scene_manager.h"
#include "gl_platform.h"
#include "mesh_loader.h"
#include "texture_cache.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <cmath>

SceneManager::SceneManager() 
{
}

SceneManager::~SceneManager() 
{
    clear();
}

bool SceneManager::loadObject(const std::string& name, const std::string& path, 
                             const glm::vec3& position, const glm::vec3& scale)
{
    // Check if object with this name already exists
    if (findObjectByName(name) != nullptr) {
        std::cerr << "Object with name '" << name << "' already exists\n";
        return false;
    }

    SceneObject obj;
    obj.name = name;
    
    // Load mesh data
    obj.objLoader.load(path.c_str());

    // Set transform (initially same for both local and world since it's a root object)
    glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
    obj.localMatrix = translateMat * scaleMat;
    obj.modelMatrix = obj.localMatrix;  // World = local for root objects

    // Setup OpenGL resources
    setupObjectOpenGL(obj);

    // Load textures if they exist
    std::string directory = path.substr(0, path.find_last_of('/'));
    if (directory == path) directory = "."; // No directory found
    
    // Try to find and load associated textures
    TextureCache& texCache = TextureCache::instance();
    
    // Look for common texture naming patterns
    std::string baseName = path.substr(path.find_last_of('/') + 1);
    baseName = baseName.substr(0, baseName.find_last_of('.'));
    
    // Try diffuse/albedo texture
    std::vector<std::string> diffuseNames = {
        directory + "/" + baseName + "_diffuse.png",
        directory + "/" + baseName + "_albedo.png", 
        directory + "/" + baseName + "_basecolor.png",
        directory + "/" + baseName + ".png",
        directory + "/" + baseName + ".jpg"
    };
    
    for (const auto& texPath : diffuseNames) {
        if (std::ifstream(texPath).good()) {
            obj.baseColorTex = texCache.get(texPath, false);
            obj.texture = obj.baseColorTex; // legacy fallback
            break;
        }
    }
    
    m_objects.push_back(std::move(obj));
    return true;
}

bool SceneManager::removeObject(const std::string& name)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    
    if (it == m_objects.end()) {
        return false;
    }
    
    int index = std::distance(m_objects.begin(), it);
    
    // Reparent children to the object's parent before removing
    int parentIndex = it->parentIndex;
    for (int childIndex : it->childIndices) {
        if (childIndex < static_cast<int>(m_objects.size())) {
            m_objects[childIndex].parentIndex = parentIndex;
            
            // Add children to the parent's child list
            if (parentIndex != -1) {
                m_objects[parentIndex].childIndices.push_back(childIndex);
            }
        }
    }
    
    // Remove from parent's children list
    if (it->parentIndex != -1) {
        SceneObject& parent = m_objects[it->parentIndex];
        auto parentIt = std::find(parent.childIndices.begin(), parent.childIndices.end(), index);
        if (parentIt != parent.childIndices.end()) {
            parent.childIndices.erase(parentIt);
        }
    }
    
    cleanupObjectOpenGL(*it);
    
    // Update selection if removing selected object
    if (m_selectedObjectIndex == index) {
        m_selectedObjectIndex = -1;
    } else if (m_selectedObjectIndex > index) {
        m_selectedObjectIndex--;
    }
    
    // Update all indices after removal
    for (auto& obj : m_objects) {
        // Update parent index
        if (obj.parentIndex > index) {
            obj.parentIndex--;
        }
        
        // Update child indices
        for (auto& childIdx : obj.childIndices) {
            if (childIdx > index) {
                childIdx--;
            }
        }
    }
    
    m_objects.erase(it);
    return true;
}

bool SceneManager::duplicateObject(const std::string& sourceName, const std::string& newName,
                                  const glm::vec3* deltaPos, const glm::vec3* deltaScale, 
                                  const glm::vec3* deltaRotDeg)
{
    const SceneObject* source = findObjectByName(sourceName);
    if (!source) {
        return false;
    }
    
    if (findObjectByName(newName) != nullptr) {
        std::cerr << "Object with name '" << newName << "' already exists\n";
        return false;
    }
    
    SceneObject newObj = *source; // Copy construct
    newObj.name = newName;
    
    // Apply deltas to transform
    if (deltaPos || deltaScale || deltaRotDeg) {
        glm::mat4 transform = newObj.modelMatrix;
        
        if (deltaPos) {
            transform = glm::translate(transform, *deltaPos);
        }
        
        if (deltaRotDeg) {
            transform = glm::rotate(transform, glm::radians(deltaRotDeg->x), glm::vec3(1,0,0));
            transform = glm::rotate(transform, glm::radians(deltaRotDeg->y), glm::vec3(0,1,0)); 
            transform = glm::rotate(transform, glm::radians(deltaRotDeg->z), glm::vec3(0,0,1));
        }
        
        if (deltaScale) {
            transform = glm::scale(transform, *deltaScale);
        }
        
        newObj.localMatrix = transform;
        newObj.modelMatrix = transform;  // For root objects, world = local
    }
    
    // Setup new OpenGL resources (don't share VAO/VBO)
    setupObjectOpenGL(newObj);
    
    m_objects.push_back(std::move(newObj));
    return true;
}

bool SceneManager::moveObject(const std::string& name, const glm::vec3& delta)
{
    SceneObject* obj = findObjectByName(name);
    if (!obj) {
        return false;
    }
    
    // Move the local transform and update world transforms
    obj->localMatrix = glm::translate(obj->localMatrix, delta);
    int index = findObjectIndex(name);
    updateWorldTransform(index);
    return true;
}

std::string SceneManager::getSelectedObjectName() const
{
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_objects.size()) {
        return m_objects[m_selectedObjectIndex].name;
    }
    return "";
}

glm::vec3 SceneManager::getSelectedObjectCenterWorld() const
{
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_objects.size()) {
        const SceneObject& obj = m_objects[m_selectedObjectIndex];
        // Extract translation from model matrix
        return glm::vec3(obj.modelMatrix[3]);
    }
    return glm::vec3(0.0f);
}

bool SceneManager::createMaterial(const std::string& name, const Material& material)
{
    m_materials[name] = material;
    return true;
}

bool SceneManager::assignMaterialToObject(const std::string& objectName, const std::string& materialName)
{
    SceneObject* obj = findObjectByName(objectName);
    if (!obj) {
        return false;
    }
    
    auto it = m_materials.find(materialName);
    if (it == m_materials.end()) {
        return false;
    }
    
    obj->material = it->second;
    return true;
}

SceneObject* SceneManager::findObjectByName(const std::string& name)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    return (it != m_objects.end()) ? &(*it) : nullptr;
}

const SceneObject* SceneManager::findObjectByName(const std::string& name) const
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    return (it != m_objects.end()) ? &(*it) : nullptr;
}

int SceneManager::findObjectIndex(const std::string& name) const
{
    for (size_t i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool SceneManager::deleteObject(const std::string& name)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](const SceneObject& obj) { return obj.name == name; });
    
    if (it != m_objects.end()) {
        // Clean up OpenGL resources
        cleanupObjectOpenGL(*it);
        
        // Update selected index if needed
        int deletedIndex = static_cast<int>(std::distance(m_objects.begin(), it));
        if (m_selectedObjectIndex == deletedIndex) {
            m_selectedObjectIndex = -1;
        } else if (m_selectedObjectIndex > deletedIndex) {
            m_selectedObjectIndex--;
        }
        
        // Remove from vector
        m_objects.erase(it);
        return true;
    }
    return false;
}

bool SceneManager::duplicateObject(const std::string& sourceName, const std::string& newName, const glm::vec3& newPosition)
{
    const SceneObject* source = findObjectByName(sourceName);
    if (!source) {
        return false;
    }
    
    if (findObjectByName(newName) != nullptr) {
        std::cerr << "Object with name '" << newName << "' already exists\n";
        return false;
    }
    
    // Copy the source object
    SceneObject newObj = *source;
    newObj.name = newName;
    
    // Set new position (for root objects, local = world)
    newObj.localMatrix[3] = glm::vec4(newPosition, 1.0f);
    newObj.modelMatrix[3] = glm::vec4(newPosition, 1.0f);
    
    // Clear OpenGL handles (they will be regenerated)
    newObj.VAO = 0;
    newObj.VBO_positions = 0;
    newObj.VBO_normals = 0;
    newObj.VBO_uvs = 0;
    newObj.VBO_tangents = 0;
    newObj.EBO = 0;
    
    // Add to scene
    m_objects.push_back(newObj);
    
    // Setup OpenGL for the new object
    setupObjectOpenGL(m_objects.back());
    
    return true;
}

void SceneManager::clear()
{
    for (auto& obj : m_objects) {
        cleanupObjectOpenGL(obj);
    }
    m_objects.clear();
    m_materials.clear();
    m_selectedObjectIndex = -1;
}

void SceneManager::setupObjectOpenGL(SceneObject& obj)
{
    if (obj.objLoader.getVertCount() == 0) {
        return;
    }

    const size_t Nv = obj.objLoader.getVertCount();
    const bool hasNormals = (obj.objLoader.getNormals() != nullptr);
    const bool hasUVs = obj.objLoader.hasTexcoords();

    if (!m_rhi) {
        // Legacy GL path
        glGenVertexArrays(1, &obj.VAO);
        glGenBuffers(1, &obj.VBO_positions);
        if (hasNormals) glGenBuffers(1, &obj.VBO_normals);
        if (hasUVs) glGenBuffers(1, &obj.VBO_uvs);
        if (obj.objLoader.getIndexCount() > 0) glGenBuffers(1, &obj.EBO);

        glBindVertexArray(obj.VAO);

        // Positions
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_positions);
        glBufferData(GL_ARRAY_BUFFER, Nv * 3 * sizeof(float), obj.objLoader.getPositions(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        // Normals
        if (hasNormals) {
            glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_normals);
            glBufferData(GL_ARRAY_BUFFER, Nv * 3 * sizeof(float), obj.objLoader.getNormals(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);
        }

        // UVs
        if (hasUVs) {
            glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_uvs);
            glBufferData(GL_ARRAY_BUFFER, Nv * 2 * sizeof(float), obj.objLoader.getTexcoords(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);
        }

        // Indices
        if (obj.objLoader.getIndexCount() > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.objLoader.getIndexCount() * sizeof(unsigned int),
                         obj.objLoader.getFaces(), GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
        return;
    }

    // RHI path: create buffers
    {
        BufferDesc bd{}; bd.type = BufferType::Vertex; bd.usage = BufferUsage::Static;
        bd.size = Nv * 3 * sizeof(float); bd.initialData = obj.objLoader.getPositions();
        bd.debugName = obj.name + ":positions";
        obj.rhiVboPositions = m_rhi->createBuffer(bd);
    }
    if (hasNormals) {
        BufferDesc bd{}; bd.type = BufferType::Vertex; bd.usage = BufferUsage::Static;
        bd.size = Nv * 3 * sizeof(float); bd.initialData = obj.objLoader.getNormals();
        bd.debugName = obj.name + ":normals";
        obj.rhiVboNormals = m_rhi->createBuffer(bd);
    }
    if (hasUVs) {
        BufferDesc bd{}; bd.type = BufferType::Vertex; bd.usage = BufferUsage::Static;
        bd.size = Nv * 2 * sizeof(float); bd.initialData = obj.objLoader.getTexcoords();
        bd.debugName = obj.name + ":uvs";
        obj.rhiVboUVs = m_rhi->createBuffer(bd);
    }
    if (obj.objLoader.getIndexCount() > 0) {
        BufferDesc bd{}; bd.type = BufferType::Index; bd.usage = BufferUsage::Static;
        bd.size = obj.objLoader.getIndexCount() * sizeof(unsigned int);
        bd.initialData = obj.objLoader.getFaces();
        bd.debugName = obj.name + ":indices";
        obj.rhiEbo = m_rhi->createBuffer(bd);
    }

    // Do not build pipelines here; RenderSystem will build per-shader pipelines
}

void SceneManager::cleanupObjectOpenGL(SceneObject& obj)
{
    if (obj.VAO) {
        glDeleteVertexArrays(1, &obj.VAO);
        obj.VAO = 0;
    }
    if (obj.VBO_positions) {
        glDeleteBuffers(1, &obj.VBO_positions);
        obj.VBO_positions = 0;
    }
    if (m_rhi && obj.rhiVboPositions != INVALID_HANDLE) { m_rhi->destroyBuffer(obj.rhiVboPositions); obj.rhiVboPositions = INVALID_HANDLE; }
    if (obj.VBO_normals) {
        glDeleteBuffers(1, &obj.VBO_normals);
        obj.VBO_normals = 0;
    }
    if (m_rhi && obj.rhiVboNormals != INVALID_HANDLE) { m_rhi->destroyBuffer(obj.rhiVboNormals); obj.rhiVboNormals = INVALID_HANDLE; }
    if (obj.VBO_uvs) {
        glDeleteBuffers(1, &obj.VBO_uvs);
        obj.VBO_uvs = 0;
    }
    if (m_rhi && obj.rhiVboUVs != INVALID_HANDLE) { m_rhi->destroyBuffer(obj.rhiVboUVs); obj.rhiVboUVs = INVALID_HANDLE; }
    if (obj.VBO_tangents) {
        glDeleteBuffers(1, &obj.VBO_tangents);
        obj.VBO_tangents = 0;
    }
    if (obj.EBO) {
        glDeleteBuffers(1, &obj.EBO);
        obj.EBO = 0;
    }
    if (m_rhi && obj.rhiEbo != INVALID_HANDLE) { m_rhi->destroyBuffer(obj.rhiEbo); obj.rhiEbo = INVALID_HANDLE; }
    if (m_rhi && obj.rhiPipelineBasic != INVALID_HANDLE) { m_rhi->destroyPipeline(obj.rhiPipelineBasic); obj.rhiPipelineBasic = INVALID_HANDLE; }
    if (m_rhi && obj.rhiPipelinePbr != INVALID_HANDLE) { m_rhi->destroyPipeline(obj.rhiPipelinePbr); obj.rhiPipelinePbr = INVALID_HANDLE; }
}

std::string SceneManager::toJson() const
{
    using namespace rapidjson;
    
    Document doc;
    doc.SetObject();
    Document::AllocatorType& allocator = doc.GetAllocator();
    
    // Create objects array
    Value objects(kArrayType);
    
    for (const auto& obj : m_objects) {
        Value objVal(kObjectType);
        
        // Add object name
        Value nameVal(obj.name.c_str(), allocator);
        objVal.AddMember("name", nameVal, allocator);
        
        // Add transform
        Value transform(kObjectType);
        
        // Extract position from model matrix
        glm::vec3 pos = glm::vec3(obj.modelMatrix[3]);
        Value position(kArrayType);
        position.PushBack(pos.x, allocator);
        position.PushBack(pos.y, allocator);
        position.PushBack(pos.z, allocator);
        transform.AddMember("position", position, allocator);
        
        // Extract scale from model matrix
        glm::vec3 scale(
            glm::length(glm::vec3(obj.modelMatrix[0])),
            glm::length(glm::vec3(obj.modelMatrix[1])),
            glm::length(glm::vec3(obj.modelMatrix[2]))
        );
        Value scaleVal(kArrayType);
        scaleVal.PushBack(scale.x, allocator);
        scaleVal.PushBack(scale.y, allocator);
        scaleVal.PushBack(scale.z, allocator);
        transform.AddMember("scale", scaleVal, allocator);
        
        // Extract rotation (Euler angles from normalized matrix)
        glm::mat3 rotMatrix(
            glm::vec3(obj.modelMatrix[0]) / scale.x,
            glm::vec3(obj.modelMatrix[1]) / scale.y,
            glm::vec3(obj.modelMatrix[2]) / scale.z
        );
        
        // Convert rotation matrix to Euler angles (simplified)
        float rotY = asin(glm::clamp(rotMatrix[0][2], -1.0f, 1.0f));
        float rotX, rotZ;
        if (cos(rotY) > 0.0001f) {
            rotX = atan2(-rotMatrix[1][2], rotMatrix[2][2]);
            rotZ = atan2(-rotMatrix[0][1], rotMatrix[0][0]);
        } else {
            rotX = atan2(rotMatrix[2][1], rotMatrix[1][1]);
            rotZ = 0;
        }
        
        Value rotation(kArrayType);
        rotation.PushBack(glm::degrees(rotX), allocator);
        rotation.PushBack(glm::degrees(rotY), allocator);
        rotation.PushBack(glm::degrees(rotZ), allocator);
        transform.AddMember("rotation", rotation, allocator);
        
        objVal.AddMember("transform", transform, allocator);
        
        // Add material information from object's built-in material
        Value material(kObjectType);
        
        Value diffuse(kArrayType);
        diffuse.PushBack(obj.material.diffuse.x, allocator);
        diffuse.PushBack(obj.material.diffuse.y, allocator);
        diffuse.PushBack(obj.material.diffuse.z, allocator);
        material.AddMember("diffuse", diffuse, allocator);
        
        Value specular(kArrayType);
        specular.PushBack(obj.material.specular.x, allocator);
        specular.PushBack(obj.material.specular.y, allocator);
        specular.PushBack(obj.material.specular.z, allocator);
        material.AddMember("specular", specular, allocator);
        
        material.AddMember("shininess", obj.material.shininess, allocator);
        material.AddMember("roughness", obj.material.roughness, allocator);
        material.AddMember("metallic", obj.material.metallic, allocator);
        
        Value ambient(kArrayType);
        ambient.PushBack(obj.material.ambient.x, allocator);
        ambient.PushBack(obj.material.ambient.y, allocator);
        ambient.PushBack(obj.material.ambient.z, allocator);
        material.AddMember("ambient", ambient, allocator);
        
        objVal.AddMember("material", material, allocator);
        
        objects.PushBack(objVal, allocator);
    }
    
    doc.AddMember("objects", objects, allocator);
    
    // Add scene metadata
    Value metadata(kObjectType);
    metadata.AddMember("objectCount", (int)m_objects.size(), allocator);
    metadata.AddMember("selectedIndex", m_selectedObjectIndex, allocator);
    doc.AddMember("metadata", metadata, allocator);
    
    // Convert to string
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return buffer.GetString();
}

bool SceneManager::reparentObject(int childIndex, int newParentIndex)
{
    // Validate indices
    if (childIndex < 0 || childIndex >= static_cast<int>(m_objects.size())) {
        std::cerr << "Invalid child index: " << childIndex << std::endl;
        return false;
    }
    
    if (newParentIndex != -1 && (newParentIndex < 0 || newParentIndex >= static_cast<int>(m_objects.size()))) {
        std::cerr << "Invalid parent index: " << newParentIndex << std::endl;
        return false;
    }
    
    // Prevent self-parenting
    if (childIndex == newParentIndex) {
        std::cerr << "Cannot parent object to itself" << std::endl;
        return false;
    }
    
    // Prevent cyclic parenting (child cannot become parent of its ancestor)
    if (newParentIndex != -1) {
        int ancestor = newParentIndex;
        while (ancestor != -1) {
            if (ancestor == childIndex) {
                std::cerr << "Cyclic parenting detected - operation would create a cycle" << std::endl;
                return false;
            }
            ancestor = m_objects[ancestor].parentIndex;
        }
    }
    
    SceneObject& child = m_objects[childIndex];
    
    // Preserve world transform by converting to appropriate local transform
    glm::mat4 currentWorldMatrix = child.modelMatrix;
    
    // Remove from old parent's children list
    if (child.parentIndex != -1) {
        SceneObject& oldParent = m_objects[child.parentIndex];
        auto it = std::find(oldParent.childIndices.begin(), oldParent.childIndices.end(), childIndex);
        if (it != oldParent.childIndices.end()) {
            oldParent.childIndices.erase(it);
        }
    }
    
    // Set new parent
    child.parentIndex = newParentIndex;
    
    // Calculate new local transform to preserve world position
    if (newParentIndex == -1) {
        // Reparenting to root: local = world
        child.localMatrix = currentWorldMatrix;
    } else {
        // Reparenting to parent: local = inverse(parent_world) * world
        glm::mat4 parentWorld = getWorldMatrix(newParentIndex);
        child.localMatrix = glm::inverse(parentWorld) * currentWorldMatrix;
    }
    
    // Add to new parent's children list
    if (newParentIndex != -1) {
        m_objects[newParentIndex].childIndices.push_back(childIndex);
    }
    
    // Update transforms
    updateWorldTransform(childIndex);
    
    return true;
}

bool SceneManager::reparentObject(const std::string& childName, const std::string& newParentName)
{
    int childIndex = findObjectIndex(childName);
    if (childIndex == -1) {
        std::cerr << "Child object '" << childName << "' not found" << std::endl;
        return false;
    }
    
    int parentIndex = -1;
    if (!newParentName.empty()) {
        parentIndex = findObjectIndex(newParentName);
        if (parentIndex == -1) {
            std::cerr << "Parent object '" << newParentName << "' not found" << std::endl;
            return false;
        }
    }
    
    return reparentObject(childIndex, parentIndex);
}

std::vector<int> SceneManager::getParentIndices() const
{
    std::vector<int> parentIndices;
    parentIndices.reserve(m_objects.size());
    
    for (const auto& obj : m_objects) {
        parentIndices.push_back(obj.parentIndex);
    }
    
    return parentIndices;
}

void SceneManager::updateWorldTransforms()
{
    // Update all objects, starting with roots
    for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
        if (m_objects[i].parentIndex == -1) {
            updateWorldTransform(i);
        }
    }
}

void SceneManager::updateWorldTransform(int objectIndex)
{
    if (objectIndex < 0 || objectIndex >= static_cast<int>(m_objects.size())) {
        return;
    }
    
    SceneObject& obj = m_objects[objectIndex];
    
    if (obj.parentIndex == -1) {
        // Root object: world = local
        obj.modelMatrix = obj.localMatrix;
    } else {
        // Child object: world = parent_world * local
        const glm::mat4& parentWorld = getWorldMatrix(obj.parentIndex);
        obj.modelMatrix = parentWorld * obj.localMatrix;
    }
    
    // Recursively update all children
    for (int childIndex : obj.childIndices) {
        updateWorldTransform(childIndex);
    }
}

glm::mat4 SceneManager::getWorldMatrix(int objectIndex) const
{
    if (objectIndex < 0 || objectIndex >= static_cast<int>(m_objects.size())) {
        return glm::mat4(1.0f);
    }
    return m_objects[objectIndex].modelMatrix;
}

void SceneManager::setLocalMatrix(int objectIndex, const glm::mat4& localMatrix)
{
    if (objectIndex < 0 || objectIndex >= static_cast<int>(m_objects.size())) {
        return;
    }
    
    m_objects[objectIndex].localMatrix = localMatrix;
    updateWorldTransform(objectIndex); // Update this object and all its children
}

void SceneManager::setLocalMatrix(const std::string& name, const glm::mat4& localMatrix)
{
    int index = findObjectIndex(name);
    if (index != -1) {
        setLocalMatrix(index, localMatrix);
    }
}

glm::mat4 SceneManager::getLocalMatrix(int objectIndex) const
{
    if (objectIndex < 0 || objectIndex >= static_cast<int>(m_objects.size())) {
        return glm::mat4(1.0f);
    }
    return m_objects[objectIndex].localMatrix;
}
