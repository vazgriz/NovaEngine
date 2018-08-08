#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "NovaEngine/Allocator.h"
#include "NovaEngine/TransferNode.h"
#include "NovaEngine/CameraManager.h"

namespace Nova {
    class Engine;

    class Camera {
        struct CameraInfo {
            glm::mat4 view;
            glm::mat4 rotationView;
            glm::mat4 projection;
        };

    public:
        Camera(CameraManager& cameraManager, glm::ivec2 size);
        Camera(const Camera& other) = delete;
        Camera& operator = (const Camera& other) = delete;
        Camera(Camera&& other) = default;
        Camera& operator = (Camera&& other) = default;
        virtual ~Camera();

        vk::DescriptorSetLayout& layout() const { return *m_layout; }
        vk::DescriptorSet& descriptor() const { return *m_descriptor; }
        Buffer& buffer() const { return *m_buffer; }

        glm::ivec2 size() const { return m_size; }
        glm::vec3 position() const { return m_pos; }
        glm::quat rotation() const { return m_rot; }
        void setSize(glm::ivec2 size);
        void setPosition(glm::vec3 pos);
        void setRotation(glm::quat rot);
        void update(TransferNode& transferNode);

    protected:
        virtual glm::mat4 getProjection() = 0;

    private:
        CameraManager* m_manager;
        std::unique_ptr<vk::DescriptorPool> m_descriptorPool;
        std::unique_ptr<vk::DescriptorSetLayout> m_layout;
        std::unique_ptr<vk::DescriptorSet> m_descriptor;
        std::unique_ptr<Buffer> m_buffer;
        glm::vec3 m_pos;
        glm::quat m_rot;
        glm::ivec2 m_size;

        void createPool();
        void createLayout();
        void createDescriptor();
        void createBuffer();
    };
}