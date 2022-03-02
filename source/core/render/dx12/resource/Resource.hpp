#pragma once
#include "DxUtils.hpp"

class Resource{
public:
    Resource(D3D12_RESOURCE_STATES resourceState) 
        : m_resourceState(resourceState)
    {}
    Resource(Resource&&)      = default;
    Resource(const Resource&) = default;
    virtual ~Resource()       = default;
    virtual ID3D12Resource* GetResource() const { return m_resource.Get(); }
    virtual D3D12_RESOURCE_STATES GetState() const { return m_resourceState; }

protected:
    ComPtr<ID3D12Resource> m_resource;
    D3D12_RESOURCE_STATES  m_resourceState;
};