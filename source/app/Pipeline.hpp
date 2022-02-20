#pragma once
#include <memory>
#include "Application.hpp"
#include "CommandQueue.hpp"
#include "Device.hpp"

class Pipeline : public Application{
public:
    Pipeline(std::string& name, uint16_t width, uint16_t height);
    virtual ~Pipeline();

    virtual void OnInit() override;
    virtual void OnTick() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;

    virtual void OnResize(const uint16_t, const uint16_t) override;

    virtual void OnKeyDown(const uint16_t key, const int16_t x = 0, const int16_t y = 0) override;
    virtual void OnKeyUp(const uint16_t key, const int16_t x = 0, const int16_t y = 0)   override;

    virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    float GetAspectRatio() const { return static_cast<float>(m_wndWidth) / static_cast<float>(m_wndHeight); }

private:
    void InitD3D();
    void InitGUI();
    void LoadContent();
    void RenderScene();
    void RenderGUI();

protected:

    static const uint16_t              m_frameCount = 3;
    uint32_t                           m_frameIndex;
    uint32_t                           m_rtvDescriptorSize;

    std::unique_ptr<Device>            m_device;
    std::unique_ptr<CommandQueue>      m_commandQueue;

    ComPtr<IDXGISwapChain3>            m_dxgiSwapChain;
    ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
    ComPtr<ID3D12Resource>             m_renderTargets[m_frameCount];

    ComPtr<ID3D12DescriptorHeap>       m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_guiSrvDescHeap;

};

