
set(BASE_ASSET
    ${CMAKE_CURRENT_SOURCE_DIR}/IComponent.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Material.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneNode.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture.hpp
)

add_subdirectory(dx12)