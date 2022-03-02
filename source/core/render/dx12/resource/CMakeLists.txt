set(RESOURE_FILES
    Buffer.hpp
    Buffer.cpp
    DefaultBuffer.hpp
    DefaultBuffer.cpp
    RenderResource.hpp
    RenderResource.cpp
    UploadBuffer.hpp
    UploadBuffer.cpp
    Resource.hpp
)

add_library(Dx12Resource STATIC ${RESOURE_FILES})

target_include_directories(Dx12Resource
PRIVATE
    ..
    ${SOURCE_DIR}/core
    ${SOURCE_DIR}/utility
)

target_link_libraries(Dx12Resource
PRIVATE
    d3d12.lib
    dxgi.lib
)
