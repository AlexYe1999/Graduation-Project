
aux_source_directory(imgui IMGUI_MAIN)
set(IMGUI_BACKENDS
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx12.cpp
)

add_library(IMGUI ${IMGUI_MAIN} ${IMGUI_BACKENDS})
target_include_directories(IMGUI PRIVATE imgui)

set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(tinygltf)