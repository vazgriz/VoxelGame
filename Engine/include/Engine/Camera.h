#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace VoxelEngine {
    class Camera {
    public:
        Camera(uint32_t width, uint32_t height, float fov, float nearPlane, float farPlane);

        uint32_t width() const { return m_width; }
        uint32_t height() const { return m_height; }
        float fov() const { return m_fov; }
        float nearPlane() const { return m_nearPlane; }
        float farPlane() const { return m_farPlane; }
        glm::mat4 projectionMatrix() const { return m_projectionMatrix; }

        void setSize(uint32_t width, uint32_t height);
        void setFOV(float value);
        void setNearPlane(float value);
        void setFarPlane(float value);

        void setPosition(glm::vec3 position);
        void setRotation(glm::quat rotation);

    private:
        uint32_t m_width;
        uint32_t m_height;
        float m_fov;
        float m_nearPlane;
        float m_farPlane;
        glm::mat4 m_projectionMatrix;

        glm::vec3 m_position;
        glm::quat m_rotation;
        glm::mat4 m_viewMatrix;

        void createProjectionMatrix();
        void createViewMatrix();
    };
}