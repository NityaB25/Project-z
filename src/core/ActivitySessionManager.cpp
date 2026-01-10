#include "ActivitySessionManager.h"
#include "ForegroundTracker.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <shlobj.h>
#pragma comment(lib, "Shell32.lib")

/* ---------------- Session Time Tracking ---------------- */

static std::chrono::system_clock::time_point sessionStart;
static std::chrono::system_clock::time_point sessionEnd;

/* ---------------- Helper: Session Directory ---------------- */

static std::wstring GetSessionDir()
{
    wchar_t path[MAX_PATH]{};
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path);

    std::wstring dir = std::wstring(path) + L"\\ProjectZ\\sessions";
    CreateDirectoryW(dir.c_str(), nullptr);

    return dir;
}

/* ---------------- Core Logic ---------------- */

void ActivitySessionManager::update(const std::wstring& foregroundApp)
{
    for (auto& [app, data] : stats)
    {
        if (!IsProcessRunning(app))
            continue;

        if (!foregroundApp.empty() && app == foregroundApp)
            data.foreground++;
        else
            data.background++;
    }
}

void ActivitySessionManager::registerApp(const std::wstring& app)
{
    stats[app]; // create entry if missing
}

void ActivitySessionManager::removeApp(const std::wstring& app)
{
    stats.erase(app);
}

void ActivitySessionManager::clear()
{
    stats.clear();
}

/* ---------------- Session Persistence ---------------- */

void ActivitySessionManager::saveSessionToDisk() const
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_s(&tm, &t);

    std::wstringstream name;
    name << L"session_"
         << std::put_time(&tm, L"%Y-%m-%d_%H-%M-%S")
         << L".csv";

    std::wstring path = GetSessionDir() + L"\\" + name.str();

    std::wofstream file(path);
    if (!file.is_open())
        return;

    file << exportCSV();
    file.close();
}

/* ---------------- CSV Export ---------------- */

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

/* ---------------- Session Summary ---------------- */

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

    auto duration =
        duration_cast<seconds>(sessionEnd - sessionStart).count();

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


std::vector<std::wstring> ActivitySessionManager::listSavedSessions()
{
    std::vector<std::wstring> sessions;

    std::wstring dir = GetSessionDir();

    if (!std::filesystem::exists(dir))
        return sessions;

    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.is_regular_file() &&
            entry.path().extension() == L".csv")
        {
            sessions.push_back(entry.path().filename().wstring());
        }
    }

    return sessions;
}


/* ---------------- Accessors ---------------- */

const std::unordered_map<std::wstring, AppUsageStats>&
ActivitySessionManager::getStats() const
{
    return stats;
}
