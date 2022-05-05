#pragma once
#include <memory>
#include "Application.hpp"
#include "Dx12SceneNode.hpp"
#include "GraphicsManager.hpp" 

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

    void RenderScene();
    void RenderGUI();

protected:

    bool                               m_openDenoising;
    bool                               m_openFrameBlend;
    bool                               m_openReprojection;
    float                              m_alpha;
    float                              m_beta;
    float                              m_gamma;
    float                              m_sigma;

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
    D3D12_VIEWPORT                     m_viewport;
    D3D12_RECT                         m_scissors;

    ComPtr<ID3D12RootSignature>        m_deferredRootSignature;
    ComPtr<ID3D12Resource>             m_depthStencil;
    std::unique_ptr<DefaultBuffer>     m_matConstBuffer;
    std::vector<Texture2D>             m_textures;

    D3D12_DISPATCH_RAYS_DESC           m_dispatchRayDesc;
    ComPtr<ID3D12StateObject>          m_rayTracingStateObject;
    ComPtr<ID3D12RootSignature>        m_rayTracingGlobalRootSignature;
    ComPtr<ID3D12RootSignature>        m_hitLocalRootSignature;
    std::unique_ptr<UploadBuffer>      m_shaderTable;

    std::unique_ptr<DefaultBuffer>      m_rayTraceMeshInfoGpu;
    std::unique_ptr<DefaultBuffer>      m_rayTraceIndexBuffer;
    std::unique_ptr<DefaultBuffer>      m_rayTraceVertexBuffer;
    std::vector<ComPtr<ID3D12Resource>> m_bottomLevelAccelerationStructures;

    ComPtr<ID3D12RootSignature>         m_denoisingRootSignature;

    ComPtr<ID3D12DescriptorHeap>       m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_cbvHeap;
    ComPtr<ID3D12DescriptorHeap>       m_guiSrvDescHeap;

};

