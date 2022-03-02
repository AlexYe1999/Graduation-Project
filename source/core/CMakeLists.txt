
set(ALL_FILES
    Timer.hpp
    Timer.cpp
)

add_library(Core STATIC ${ALL_FILES})

add_subdirectory(render)