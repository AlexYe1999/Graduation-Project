#pragma once
#include "tiny_gltf.h"
#include "SceneNode.hpp"
#include <vector>

class Model{
public:
    Model(const char* fileName);

protected:
    tinygltf::Model m_model;
    uint8_t* GetBuffer(size_t bufferViewIndex);
};

