#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>

struct AppUsageStats {
    uint64_t foreground = 0;
    uint64_t background = 0;
};

class ActivitySessionManager
{
public:
    void update(const std::wstring& foregroundApp);
    void removeApp(const std::wstring& app);
    void clear();
void registerApp(const std::wstring& app);


    const std::unordered_map<std::wstring, AppUsageStats>& getStats() const;

private:
    std::unordered_map<std::wstring, AppUsageStats> stats;
};
