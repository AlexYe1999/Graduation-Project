set(ALL_FILES
    Application.hpp
    AppFramework.hpp
    AppFramework.cpp
    Pipeline.hpp
    Pipeline.cpp
    main.cpp
)

include_directories(
    ${3RDPARTY_DIR}/imgui
    ${3RDPARTY_DIR}/imgui/backends
)

include_directories(
    ${SOURCE_DIR}/core/RHI/D3D12
)

add_executable(Graduation-Project WIN32 ${ALL_FILES})
target_link_libraries(Graduation-Project
    D3D12 
    IMGUI
)

set_property(TARGET Graduation-Project PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")