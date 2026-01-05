#include "ActivitySessionManager.h"
#include "ForegroundTracker.h"

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




const std::unordered_map<std::wstring, AppUsageStats>&
ActivitySessionManager::getStats() const
{
    return stats;
}
