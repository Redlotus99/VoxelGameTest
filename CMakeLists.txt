cmake_minimum_required(VERSION 3.16)
project(VoxelGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
include(FetchContent)
FetchContent_Declare(
  sdl2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-2.30.0
)
FetchContent_MakeAvailable(sdl2)

# Fetch GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# Fetch GLAD
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v0.1.34
)
FetchContent_MakeAvailable(glad)

# Create executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/core/Window.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
    ${sdl2_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} 
    SDL2::SDL2
    glad
)
