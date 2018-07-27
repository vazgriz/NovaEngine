#include "NovaEngine/Texture.h"

using namespace Nova;

Texture::Texture(Engine& engine, Image&& image) {
    m_engine = &engine;

    m_image = std::make_unique<Image>(std::move(image));
    createImageView();
}

void Texture::createImageView() {
    vk::ImageViewCreateInfo info = {};
    auto& image = m_image->resource();
    info.image = &image;
    auto format = image.format();
    info.format = format;

    if (vk::isColorFormat(format)) {
        info.subresourceRange.aspectMask |= vk::ImageAspectFlags::Color;
    }
    if (vk::isDepthFormat(format)) {
        info.subresourceRange.aspectMask |= vk::ImageAspectFlags::Depth;
    }
    if (vk::isStencilFormat(format)) {
        info.subresourceRange.aspectMask |= vk::ImageAspectFlags::Stencil;
    }

    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = image.arrayLayers();
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = image.mipLevels();

    auto type = image.imageType();
    if (type == vk::ImageType::_1D) {
        if (image.arrayLayers() == 1) {
            info.viewType = vk::ImageViewType::_1D;
        } else {
            info.viewType = vk::ImageViewType::_1D_Array;
        }
    } else if (type == vk::ImageType::_2D) {
        if (image.arrayLayers() == 1) {
            info.viewType = vk::ImageViewType::_2D;
        } else {
            if ((image.flags() & vk::ImageCreateFlags::CubeCompatible) != vk::ImageCreateFlags::None
                && image.arrayLayers() >= 6) {
                info.subresourceRange.layerCount -= image.arrayLayers() % 6;
                if (image.arrayLayers() < 12) {
                    info.viewType = vk::ImageViewType::Cube;
                } else {
                    info.viewType = vk::ImageViewType::CubeArray;
                }
            } else {
                info.viewType = vk::ImageViewType::_2D_Array;
            }
        }
    } else if (type == vk::ImageType::_3D) {
        info.viewType = vk::ImageViewType::_3D;
    }

    m_imageView = std::make_unique<vk::ImageView>(m_engine->renderer().device(), info);
}