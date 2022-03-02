#pragma once
#include "Buffer.hpp"

class UploadBuffer : public Buffer{
public:
    UploadBuffer(const ComPtr<ID3D12Device8>&, size_t byteSize);
    UploadBuffer(UploadBuffer&&)      = default;
	UploadBuffer(const UploadBuffer&) = delete;

    void CopyData(const uint8_t* data, size_t byteSize, size_t offset = 0) const;
};