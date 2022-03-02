set(BASIC_FILES
    CommandQueue.hpp
    CommandQueue.cpp
    d3dx12.h    
    Device.hpp
    Device.cpp
    DXSampleHelper.h
    DxUtils.hpp
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

add_subdirectory(resource)
