#pragma once
#include <string>
#include <unordered_set>

class SensitiveZoneManager
{
public:
    SensitiveZoneManager();

    bool addApp(const std::wstring& app);
    bool removeApp(const std::wstring& app);
    bool isSensitive(const std::wstring& app) const;
    void listApps() const;
    const std::unordered_set<std::wstring>& getAllApps() const;
    void loadFromDisk();
void saveToDisk() const;



    void loadFromFile();
    void saveToFile() const;

private:
    std::unordered_set<std::wstring> sensitiveApps;
    std::wstring normalize(const std::wstring& app) const;
};
