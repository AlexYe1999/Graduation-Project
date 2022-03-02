#pragma once
#include "IComponent.hpp"
#include <vector>
#include <memory>

class Mesh{
public:
    virtual void OnRender() = 0;
};

class StaticMesh : public IComponent{
public:
    StaticMesh() : m_meshes(){}

    template<typename... Args>
    void CreateNewMesh(Args... args){
        m_meshes.emplace_back(args...);
    }

    virtual void Execute() override;
    virtual void OnRender();

protected:
    std::vector<std::unique_ptr<Mesh>> m_meshes;
};