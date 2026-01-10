#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <fstream>
#include <string>

#include "../core/SensitiveZoneManager.h"
#include "../core/ForegroundTracker.h"
#include "../core/ActivitySessionManager.h"

#pragma comment(lib, "comctl32.lib")

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

/* ===================== Globals ===================== */

HWND hListApps    = nullptr;
HWND hListStats   = nullptr;
HWND hListHistory = nullptr;
HWND hBtnSmart    = nullptr;

SensitiveZoneManager   g_zone;
ActivitySessionManager g_session;

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

    for (int i = 0; i < count; i++)
    {
        ListView_GetItemText(list, i, 0, buf, 260);
        if (text == buf) return i;
    }
    return -1;
}

void AddAppToList(HWND list, const std::wstring& app)
{
    if (FindRow(list, app) != -1) return;

    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(list);
    item.pszText = (LPWSTR)app.c_str();
    ListView_InsertItem(list, &item);
}

void LoadSensitiveApps()
{
    ListView_DeleteAllItems(hListApps);
    for (const auto& app : g_zone.getAllApps())
        AddAppToList(hListApps, app);
}

void UpdateDashboard()
{
    for (const auto& [app, data] : g_session.getStats())
    {
        // 🔒 HARD FILTER
        if (!g_zone.isSensitive(app))
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

void LoadSessionHistory()
{
    ListView_DeleteAllItems(hListHistory);
    for (const auto& s : ActivitySessionManager::listSavedSessions())
        AddAppToList(hListHistory, s);
}

void ExportCSVFile(HWND hwnd)
{
    OPENFILENAMEW ofn{};
    wchar_t path[MAX_PATH] = L"session.csv";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    if (!GetSaveFileNameW(&ofn)) return;

    std::wofstream file(path);
    file << g_session.exportCSV();
    file.close();

    MessageBoxW(hwnd, L"CSV exported successfully.", L"Export", MB_OK);
}

/* ===================== Window Procedure ===================== */

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Add App:", WS_CHILD | WS_VISIBLE,
            10, 10, 70, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
            90, 8, 180, 22, hwnd, (HMENU)ID_EDIT_APP, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE,
            280, 8, 60, 22, hwnd, (HMENU)ID_BTN_ADD, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE,
            350, 8, 80, 22, hwnd, (HMENU)ID_BTN_REMOVE, nullptr, nullptr);

        hBtnSmart = CreateWindowW(L"BUTTON", L"Smart Monitor: OFF",
            WS_CHILD | WS_VISIBLE,
            440, 8, 170, 22, hwnd, (HMENU)ID_BTN_SMART, nullptr, nullptr);

        hListApps = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            10, 40, 220, 150, hwnd, (HMENU)ID_LIST_APPS, nullptr, nullptr);

        hListStats = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
            240, 40, 380, 150, hwnd, (HMENU)ID_LIST_STATS, nullptr, nullptr);

        hListHistory = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
            10, 230, 610, 120, hwnd, (HMENU)ID_LIST_HISTORY, nullptr, nullptr);

        ListView_InsertColumn(hListApps, 0, &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 200, (LPWSTR)L"Sensitive Apps" }));
        ListView_InsertColumn(hListStats, 0, &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 100, (LPWSTR)L"App" }));
        ListView_InsertColumn(hListStats, 1, &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 90, (LPWSTR)L"Foreground" }));
        ListView_InsertColumn(hListStats, 2, &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 90, (LPWSTR)L"Background" }));
        ListView_InsertColumn(hListStats, 3, &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 90, (LPWSTR)L"Total" }));
        ListView_InsertColumn(hListHistory, 0, &(LVCOLUMNW{ LVCF_TEXT | LVCF_WIDTH, 0, 580, (LPWSTR)L"Saved Sessions" }));

        CreateWindowW(L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE,
            10, 200, 80, 26, hwnd, (HMENU)ID_BTN_START, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Stop", WS_CHILD | WS_VISIBLE,
            100, 200, 80, 26, hwnd, (HMENU)ID_BTN_STOP, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Export CSV", WS_CHILD | WS_VISIBLE,
            190, 200, 120, 26, hwnd, (HMENU)ID_BTN_EXPORT, nullptr, nullptr);

        g_zone.loadFromDisk();
        LoadSensitiveApps();
        LoadSessionHistory();

        SetTimer(hwnd, ID_TIMER_TRACK, 1000, nullptr);
        break;
    }

    case WM_TIMER:
        if (g_logging)
        {
            std::wstring fg = GetForegroundProcessName();

            if (g_smart && !fg.empty() && !g_zone.isSensitive(fg) &&
                fg != L"explorer.exe")
            {
                g_zone.addApp(fg);
                g_session.registerApp(fg);
                AddAppToList(hListApps, fg);
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

        case ID_BTN_START:
            g_logging = true;
            g_session.clear();
            for (const auto& app : g_zone.getAllApps())
                g_session.registerApp(app);
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


                case ID_BTN_REMOVE:
{
    int index = ListView_GetNextItem(hListApps, -1, LVNI_SELECTED);
    if (index == -1) break;

    wchar_t buf[260]{};
    ListView_GetItemText(hListApps, index, 0, buf, 260);
    std::wstring app = buf;

    // 1. Remove from Sensitive Zone
    g_zone.removeApp(app);

    // 2. Remove from active session tracking
    g_session.removeApp(app);

    // 3. Remove from Sensitive Apps UI
    ListView_DeleteItem(hListApps, index);

    // 4. Remove from Dashboard UI
    int row = FindRow(hListStats, app);
    if (row != -1)
        ListView_DeleteItem(hListStats, row);

    break;
}

        }
        break;

    case WM_DESTROY:
        g_zone.saveToDisk();
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
        L"Project Z – Sensitive App Auditor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 500,
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
