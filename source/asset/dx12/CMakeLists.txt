
set(DX12_ASSET
    Dx12Mesh.hpp
    Dx12Mesh.cpp
    Dx12Model.hpp
    Dx12Model.cpp
    Dx12SceneNode.hpp
    Dx12SceneNode.cpp
    Dx12Struct.hpp
    Dx12Texture.hpp
)

add_library(Dx12Asset ${BASE_ASSET} ${DX12_ASSET})

target_include_directories(Dx12Asset
PRIVATE
    ..
    ${SOURCE_DIR}/utility
    ${SOURCE_DIR}/core/render/dx12
    ${SOURCE_DIR}/core/render/dx12/resource
    ${SOURCE_DIR}/3rdparty/tinygltf
)

target_link_libraries(Dx12Asset
    Utility
    Dx12Resource
    Dx12
    dxgi.lib
    d3d12.lib
    tinygltf
)