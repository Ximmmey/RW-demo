#define RWPP_VERSION 0.0.0.0
#define RWPP_VK_VERSION VK_MAKE_API_VERSION(0, 0, 0, 0)

#ifdef NDEBUG
#define WLOG_DISABLE_TRACE;
#endif
#define WLOG_MIRROR_CONSOLE

#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include <libgui.h>
#include <libgui_imgui.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <complex>
#include <filesystem>

// GLOB BREAKS so here's a bad fix
#include <draw_poller.cpp>
#include <textures.cpp>
#include <pipelines.cpp>
#include <scene.cpp>
#include <circle.cpp>
#include <level.cpp>
#include <simpleCollider.cpp>
#include <debuggeo.cpp>

static libgui::GUIManager GUI;

static std::shared_ptr<TextureLease> Textures;
static std::shared_ptr<Scene> MainScene;

#define FIXED_UPDATE_MS 25 // 40 FPS

static std::chrono::steady_clock::time_point LastDelta;
static std::chrono::steady_clock::time_point LastFixed;

static float FixedStacker;

static float DeltaTime;
static float FixedTime;

void dispose();

int main(int, char**) {
    wlog::redirect_printf();

    if (const auto init_result = volkInitialize(); init_result != VK_SUCCESS)
        throw std::runtime_error("Failed to initialise Volk for Vulkan: " + std::to_string(init_result));

    GUI = {};

    // Create the window
    if (const auto sdl_error = GUI.sdl_init("RW++"); sdl_error.has_value())
        throw std::runtime_error(sdl_error.value());

    // Set icon
    {
        int w, h, c;
        const auto stbi_handle = stbi_load("assets/icon.png", &w, &h, &c, STBI_rgb_alpha);
        SDL_Surface* icon = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, stbi_handle, w);
        SDL_SetWindowIcon(GUI.Window, icon);
        stbi_image_free(stbi_handle);
    }

    // Set up Vulkan
    {
        const auto vulkan_error = GUI.vulkan_init(
            "RW++",
            RWPP_VK_VERSION,
            "Blackgoo",
            RWPP_VK_VERSION,
#ifdef NDEBUG
            false,
#else
            true,
#endif
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        );

        if (vulkan_error.has_value())
            throw std::runtime_error(vulkan_error.value());
    }

    // Set up ImGui
    if (const auto imgui_error = libgui::imgui_init(GUI); imgui_error.has_value())
        throw std::runtime_error(imgui_error.value());

    Textures = std::make_shared<TextureLease>(GUI.GPU, GUI.VMA);
    MainScene = std::make_shared<Scene>(GUI.GPU, *Textures);

    glm::vec2 camOffset = custom::getCamOffsetFromFile("assets/levels/SU_A40.txt");
    RoomGeometry geo = RoomGeometry::fromFile("assets/levels/SU_A40.txt");

    MainScene->SceneObjects.push_back(std::make_unique<SceneLevel>(MainScene, "assets/levels/SU_A40.png"));
    // MainScene->SceneObjects.push_back(std::make_unique<SceneCircle>(MainScene, glm::vec2(500, 500)));
    // MainScene->SceneObjects.push_back(std::make_unique<SceneCircle>(MainScene, glm::vec2(400, 300)));
    MainScene->SceneObjects.push_back(std::make_unique<SimpleCollider>(MainScene, glm::vec2(200,700), glm::vec2(0,-100), camOffset, geo));

    SceneDebugGeo scene_debug_geo {geo, camOffset};

    LastDelta = std::chrono::steady_clock::now();
    LastFixed = std::chrono::steady_clock::now();

    // frame process
    while (GUI.WindowOpen) {
        GUI.event([] (const SDL_Event &event) {
            ImGui_ImplSDL3_ProcessEvent(&event);
        });

        libgui::imgui_frame_begin();

        // Fixed update
        FixedStacker += DeltaTime;
        while (FixedStacker >= FIXED_UPDATE_MS) {
            MainScene->physics_tick();

            auto now = std::chrono::steady_clock::now();
            auto fms = now - LastFixed;
            FixedTime = std::chrono::duration_cast<std::chrono::milliseconds>(fms).count();
            LastFixed = now;

            FixedStacker -= FIXED_UPDATE_MS;
        }

        // Frame update
        MainScene->frame_update();
        scene_debug_geo.frame_update();

        MainScene->poll_and_draw();

        libgui::imgui_frame_end();
        GUI.sync_draw_frame([&](const VkCommandBuffer cmd, const libgui::VkAllocatedImage &draw_img) {
            libgui::change_image_layout(cmd, MainScene->DrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            libgui::change_image_layout(cmd, draw_img.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            libgui::blit_image(cmd, MainScene->DrawImage.image, VkExtent2D(MainScene->DrawImage.width, MainScene->DrawImage.height), draw_img.image, VkExtent2D(draw_img.width, draw_img.height));
            libgui::change_image_layout(cmd, draw_img.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
            libgui::change_image_layout(cmd, MainScene->DrawImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            libgui::imgui_draw(cmd, draw_img);
        });

        auto now = std::chrono::steady_clock::now();
        auto dms = now - LastDelta;
        DeltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(dms).count();
        LastDelta = now;
    }

    MainScene->dispose();
    Textures->dispose_all();
    dispose();

    return 0;
}

void dispose()
{
    wlog::log(wlog::WLOG_INFO, "Proper exit!");

    GUI.dispose();
    wlog::restore_printf();
}
