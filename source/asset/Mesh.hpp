#pragma once
#include "IComponent.hpp"
#include "Material.hpp"
#include "Utility.hpp"

class Mesh{
public:
    Mesh(const std::shared_ptr<Material>& material)
        : m_material(material)
    {}
    virtual void OnRender() = 0;

public:
    std::shared_ptr<Material> m_material;
};

class StaticMesh : public IComponent{
public:
    StaticMesh() : m_meshes(){}

    template<typename... Args>
    void CreateNewMesh(Args... args){
        m_meshes.emplace_back(args...);
    }

    virtual void Execute() override;
    virtual void OnUpdate();
    virtual void OnRender();

protected:
    std::vector<std::unique_ptr<Mesh>> m_meshes;
};