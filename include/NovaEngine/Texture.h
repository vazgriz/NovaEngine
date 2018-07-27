#pragma once
#include <VulkanWrapper/VulkanWrapper.h>
#include "NovaEngine/Allocator.h"

namespace Nova {
    class Texture {
    public:
        Texture(Engine& engine, Image&& image);
        Texture(const Texture& other) = delete;
        Texture& operator = (const Texture& other) = delete;
        Texture(Texture&& other) = default;
        Texture& operator = (Texture&& other) = default;

        Nova::Image& image() const { return *m_image; }
        vk::ImageView& imageView() const { return *m_imageView; }

    private:
        Engine* m_engine;
        std::unique_ptr<Image> m_image;
        std::unique_ptr<vk::ImageView> m_imageView;

        void createImageView();
    };
}