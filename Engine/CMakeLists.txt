cmake_minimum_required (VERSION 3.8)

project ("Engine")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_ROOT} ${GLFW_BUILD} EXCLUDE_FROM_ALL)

add_library("Engine"
    include/Engine/math.h
    math.cpp
    vma.cpp
    include/Engine/Engine.h
    Engine.cpp
    include/Engine/Window.h
    Window.cpp
    include/Engine/System.h
    System.cpp
    include/Engine/Clock.h
    Clock.cpp
    include/Engine/Graphics.h
    Graphics.cpp
    include/Engine/MemoryManager.h
    MemoryManager.cpp
    include/Engine/Buffer.h
    Buffer.cpp
    include/Engine/Mesh.h
    Mesh.cpp
    include/Engine/RenderGraph/RenderGraph.h
    RenderGraph.cpp
    include/Engine/RenderGraph/AcquireNode.h
    AcquireNode.cpp
    include/Engine/RenderGraph/PresentNode.h
    PresentNode.cpp
    include/Engine/RenderGraph/TransferNode.h
    TransferNode.cpp
    include/Engine/Utilities.h
    Utilities.cpp
    include/Engine/UniformWriter.h
    UniformWriter.cpp
    include/Engine/BufferWriter.h
    BufferWriter.cpp
    include/Engine/Camera.h
    Camera.cpp
    include/Engine/CameraSystem.h
    CameraSystem.cpp
    include/Engine/Input.h
    Input.cpp
    include/Engine/Image.h
    Image.cpp
    include/Engine/BlockingQueue.h
    include/Engine/BufferedQueue.h
    include/Engine/FreeListAllocator.h
    FreeListAllocator.cpp
)

target_compile_definitions("Engine" PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

target_include_directories("Engine"
    PUBLIC "${GLFW_ROOT}/include"
    PUBLIC "./include"
    PUBLIC "${VULKAN_ROOT}/Include"
    PUBLIC "${VKW_ROOT}/include"
    PUBLIC "${VMA_INCLUDE}"
    PUBLIC "${GLM_INCLUDE}"
    PUBLIC "${ENTT_INCLUDE}"
)

target_link_libraries("Engine"
    "${GLFW_BUILD}/src/glfw3.lib"
    "${VKW_BUILD}/src/VulkanWrapper.lib"
    "${VULKAN_ROOT}/Lib/vulkan-1.lib"
)

set_property(TARGET "Engine" PROPERTY CXX_STANDARD 17)