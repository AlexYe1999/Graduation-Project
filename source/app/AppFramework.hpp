#pragma once
#include"Application.hpp"
#include<windows.h>
#include<windowsx.h>

class AppFramework{
public:
    static int Run(Application* pApp, HINSTANCE hInstance, int nCmdShow);
    static HWND GetWnd() { return hWnd; }

protected:
    static LRESULT WINAPI WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    inline static HWND hWnd = nullptr;
    inline static Application* app = nullptr;
};