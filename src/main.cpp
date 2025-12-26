#include <windows.h>

int RunMainWindow(HINSTANCE, int);

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow
)
{
    return RunMainWindow(hInstance, nCmdShow);
}
