#pragma once
#include <windows.h>
#include <vector>
#include <string>

class AppVisibilityMonitor
{
public:
    void setProtectedApps(const std::vector<std::wstring>& apps);
    void applyCaptureExclusion();

private:
    std::vector<std::wstring> protectedApps;
};
