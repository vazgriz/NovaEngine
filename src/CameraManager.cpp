#include "NovaEngine/CameraManager.h"
#include "NovaEngine/Camera.h"

#define PAGE_SIZE (1024 * 1024)

using namespace Nova;

CameraManager::CameraManager(Engine& engine, TransferNode& transferNode) {
    m_engine = &engine;
    m_transferNode = &transferNode;

    m_allocator = std::make_unique<BufferAllocator>(*m_engine, PAGE_SIZE);
}

void CameraManager::addCamera(Camera& camera) {
    m_cameras.insert(&camera);
}

void CameraManager::removeCamera(Camera& camera) {
    m_cameras.erase(&camera);
}

void CameraManager::update(float delta) {
    for (auto camera : m_cameras) {
        camera->update(*m_transferNode);
    }
}