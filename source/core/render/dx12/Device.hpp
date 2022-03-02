#pragma once
#include "DXSampleHelper.h"

class Device{
public:
    Device();
    ~Device();

    Device(const Device&) = delete;
    Device(Device&&) = delete;

    const ComPtr<IDXGIFactory5>& DxgiFactory() const { return m_dxgiFactory; }
    const ComPtr<IDXGIAdapter1>& DxAdapter() const { return m_dxgiAdapter; }
    const ComPtr<ID3D12Device8>& DxDevice() const { return m_d3d12Device; }

private:
    ComPtr<IDXGIFactory5> m_dxgiFactory;
    ComPtr<IDXGIAdapter1> m_dxgiAdapter;
    ComPtr<ID3D12Device8> m_d3d12Device;

};