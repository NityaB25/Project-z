
#include "ForegroundTracker.h"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>


#pragma comment(lib, "psapi.lib")

std::wstring GetForegroundProcessName()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return L"";

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        pid
    );

    if (!hProcess)
        return L"";

    wchar_t name[MAX_PATH]{};
    GetModuleBaseNameW(hProcess, nullptr, name, MAX_PATH);

    CloseHandle(hProcess);
    return name;
}

bool IsProcessRunning(const std::wstring& exeName)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0)
            {
                CloseHandle(snap);
                return true;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return false;
}


