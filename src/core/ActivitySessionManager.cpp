#include "ActivitySessionManager.h"
#include <windows.h>
#include <ShlObj.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

/* ===================== Session Timing ===================== */

static chrono::system_clock::time_point g_sessionStart;
static chrono::system_clock::time_point g_sessionEnd;

/* ===================== Helpers ===================== */

static wstring GetSessionsDir()
{
    wchar_t path[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path);

    wstring dir = wstring(path) + L"\\ProjectZ\\sessions";
    CreateDirectoryW((wstring(path) + L"\\ProjectZ").c_str(), nullptr);
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

/* ===================== Core Lifecycle ===================== */

void ActivitySessionManager::startSession()
{
    g_sessionStart = chrono::system_clock::now();
}

void ActivitySessionManager::stopSession()
{
    g_sessionEnd = chrono::system_clock::now();
}

void ActivitySessionManager::update(const wstring& foregroundApp)
{
    for (auto& [app, data] : stats)
    {
        if (!foregroundApp.empty() && app == foregroundApp)
            data.foreground++;
        else
            data.background++;
    }
}

/* ===================== App Management ===================== */

bool ActivitySessionManager::addApp(const wstring& app)
{
    return stats.emplace(app, AppUsageStats{}).second;
}

void ActivitySessionManager::removeApp(const wstring& app)
{
    stats.erase(app);
}

bool ActivitySessionManager::hasApp(const wstring& app) const
{
    return stats.find(app) != stats.end();
}

void ActivitySessionManager::clear()
{
    stats.clear();
}

/* ===================== Data Access ===================== */

const unordered_map<wstring, AppUsageStats>&
ActivitySessionManager::getStats() const
{
    return stats;
}

/* ===================== Export ===================== */

wstring ActivitySessionManager::exportCSV() const
{
    wstring csv = L"App,Foreground(sec),Background(sec),Total(sec)\n";

    for (const auto& [app, data] : stats)
    {
        uint64_t total = data.foreground + data.background;
        csv += app + L"," +
               to_wstring(data.foreground) + L"," +
               to_wstring(data.background) + L"," +
               to_wstring(total) + L"\n";
    }
    return csv;
}

/* ===================== Session Persistence ===================== */

void ActivitySessionManager::saveSessionToDisk() const
{
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);

    tm tm{};
    localtime_s(&tm, &t);

    wstringstream name;
    name << L"session_"
         << put_time(&tm, L"%Y-%m-%d_%H-%M-%S")
         << L".csv";

    wstring path = GetSessionsDir() + L"\\" + name.str();

    wofstream file(path);
    if (!file.is_open())
        return;

    file << exportCSV();
    file.close();
}

vector<wstring> ActivitySessionManager::listSavedSessions()
{
    vector<wstring> sessions;
    wstring dir = GetSessionsDir();

    for (const auto& entry : filesystem::directory_iterator(dir))
    {
        if (entry.path().extension() == L".csv")
            sessions.push_back(entry.path().filename().wstring());
    }
    return sessions;
}

/* ===================== Summary ===================== */

wstring ActivitySessionManager::getSessionSummary() const
{
    using namespace chrono;

    auto duration =
        duration_cast<seconds>(g_sessionEnd - g_sessionStart).count();

    wstring topApp = L"N/A";
    uint64_t maxFg = 0;

    for (const auto& [app, data] : stats)
    {
        if (data.foreground > maxFg)
        {
            maxFg = data.foreground;
            topApp = app;
        }
    }

    wstring summary;
    summary += L"Session Duration: " + to_wstring(duration) + L" seconds\n";
    summary += L"Tracked Apps: " + to_wstring(stats.size()) + L"\n";
    summary += L"Top App (Foreground): " + topApp + L"\n";

    return summary;
}
