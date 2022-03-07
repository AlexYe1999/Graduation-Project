#pragma once
#include <memory>
#include "Application.hpp"
#include "CommandQueue.hpp"
#include "RenderResource.hpp" 
#include "Device.hpp"
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

    static uint16_t GetFrameCount() { return FrameCount; }

private:
    void InitD3D();
    void InitGUI();

    void LoadContent();

    void RenderScene();
    void RenderGUI();

protected:

    static const uint8_t               FrameCount = 3;

    bool                               m_isLeftMouseDown;
    uint16_t                           m_lastMousePosX;
    uint16_t                           m_lastMousePosY;

    Dx12Camera*                        m_camera;
    D3D12_VIEWPORT                     m_viewport;
    D3D12_RECT                         m_scissors;
    GeoMath::Vector4f                  m_backgroundColor;

    std::unique_ptr<Scene>             m_scene;
    std::unique_ptr<RenderResource>    m_renderResource;

    ComPtr<ID3D12DescriptorHeap>       m_guiSrvDescHeap;

};

