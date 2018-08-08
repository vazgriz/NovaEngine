#pragma once
#include <unordered_set>
#include "NovaEngine/TransferNode.h"
#include "NovaEngine/Allocator.h"
#include "NovaEngine/ISystem.h"

namespace Nova {
    class Camera;

    class CameraManager : public ISystem {
    public:
        CameraManager(Engine& engine, TransferNode& transferNode);
        CameraManager(const CameraManager& other) = delete;
        CameraManager& operator = (const CameraManager& other) = delete;
        CameraManager(CameraManager&& other) = default;
        CameraManager& operator = (CameraManager&& other) = default;

        Engine& engine() const { return *m_engine; }
        BufferAllocator& allocator() const { return *m_allocator; }

        void addCamera(Camera& camera);
        void removeCamera(Camera& camera);
        void update(float delta) override;

    private:
        Engine* m_engine;
        TransferNode* m_transferNode;
        std::unordered_set<Camera*> m_cameras;
        std::unique_ptr<BufferAllocator> m_allocator;
    };
}