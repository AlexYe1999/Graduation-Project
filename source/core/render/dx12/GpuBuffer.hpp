#pragma once
#include "GpuResource.hpp"
#include <cstdint>

class GpuBuffer : public GpuResource{
public:
    GpuBuffer(uint32_t elementNum, uint32_t elementSize, D3D12_RESOURCE_STATES usageState)
        : GpuResource(usageState)
        , m_elementNum(elementNum)
        , m_elementSize(elementSize)
        , m_byteSize(elementNum*elementSize)
        , m_GpuVirtualAddress(0)
    {}

    ~GpuBuffer() = default;

    uint32_t GetByteSize() const { return m_byteSize; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t elementIndex = 0) const {
        return m_GpuVirtualAddress + elementIndex * m_elementSize; 
    }  

protected:
    uint32_t m_elementNum;
    uint32_t m_elementSize;
    uint32_t m_byteSize;
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
};

template<typename BufferType>
class VertexBuffer{

};

template<typename BufferType>
class IndexBuffer{

};

template<typename BufferType>
class ByteAddressedBuffer{
public:
    ByteAddressedBuffer();

};

template<typename BufferType>
class ConstantBuffer{
public:
    ConstantBuffer();

};

template<typename BufferType>
class StructuredBuffer{
public:
    StructuredBuffer();


};

