cmake_minimum_required(VERSION 3.30)
project(rwpp)

set(CMAKE_CXX_STANDARD 26)

file(GLOB RWPP_SRC RW++/*.cpp)
file(GLOB IMGUI_SRC imgui/*.cpp)

add_compile_definitions(IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
add_compile_definitions(IMGUI_IMPL_VULKAN_USE_VOLK)

add_executable(rwpp RW++/main.cpp RW++/icon.rc ${IMGUI_SRC} imgui/backends/imgui_impl_sdl3.cpp imgui/backends/imgui_impl_vulkan.cpp)

target_include_directories(rwpp PRIVATE RW++)
target_include_directories(rwpp PRIVATE libgui)

target_include_directories(rwpp PRIVATE stb)
target_include_directories(rwpp PRIVATE imgui)
target_include_directories(rwpp PRIVATE imgui/backends)

# Copy assets to output
file(COPY "RW++/assets" DESTINATION "${CMAKE_BINARY_DIR}")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/shaders/")

# Copy pre-compiled SPV to output
file(GLOB SPV_FILES "RW++/shaders/spv/*.*")
file(COPY ${SPV_FILES} DESTINATION "${CMAKE_BINARY_DIR}/shaders")

# Compile /shaders to output
# You need glslang for this
file(GLOB SHADER_FILES "RW++/shaders/*.*")
set(COMMANDS)
foreach(SHADER_SOURCE IN LISTS SHADER_FILES)
    cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
    cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)

    set(INCLUDE_DIR "RW++/shaders/include")
    cmake_path(ABSOLUTE_PATH INCLUDE_DIR NORMALIZE)

    file(TOUCH "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    LIST(APPEND COMMANDS COMMAND glslang -V100 -I${INCLUDE_DIR} -o ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv -e main ${SHADER_SOURCE})
endforeach()

add_custom_target(
        shader_compile
        SOURCES ${SHADER_FILES}
        COMMENT "COMPILING SHADERS"
        ${COMMANDS}
)
add_dependencies(rwpp shader_compile)

add_subdirectory(SDL EXCLUDE_FROM_ALL)
target_link_libraries(rwpp PRIVATE SDL3::SDL3)

if(WIN32)
    add_custom_command(
            TARGET rwpp POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:rwpp>
            VERBATIM
    )
endif()

add_subdirectory(vk-bootstrap EXCLUDE_FROM_ALL)
target_link_libraries(rwpp PRIVATE vk-bootstrap::vk-bootstrap)

# VCPKG INSTALLED LIBRARIES
find_package(glm CONFIG REQUIRED)
target_link_libraries(rwpp PRIVATE glm::glm)

find_package(volk CONFIG REQUIRED) # Vulkan loader / utils
target_link_libraries(rwpp PRIVATE volk::volk_headers)

find_package(VulkanMemoryAllocator CONFIG REQUIRED) # Vulkan easier allocation of buffers
target_link_libraries(rwpp PRIVATE GPUOpen::VulkanMemoryAllocator)

find_package(unofficial-iniparser CONFIG REQUIRED) # .INI file support
target_link_libraries(rwpp PRIVATE unofficial::iniparser::iniparser)
