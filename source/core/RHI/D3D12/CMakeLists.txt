set(ALL_FILES
    CommandQueue.hpp
    CommandQueue.cpp
    d3dx12.h    
    DXSampleHelper.h
    Device.hpp
    Device.cpp
)

add_library(D3D12 STATIC ${ALL_FILES})
target_link_libraries(D3D12
    d3d12.lib
    dxgi.lib
)