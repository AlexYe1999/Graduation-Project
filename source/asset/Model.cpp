#include "Model.hpp"

Model::Model(const char* fileName){

    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&m_model, &err, &warn, fileName);

    if(!warn.empty()){
        OutputDebugString(("Warn: %s\n"+warn).c_str());
    }

    if(!err.empty()){
        OutputDebugString(("Err: %s\n"+err).c_str());
    }

    if(!ret){
        OutputDebugString("Failed to parse glTF\n");
    }
}

uint8_t* Model::GetBuffer(size_t bufferViewIndex){
    const auto& bufferView = m_model.bufferViews[bufferViewIndex];
    return m_model.buffers[bufferView.buffer].data.data() + bufferView.byteOffset;
}