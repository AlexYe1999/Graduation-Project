set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "source code dir")
set(3RDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty CACHE PATH "3rdparty dir")

add_subdirectory(3rdparty)
add_subdirectory(app)
add_subdirectory(asset)
add_subdirectory(core)
add_subdirectory(utility)