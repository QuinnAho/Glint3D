#include "application.h"
#include <vector>
#include <string>
#include <cstring>

#ifndef __EMSCRIPTEN__
#include "stb_image_write.h" // declarations only; implementation compiled elsewhere
#endif

// Headless: render current scene into an offscreen framebuffer and save PNG
bool Application::renderToPNG(const std::string& outPath, int W, int H)
{
#ifdef __EMSCRIPTEN__
    (void)outPath; (void)W; (void)H; return false;
#else
    if (!m_window) return false;

    GLuint fbo = 0, color = 0, depth = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &color);
        glDeleteRenderbuffers(1, &depth);
        return false;
    }

    // Shadow pass
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glm::vec3 lightDir = glm::normalize(glm::vec3(-6.0f, 7.0f, 8.0f));
    glm::mat4 lightView = glm::lookAt(lightDir * 20.0f, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
    m_lightSpaceMatrix = lightProjection * lightView;
    m_shadowShader->use();
    m_shadowShader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix);
    for (auto& obj : m_sceneObjects) {
        m_shadowShader->setMat4("model", obj.modelMatrix);
        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    }

    // Normal pass to offscreen FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, W, H);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = float(W) / float(H);
    m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspect, m_nearClip, m_farClip);
    m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    // draw grid
    m_grid.render(m_viewMatrix, m_projectionMatrix);
    // draw objects
    for (auto& obj : m_sceneObjects) {
        if (!obj.shader) obj.shader = m_standardShader;
        obj.shader->use();
        obj.shader->setMat4("model", obj.modelMatrix);
        obj.shader->setMat4("view", m_viewMatrix);
        obj.shader->setMat4("projection", m_projectionMatrix);
        obj.shader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
        obj.shader->setInt("shadowMap", 1);
        obj.shader->setVec3("viewPos", m_cameraPos);
        m_lights.applyLights(obj.shader->getID());
        obj.material.apply(obj.shader->getID(), "material");
        if (obj.shader == m_pbrShader) {
            obj.shader->setVec4("baseColorFactor", obj.baseColorFactor);
            obj.shader->setFloat("metallicFactor", obj.metallicFactor);
            obj.shader->setFloat("roughnessFactor", obj.roughnessFactor);
        } else {
            obj.shader->setInt("shadingMode", m_shadingMode);
            obj.shader->setVec3("objectColor", obj.color);
        }
        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Read back pixels (flip vertically)
    std::vector<unsigned char> pixels(size_t(W) * size_t(H) * 4);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<unsigned char> flipped(size_t(W) * size_t(H) * 4);
    for (int y = 0; y < H; ++y) {
        std::memcpy(&flipped[size_t(y) * size_t(W) * 4], &pixels[size_t(H - 1 - y) * size_t(W) * 4], size_t(W) * 4);
    }

    // Write PNG
    int ok = stbi_write_png(outPath.c_str(), W, H, 4, flipped.data(), W * 4);

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
    glDeleteRenderbuffers(1, &depth);

    return ok != 0;
#endif
}

