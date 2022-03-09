#pragma once
#include "DxUtility.hpp"

class Shader{
public:
    Shader(const char* fileName)
        : shaderName(fileName)
    {}

protected:
    const char* shaderName;
};
