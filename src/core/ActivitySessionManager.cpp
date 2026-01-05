#include "ActivitySessionManager.h"
#include "ForegroundTracker.h"
#include<chrono>
static std::chrono::system_clock::time_point sessionStart;
static std::chrono::system_clock::time_point sessionEnd;


void ActivitySessionManager::update(const std::wstring& foregroundApp)
{
    for (auto& [app, data] : stats)
    {
        if (!IsProcessRunning(app))
            continue; // not running → no time counted

        if (!foregroundApp.empty() && app == foregroundApp)
            data.foreground++;
        else
            data.background++;
    }
}


void ActivitySessionManager::removeApp(const std::wstring& app)
{
    stats.erase(app);
}

void ActivitySessionManager::clear()
{
    stats.clear();
}

void ActivitySessionManager::registerApp(const std::wstring& app)
{
    stats[app]; // creates entry if missing
}


std::wstring ActivitySessionManager::exportCSV() const
{
    std::wstring csv = L"App,Foreground(sec),Background(sec),Total(sec)\n";

    for (const auto& [app, data] : stats)
    {
        uint64_t total = data.foreground + data.background;

        csv += app + L"," +
               std::to_wstring(data.foreground) + L"," +
               std::to_wstring(data.background) + L"," +
               std::to_wstring(total) + L"\n";
    }

    return csv;
}


void ActivitySessionManager::startSession()
{
    sessionStart = std::chrono::system_clock::now();
}

void ActivitySessionManager::stopSession()
{
    sessionEnd = std::chrono::system_clock::now();
}

std::wstring ActivitySessionManager::getSessionSummary() const
{
    using namespace std::chrono;

    auto duration = duration_cast<seconds>(sessionEnd - sessionStart).count();

    std::wstring topApp = L"N/A";
    uint64_t maxFg = 0;

    for (const auto& [app, data] : stats)
    {
        if (data.foreground > maxFg)
        {
            maxFg = data.foreground;
            topApp = app;
        }
    }

    std::wstring summary;
    summary += L"Session Duration: " + std::to_wstring(duration) + L" seconds\n";
    summary += L"Apps Tracked: " + std::to_wstring(stats.size()) + L"\n";
    summary += L"Top App (Foreground): " + topApp + L"\n";

    return summary;
}




const std::unordered_map<std::wstring, AppUsageStats>&
ActivitySessionManager::getStats() const
{
    return stats;
}
