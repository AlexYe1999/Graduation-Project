#pragma once
#include <memory>
#include "Application.hpp"
#include "GraphicsManager.hpp" 
#include "Dx12Model.hpp"

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

    bool                               m_isLeftMouseDown;
    uint16_t                           m_lastMousePosX;
    uint16_t                           m_lastMousePosY;

    int                                m_cameraMode;
    Dx12Camera*                        m_camera;
    std::unique_ptr<Scene>             m_scene;

    uint32_t                           m_rtvDescriptorSize;
    uint32_t                           m_dsvDescriptorSize;
    uint32_t                           m_svvDescriptorSize;

    uint32_t                           m_texConstViewOffset;
    uint32_t                           m_gbufferViewOffset;

    GeoMath::Vector4f                  m_backgroundColor;

    Dx12GraphicsManager*               m_graphicsMgr;
    D3D12_VIEWPORT                     m_viewports[6];
    D3D12_RECT                         m_scissors[6];

    ComPtr<ID3D12RootSignature>        m_deferredRootSignature;

    ComPtr<ID3D12Resource>             m_depthStencil;
    std::unique_ptr<DefaultBuffer>     m_matConstBuffer;

    ComPtr<ID3D12DescriptorHeap>       m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_cbvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_srvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_guiSrvDescHeap;
};

