#include "SensitiveZoneManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

static const std::wstring CONFIG_PATH = L"config/sensitive_apps.txt";

SensitiveZoneManager::SensitiveZoneManager()
{
    loadFromFile();
}

std::wstring SensitiveZoneManager::normalize(const std::wstring& app) const
{
    std::wstring result = app;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

bool SensitiveZoneManager::addApp(const std::wstring& app)
{
    auto norm = normalize(app);
    if (sensitiveApps.insert(norm).second)
    {
        saveToFile();
        return true;
    }
    return false;
}

bool SensitiveZoneManager::removeApp(const std::wstring& app)
{
    auto norm = normalize(app);
    if (sensitiveApps.erase(norm))
    {
        saveToFile();
        return true;
    }
    return false;
}

bool SensitiveZoneManager::isSensitive(const std::wstring& app) const
{
    return sensitiveApps.count(normalize(app)) > 0;
}

void SensitiveZoneManager::listApps() const
{
    if (sensitiveApps.empty())
    {
        std::wcout << L"(Sensitive Zone is empty)\n";
        return;
    }

    for (const auto& app : sensitiveApps)
        std::wcout << L"- " << app << L"\n";
}

void SensitiveZoneManager::loadFromFile()
{
    sensitiveApps.clear();

    std::filesystem::create_directories(L"config");

    std::wifstream file(CONFIG_PATH);
    if (!file.is_open())
        return;

    std::wstring line;
    while (std::getline(file, line))
    {
        if (!line.empty())
            sensitiveApps.insert(normalize(line));
    }
}

void SensitiveZoneManager::saveToFile() const
{
    std::filesystem::create_directories(L"config");

    std::wofstream file(CONFIG_PATH, std::ios::trunc);
    for (const auto& app : sensitiveApps)
        file << app << L"\n";
}
const std::unordered_set<std::wstring>& SensitiveZoneManager::getAllApps() const
{
    return sensitiveApps;
}
