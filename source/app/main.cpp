#include "AppFramework.hpp"
#include "Pipeline.hpp"

int WINAPI WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR pCmdLine, int nCmdShow
){

    Pipeline app(std::string("Simple DXR Hybrid Rendering Pipeline"), 1280, 720);
    AppFramework::Run(&app, hInstance, nCmdShow);

    return 0;
}
