#include "NovaEngine/CameraManager.h"
#include "NovaEngine/Camera.h"

using namespace Nova;

CameraManager::CameraManager(TransferNode& transferNode) {
    m_transferNode = &transferNode;
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