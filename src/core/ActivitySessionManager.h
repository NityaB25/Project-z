#pragma once
#include <unordered_map>
#include <string>
#include <vector>

struct AppUsageStats {
    uint64_t foreground = 0;
    uint64_t background = 0;
};

class ActivitySessionManager {
public:
    // Core lifecycle
    void startSession();
    void stopSession();
    void update(const std::wstring& foregroundApp);

    // App management (ONLY source of truth)
    bool addApp(const std::wstring& app);
    void removeApp(const std::wstring& app);
    bool hasApp(const std::wstring& app) const;
    void clear();

    // Data access
    const std::unordered_map<std::wstring, AppUsageStats>& getStats() const;

    // Export / history
    std::wstring exportCSV() const;
    void saveSessionToDisk() const;
    std::wstring getSessionSummary() const;
    static std::vector<std::wstring> listSavedSessions();

private:
    std::unordered_map<std::wstring, AppUsageStats> stats;
};
