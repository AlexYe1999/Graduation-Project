set(BASIC_FILES
    Buffer.hpp
    CommandQueue.hpp
    CommandQueue.cpp
    d3dx12.h      
    DefaultBuffer.hpp
    Device.hpp
    Device.cpp
    DXSampleHelper.h
    DxUtility.hpp
    GBuffer.hpp
    GraphicsManager.hpp
    GraphicsManager.cpp
    ReadBackBuffer.hpp
    ReadBackBuffer.cpp
    Resource.hpp
    Texture2D.hpp
    UploadBuffer.hpp
)

add_library(Dx12 STATIC ${BASIC_FILES})

target_include_directories(Dx12
PRIVATE
    ${SOURCE_DIR}/asset
    ${SOURCE_DIR}/asset/dx12
    ${SOURCE_DIR}/core
    ${SOURCE_DIR}/utility
)

target_link_libraries(Dx12
PRIVATE
    d3d12.lib
    dxgi.lib
)