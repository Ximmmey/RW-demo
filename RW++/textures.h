#pragma once

#define STB_IMAGE_IMPLEMENTATION

#include "libgui.h"
#include "libgui_vma.h"

#include <stb_image.h>

#include <map>
#include <memory>

struct Texture;

// Shared Ptr of Texture
typedef std::shared_ptr<Texture> TexturePtr;

// TODO: WEAK PTRS FOR THE LIST AS TEXTURES WON'T BE DESTROYED AUTOMATICALLY
// A leaser for automatically managing textures
class TextureLease {
public:
    vkb::Device GPU;
    VmaAllocator VMA;
    std::pmr::map<const char*, TexturePtr> Textures;

    TextureLease(const vkb::Device &device, VmaAllocator vma);

    TexturePtr load_file(const libgui::VkImmediateCommandBuffer &cmd, const char *path, const char *name);

    bool try_get(const char *name, TexturePtr *texture) const;

    void dispose_all();
};

// A VMA allocated image with a leaser
struct Texture {
    TextureLease owner;

    libgui::VkAllocatedImage image {};

    explicit Texture(const TextureLease &lease, uint32_t width, uint32_t height, VkFormat format);

    ~Texture();
};
