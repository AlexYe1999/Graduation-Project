#pragma once
#include "DxUtils.hpp"

class Shader{
public:
    Shader(const char* fileName)
        : shaderName(fileName)
    {}

protected:
    const char* shaderName;
};
