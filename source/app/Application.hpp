#pragma once
#include <cstdint>
#include <string>
#include <cstdlib>
#include <windows.h>

class Application{
public:
    Application(const std::string& name, uint16_t width, uint16_t height) 
        : m_title(name)
        , m_wndWidth(width)
        , m_wndHeight(height)
    {};

    virtual ~Application() {};

    virtual void OnInit()    = 0;
    virtual void OnTick()    = 0;
    virtual void OnUpdate()  = 0;
    virtual void OnRender()  = 0;
    virtual void OnDestroy() = 0;

    virtual void OnResize(const uint16_t, const uint16_t) = 0;

    virtual void OnKeyDown(const uint16_t key, const int16_t x = 0, const int16_t y = 0) = 0;
    virtual void OnKeyUp(const uint16_t key, const int16_t x = 0, const int16_t y = 0)   = 0;

    virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

    const std::string& GetTitle() const { return m_title; }
    uint16_t GetWidth()  const { return m_wndWidth; }
    uint16_t GetHeight() const { return m_wndHeight; }

protected:
    std::string m_title;
    uint16_t m_wndWidth;
    uint16_t m_wndHeight;
};