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
    ${SOURCE_DIR}/asset
    ${SOURCE_DIR}/asset/dx12
    ${SOURCE_DIR}/core
    ${SOURCE_DIR}/core/render/dx12
    ${SOURCE_DIR}/core/render/dx12/resource
    ${SOURCE_DIR}/utility
)

add_executable(Graduation-Project WIN32 ${ALL_FILES})
target_link_libraries(Graduation-Project
    Dx12
    IMGUI
    Dx12Asset
)

set_property(TARGET Graduation-Project PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")