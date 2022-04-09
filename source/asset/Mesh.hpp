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
    void CreateNewMesh(uint32_t index, Args... args){
        m_meshIndices.emplace_back(index);
        m_meshes.emplace_back(args...);
    }

    virtual void Execute() override;
    virtual void OnUpdate();
    virtual void OnRender();

    const std::vector<uint32_t>& GetIndices() const{
        return m_meshIndices;
    }

protected:
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::vector<uint32_t> m_meshIndices;
};