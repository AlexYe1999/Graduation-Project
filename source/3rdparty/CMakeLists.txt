
aux_source_directory(imgui IMGUI_HEADER)
set(IMGUI_BACKENDS
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx12.cpp
)

add_library(IMGUI ${IMGUI_HEADER} ${IMGUI_BACKENDS})
target_include_directories(IMGUI PRIVATE imgui)