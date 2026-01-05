#pragma once
#include <string>

std::wstring GetForegroundProcessName();
bool IsProcessRunning(const std::wstring& exeName);