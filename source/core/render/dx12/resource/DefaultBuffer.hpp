#pragma once
#include "Buffer.hpp"
#include "UploadBuffer.hpp"

class DefaultBuffer : public Buffer{
public:
    DefaultBuffer(
        const ComPtr<ID3D12Device8>&,
        const ComPtr<ID3D12GraphicsCommandList2>&,
        const UploadBuffer& uploadBuffer
    );
    DefaultBuffer(DefaultBuffer&&)      = delete;
    DefaultBuffer(const DefaultBuffer&) = delete;

};
