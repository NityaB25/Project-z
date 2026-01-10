#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <fstream>
#include <string>
#include <unordered_set>

#include "../core/ForegroundTracker.h"
#include "../core/ActivitySessionManager.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shell32.lib")

/* ===================== Control IDs ===================== */

#define ID_EDIT_APP      101
#define ID_BTN_ADD       102
#define ID_BTN_REMOVE    103
#define ID_LIST_APPS     104
#define ID_LIST_STATS    105
#define ID_BTN_START     106
#define ID_BTN_STOP      107
#define ID_BTN_EXPORT    108
#define ID_BTN_SMART     109
#define ID_TIMER_TRACK   200
#define ID_LIST_HISTORY  300
#define ID_BTN_OPEN_DIR  301

/* ===================== Globals ===================== */

HWND hListApps    = nullptr;
HWND hListStats   = nullptr;
HWND hListHistory = nullptr;
HWND hEditApp     = nullptr;
HWND hBtnSmart    = nullptr;

ActivitySessionManager g_session;
std::unordered_set<std::wstring> g_trackedApps;

bool g_logging = false;
bool g_smart   = false;

/* ===================== Helpers ===================== */

std::wstring FormatTime(uint64_t sec)
{
    wchar_t buf[32];
    swprintf_s(buf, L"%02llu:%02llu", sec / 60, sec % 60);
    return buf;
}

int FindRow(HWND list, const std::wstring& text)
{
    int count = ListView_GetItemCount(list);
    wchar_t buf[260];

    for (int i = 0; i < count; ++i)
    {
        ListView_GetItemText(list, i, 0, buf, 260);
        if (text == buf)
            return i;
    }
    return -1;
}

void AddUniqueItem(HWND list, const std::wstring& text)
{
    if (FindRow(list, text) != -1)
        return;

    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(list);
    item.pszText = (LPWSTR)text.c_str();
    ListView_InsertItem(list, &item);
}

void RemoveItem(HWND list, const std::wstring& text)
{
    int row = FindRow(list, text);
    if (row != -1)
        ListView_DeleteItem(list, row);
}

/* ===================== Dashboard ===================== */

void UpdateDashboard()
{
    for (const auto& [app, data] : g_session.getStats())
    {
        if (g_trackedApps.find(app) == g_trackedApps.end())
            continue;

        int row = FindRow(hListStats, app);
        if (row == -1)
        {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = ListView_GetItemCount(hListStats);
            item.pszText = (LPWSTR)app.c_str();
            row = ListView_InsertItem(hListStats, &item);
        }

        ListView_SetItemText(hListStats, row, 1,
            (LPWSTR)FormatTime(data.foreground).c_str());
        ListView_SetItemText(hListStats, row, 2,
            (LPWSTR)FormatTime(data.background).c_str());
        ListView_SetItemText(hListStats, row, 3,
            (LPWSTR)FormatTime(data.foreground + data.background).c_str());
    }
}

/* ===================== Session History ===================== */

void LoadSessionHistory()
{
    ListView_DeleteAllItems(hListHistory);
    for (const auto& s : ActivitySessionManager::listSavedSessions())
        AddUniqueItem(hListHistory, s);
}

void OpenSessionsFolder()
{
    wchar_t path[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path);

    std::wstring dir = std::wstring(path) + L"\\ProjectZ\\sessions";
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
}

void ExportCSVFile(HWND hwnd)
{
    OPENFILENAMEW ofn{};
    wchar_t file[MAX_PATH] = L"session.csv";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    if (!GetSaveFileNameW(&ofn))
        return;

    std::wofstream out(file);
    out << g_session.exportCSV();
    out.close();

    MessageBoxW(hwnd, L"CSV exported successfully.", L"Export", MB_OK);
}

/* ===================== Window Procedure ===================== */

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Add App:",
            WS_CHILD | WS_VISIBLE, 10, 10, 60, 20,
            hwnd, nullptr, nullptr, nullptr);

        hEditApp = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            80, 8, 180, 22,
            hwnd, (HMENU)ID_EDIT_APP, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Add",
            WS_CHILD | WS_VISIBLE,
            270, 8, 60, 22,
            hwnd, (HMENU)ID_BTN_ADD, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Remove",
            WS_CHILD | WS_VISIBLE,
            340, 8, 80, 22,
            hwnd, (HMENU)ID_BTN_REMOVE, nullptr, nullptr);

        hBtnSmart = CreateWindowW(L"BUTTON", L"Smart Monitor: OFF",
            WS_CHILD | WS_VISIBLE,
            430, 8, 180, 22,
            hwnd, (HMENU)ID_BTN_SMART, nullptr, nullptr);

        hListApps = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            10, 40, 220, 150,
            hwnd, (HMENU)ID_LIST_APPS, nullptr, nullptr);

        hListStats = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
            240, 40, 380, 150,
            hwnd, (HMENU)ID_LIST_STATS, nullptr, nullptr);

        hListHistory = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
            10, 230, 610, 120,
            hwnd, (HMENU)ID_LIST_HISTORY, nullptr, nullptr);

        ListView_InsertColumn(hListApps, 0,
            &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 200, (LPWSTR)L"Tracked Apps" }));

        ListView_InsertColumn(hListStats, 0,
            &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 120, (LPWSTR)L"App" }));
        ListView_InsertColumn(hListStats, 1,
            &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 90, (LPWSTR)L"Foreground" }));
        ListView_InsertColumn(hListStats, 2,
            &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 90, (LPWSTR)L"Background" }));
        ListView_InsertColumn(hListStats, 3,
            &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 90, (LPWSTR)L"Total" }));

        ListView_InsertColumn(hListHistory, 0,
            &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 580, (LPWSTR)L"Saved Sessions" }));

        CreateWindowW(L"BUTTON", L"Start",
            WS_CHILD | WS_VISIBLE, 10, 200, 80, 26,
            hwnd, (HMENU)ID_BTN_START, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Stop",
            WS_CHILD | WS_VISIBLE, 100, 200, 80, 26,
            hwnd, (HMENU)ID_BTN_STOP, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Export CSV",
            WS_CHILD | WS_VISIBLE, 190, 200, 120, 26,
            hwnd, (HMENU)ID_BTN_EXPORT, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Open Sessions Folder",
            WS_CHILD | WS_VISIBLE, 320, 200, 200, 26,
            hwnd, (HMENU)ID_BTN_OPEN_DIR, nullptr, nullptr);

        LoadSessionHistory();
        SetTimer(hwnd, ID_TIMER_TRACK, 1000, nullptr);
        break;
    }

    case WM_TIMER:
        if (g_logging)
        {
            std::wstring fg = GetForegroundProcessName();

            if (g_smart && !fg.empty() && g_trackedApps.insert(fg).second)
            {
                g_session.addApp(fg);
                AddUniqueItem(hListApps, fg);
            }

            g_session.update(fg);
            UpdateDashboard();
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BTN_SMART:
            g_smart = !g_smart;
            SetWindowTextW(hBtnSmart,
                g_smart ? L"Smart Monitor: ON" : L"Smart Monitor: OFF");
            break;

        case ID_BTN_ADD:
        {
            wchar_t buf[260]{};
            GetDlgItemTextW(hwnd, ID_EDIT_APP, buf, 260);
            std::wstring app = buf;

            if (!app.empty() && g_trackedApps.insert(app).second)
            {
                g_session.addApp(app);
                AddUniqueItem(hListApps, app);
            }
            break;
        }

        case ID_BTN_REMOVE:
        {
            int index = ListView_GetNextItem(hListApps, -1, LVNI_SELECTED);
            if (index == -1) break;

            wchar_t buf[260]{};
            ListView_GetItemText(hListApps, index, 0, buf, 260);
            std::wstring app = buf;

            g_trackedApps.erase(app);
            g_session.removeApp(app);

            ListView_DeleteItem(hListApps, index);
            RemoveItem(hListStats, app);
            break;
        }

        case ID_BTN_START:
            g_logging = true;
            g_session.clear();
            for (const auto& app : g_trackedApps)
                g_session.addApp(app);
            break;

        case ID_BTN_STOP:
            g_logging = false;
            g_session.saveSessionToDisk();
            LoadSessionHistory();
            MessageBoxW(hwnd,
                g_session.getSessionSummary().c_str(),
                L"Session Summary", MB_OK);
            break;

        case ID_BTN_EXPORT:
            ExportCSVFile(hwnd);
            break;

        case ID_BTN_OPEN_DIR:
            OpenSessionsFolder();
            break;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_TRACK);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ===================== Entry ===================== */

int RunMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ProjectZWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"Project Z – App Usage Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        820, 520,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
