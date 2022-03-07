#pragma once
#include "Resource.hpp"
#include <cstdint>

class Buffer : public Resource{
public:
    Buffer(size_t byteSize, D3D12_RESOURCE_STATES resourceState)
        : Resource(resourceState)
        , m_byteSize(byteSize)
    {}
    Buffer(Buffer&&)      = default;
    Buffer(const Buffer&) = default;
    virtual ~Buffer()     = default;
    virtual D3D12_GPU_VIRTUAL_ADDRESS GetAddress() { return m_resource->GetGPUVirtualAddress(); }
    virtual size_t GetByteSize() const { return m_byteSize; }

protected:
    size_t m_byteSize;

};