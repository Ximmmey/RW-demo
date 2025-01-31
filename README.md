RWPP has been built and tested under 257.2-2-arch and xorg display server, it should build and work under windows as well (but steps to building the project may be different). Wayland should also be just fine as the project does not use any xorg specific elements. 

Tested on device with Ryzen 3 5300U with Radeon Graphics 384SP integrated graphics, it is at this point quite outdated (technically only supporting vulkan 1.3) so you should be fine.

To compile project, you will have to initiate git submodules and install vcpkg packages. If git submodules fail to clone, you add them manually using:

git submodule add --force https://github.com/libsdl-org/SDL SDL
git submodule add --force --branch docking https://github.com/ocornut/imgui.git imgui
git submodule add --force https://github.com/charles-lunarg/vk-bootstrap vk-bootstrap

Make sure you are pulling DOCKING branch of imgui. You are also free to just clone git repos instead of using submodules.

For vcpkg packages, run:

./vcpkg install volk
./vcpkg install vulkan-memory-allocator
./vcpkg install iniparser
./vcpkg install glm

Now you should be able to run:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

If that fails, most likely your vcpkg root is not configured, you may point cmake at your vcpkg directory using:

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/YOUR_PATH_HERE/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug

Once cmake finishes, you will have to navigate to /build directory and simple run make

Once executable builds (it may take a little as cmake and make cache a lot of the results for future compilations), please note that:

You can use right click to teleport the simple collider to position of the cursor
You may use AWSD for moving the collider, however S does nothing and jumping isn't really supported.
You are free to modify gravity, friction, bouncy (although friction abd bouncy must be between 0 and 1 else things get weird)
Mass currently does nothing (as it is leftover from interactions with other colliders that I didn't have time to finish)
Radius does technically work, but sprite of the collider is unaffected (and collision checks with geometry fail for smaller colliders)
You may also enable the geo debug, which will show you what is considered "solid" geometry and modify collider's gravity.

You may run example GTest with:

./test_room_geometry

![alt text](https://raw.githubusercontent.com/Ximmmey/RW-demo/main/images/demo.png "C++ demo")

There is still a long way to go

![alt text](https://raw.githubusercontent.com/Ximmmey/RW-demo/main/images/rw.png "Rain World")
