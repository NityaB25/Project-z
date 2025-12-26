#include "AppVisibilityMonitor.h"
#include <psapi.h>

void AppVisibilityMonitor::setProtectedApps(const std::vector<std::wstring>& apps)
{
    protectedApps = apps;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* apps = reinterpret_cast<std::vector<std::wstring>*>(lParam);

    if (!IsWindowVisible(hwnd))
        return TRUE;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc)
        return TRUE;

    wchar_t exeName[MAX_PATH];
    GetModuleBaseNameW(hProc, nullptr, exeName, MAX_PATH);
    CloseHandle(hProc);

    for (const auto& app : *apps)
    {
        if (_wcsicmp(exeName, app.c_str()) == 0)
        {
            // 🔑 Apply to ALL Chrome windows
            SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        }
    }

    return TRUE;
}

void AppVisibilityMonitor::applyCaptureExclusion()
{
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&protectedApps));
}
