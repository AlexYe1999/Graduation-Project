#pragma once
#include "DxUtility.hpp"

class GpuResource{
public:
    GpuResource(D3D12_RESOURCE_STATES resourceUsage = D3D12_RESOURCE_STATE_COMMON) 
        : m_usageState(resourceUsage)
        , m_transitioningState(resourceUsage)
    {}
    ~GpuResource(){};

    ID3D12Resource* GetResource() { return m_resource.Get(); }
    const ID3D12Resource* GetResource() const { return m_resource.Get(); }
    ID3D12Resource* operator->() { return m_resource.Get(); } 
    const ID3D12Resource* operator->() const { return m_resource.Get(); }
    
protected:
    ComPtr<ID3D12Resource>    m_resource;
    D3D12_RESOURCE_STATES     m_usageState;
    D3D12_RESOURCE_STATES     m_transitioningState;
};