#pragma once
#include <unordered_set>
#include "NovaEngine/TransferNode.h"

namespace Nova {
    class Camera;

    class CameraManager : public ISystem {
    public:
        CameraManager(TransferNode& engine);
        CameraManager(const CameraManager& other) = delete;
        CameraManager& operator = (const CameraManager& other) = delete;
        CameraManager(CameraManager&& other) = default;
        CameraManager& operator = (CameraManager&& other) = default;

        void addCamera(Camera& camera);
        void removeCamera(Camera& camera);
        void update(float delta) override;

    private:
        TransferNode* m_transferNode;
        std::unordered_set<Camera*> m_cameras;
    };
}