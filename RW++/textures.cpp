#include "textures.h"

Texture::Texture(const TextureLease &lease, const uint32_t width, const uint32_t height, const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM) : owner(lease) {
    image = {};
    libgui::create_image(lease.VMA, lease.GPU, &image, width, height, 0, format);
}


Texture::~Texture() {
    image.dispose();
}

TextureLease::TextureLease(const vkb::Device &device, const VmaAllocator vma) {
    GPU = device;
    VMA = vma;
}

TexturePtr TextureLease::load_file(const libgui::VkImmediateCommandBuffer &cmd, const char *path, const char *name) {
    if (!std::filesystem::exists(path)) throw std::runtime_error("Given path doesn't exist: " + std::string(path));

    int w, h, c;
    const auto stbi_handle = stbi_load(path, &w, &h, &c, STBI_rgb_alpha);

    const auto return_created = std::make_shared<Texture>(*this, w, h, VK_FORMAT_R8G8B8A8_UNORM);

    // Create the VkImage
    const auto image_create = libgui::image_create_info(return_created->image.format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VkExtent3D(w, h, 1));
    constexpr VmaAllocationCreateInfo alloc_create {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VK_ASSERT(vmaCreateImage(VMA, &image_create, &alloc_create, &return_created->image.image, &return_created->image.allocation, nullptr));

    // Copy our stbi image to the VkImage
    libgui::VkSizedBuffer upload {};

    libgui::cmd_immediate(cmd, [&] {
        libgui::change_image_layout(cmd.cmd, return_created->image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        libgui::data_to_image(cmd.cmd, upload, VMA, stbi_handle, w * h * 1 * 4, 0, return_created->image.image, w, h);
        libgui::change_image_layout(cmd.cmd, return_created->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    stbi_image_free(stbi_handle);
    upload.dispose();

    // Create the view
    const VkImageViewCreateInfo view_create = libgui::imageview_create_info(return_created->image.format, return_created->image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_ASSERT(vkCreateImageView(GPU, &view_create, nullptr, &return_created->image.view));

    Textures.emplace(name, return_created);

    return return_created;
}

bool TextureLease::try_get(const char *name, TexturePtr *texture) const {
    if (!Textures.contains(name)) return false;

    *texture = Textures.at(name);

    return true;
}

void TextureLease::dispose_all() {
    // destructors be damned!
    for (const auto &texture: Textures | std::views::values) {
        texture->image.dispose();
    }

    Textures.clear();
}
