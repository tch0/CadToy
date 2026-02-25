# An OpenGL Project Example

3rd-party libraries:
- glfw
- glad
- glm
- SOIL2
- magic_enum
- rapidjson
- sqlite
- fmt
- imgui

Cross Platform:
- UNIX-like/Linux
- Apple
- Windows

Cross Compiler: C++20 required currently, downgrade as you like by modifying or removing codes that require C++ 20.
- GCC
- Clang
- MSVC (2022 recommended)
- Clang-cl

Usage:
- Search for all "OpenGLExample", replace to your project name.
- Build 3rd-party libraries and install them (sources/headers/archives):
```sh
cd 3rdparty
mkdir build
cd build
cmake .. [-G "Some Generator"] [-DCMAKE_BUILD_TYPE=Release]
cmake --build . [--config=Release]
cmake --install .
```
- Build main project:
```sh
cd project-root
mkdir build
cd build
cmake .. [-G "Same Generator as above"] [-DCMAKE_BUILD_TYPE=Release]
cmake --build . [--config=Release]
```