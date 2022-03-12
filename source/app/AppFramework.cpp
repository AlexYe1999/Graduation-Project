#include"AppFramework.hpp"

int AppFramework::Run(Application* pApp, HINSTANCE hInstance, int nCmdShow)
{
    app = pApp;

    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = "RenderPipeline";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(pApp->GetWidth()), static_cast<LONG>(pApp->GetHeight()) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    hWnd = CreateWindow(
        windowClass.lpszClassName,
        pApp->GetTitle().c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        pApp
    );

    pApp->OnInit();

    ShowWindow(hWnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        pApp->OnTick();
    }

    pApp->OnDestroy();

    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
    return static_cast<char>(msg.wParam);
}

LRESULT WINAPI AppFramework::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    return app->MsgProc(hWnd, msg, wParam, lParam);
}