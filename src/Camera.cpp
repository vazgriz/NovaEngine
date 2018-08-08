#include "NovaEngine/Camera.h"
#include <glm/gtc/matrix_transform.hpp> 
#include "NovaEngine/Engine.h"

using namespace Nova;

Camera::Camera(CameraManager& cameraManager, glm::ivec2 size) {
    m_manager = &cameraManager;
    m_size = size;

    createPool();
    createLayout();
    createBuffer();
    createDescriptor();

    m_manager->addCamera(*this);
}

Camera::~Camera() {
    m_manager->removeCamera(*this);
}

void Camera::setSize(glm::ivec2 size) {
    m_size = size;
}

void Camera::setPosition(glm::vec3 pos) {
    m_pos = pos;
}

void Camera::setRotation(glm::quat rot) {
    m_rot = rot;
}

void Camera::update(TransferNode& transferNode) {
    glm::vec3 forward = m_rot * glm::vec3(0, 0, -1);
    glm::vec3 up = m_rot * glm::vec3(0, 1, 0);

    CameraInfo info = {};
    info.view = glm::lookAtRH(m_pos, m_pos + forward, up);
    info.rotationView = glm::lookAtRH({}, forward, up);
    info.projection = getProjection();

    vk::BufferCopy copy = {};
    copy.size = sizeof(info);

    transferNode.transfer(&info, *m_buffer, copy);
}

void Camera::createPool() {
    vk::DescriptorPoolSize size = {};
    size.type = vk::DescriptorType::UniformBuffer;
    size.descriptorCount = 1;

    vk::DescriptorPoolCreateInfo info = {};
    info.maxSets = 1;
    info.poolSizes = { size };

    m_descriptorPool = std::make_unique<vk::DescriptorPool>(m_manager->engine().renderer().device(), info);
}

void Camera::createLayout() {
    vk::DescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = vk::DescriptorType::UniformBuffer;
    binding.descriptorCount = 1;
    binding.stageFlags = vk::ShaderStageFlags::Vertex;

    vk::DescriptorSetLayoutCreateInfo info = {};
    info.bindings = { binding };

    m_layout = std::make_unique<vk::DescriptorSetLayout>(m_manager->engine().renderer().device(), info);
}

void Camera::createBuffer() {
    vk::BufferCreateInfo info = {};
    info.size = sizeof(CameraInfo);
    info.usage = vk::BufferUsageFlags::UniformBuffer | vk::BufferUsageFlags::TransferDst;

    m_buffer = std::make_unique<Buffer>(m_manager->allocator().allocate(info, vk::MemoryPropertyFlags::DeviceLocal, {}));
}

void Camera::createDescriptor() {
    vk::DescriptorSetAllocateInfo info = {};
    info.descriptorPool = m_descriptorPool.get();
    info.setLayouts = { *m_layout };

    m_descriptor = std::make_unique<vk::DescriptorSet>(std::move(m_descriptorPool->allocate(info)[0]));

    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = &m_buffer->resource();
    bufferInfo.range = VK_WHOLE_SIZE;

    vk::WriteDescriptorSet write = {};
    write.dstSet = m_descriptor.get();
    write.descriptorType = vk::DescriptorType::UniformBuffer;
    write.bufferInfo = { bufferInfo };

    vk::DescriptorSet::update(m_manager->engine().renderer().device(), { write }, {});
}