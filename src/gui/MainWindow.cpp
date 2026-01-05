#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <fstream>

#include "../core/SensitiveZoneManager.h"
#include "../core/ForegroundTracker.h"
#include "../core/ActivitySessionManager.h"

#include <string>

#pragma comment(lib, "comctl32.lib")

#define ID_EDIT_APP     101
#define ID_BTN_ADD      102
#define ID_BTN_REMOVE   103
#define ID_LIST_APPS    104
#define ID_LIST_STATS   105
#define ID_BTN_START    106
#define ID_BTN_STOP     107
#define ID_BTN_EXPORT   108
#define ID_TIMER_TRACK  200

HWND hListApps = nullptr;
HWND hListStats = nullptr;

SensitiveZoneManager g_zone;
ActivitySessionManager g_session;

bool g_logging = false;

/* -------------------- Helper Functions -------------------- */

void AddAppToListView(const std::wstring& app)
{
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(hListApps);
    item.pszText = (LPWSTR)app.c_str();
    ListView_InsertItem(hListApps, &item);
}

std::wstring RemoveSelectedAppFromListView()
{
    int index = ListView_GetNextItem(hListApps, -1, LVNI_SELECTED);
    if (index == -1)
        return L"";

    wchar_t buffer[260]{};
    ListView_GetItemText(hListApps, index, 0, buffer, 260);
    ListView_DeleteItem(hListApps, index);
    return buffer;
}

void LoadSensitiveAppsIntoUI()
{
    ListView_DeleteAllItems(hListApps);
    for (const auto& app : g_zone.getAllApps())
        AddAppToListView(app);
}

std::wstring FormatTime(uint64_t seconds)
{
    wchar_t buf[32];
    swprintf_s(buf, L"%02llu:%02llu", seconds / 60, seconds % 60);
    return buf;
}

int FindDashboardRow(const std::wstring& app)
{
    int count = ListView_GetItemCount(hListStats);
    wchar_t buf[260];

    for (int i = 0; i < count; ++i)
    {
        ListView_GetItemText(hListStats, i, 0, buf, 260);
        if (app == buf)
            return i;
    }
    return -1;
}

void UpdateDashboard()
{
    const auto& stats = g_session.getStats();

    for (const auto& [app, data] : stats)
    {
        int row = FindDashboardRow(app);

        std::wstring fg = FormatTime(data.foreground);
        std::wstring bg = FormatTime(data.background);
        std::wstring total = FormatTime(data.foreground + data.background);

        if (row == -1)
        {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = ListView_GetItemCount(hListStats);
            item.pszText = (LPWSTR)app.c_str();
            row = ListView_InsertItem(hListStats, &item);
        }

        ListView_SetItemText(hListStats, row, 1, (LPWSTR)fg.c_str());
        ListView_SetItemText(hListStats, row, 2, (LPWSTR)bg.c_str());
        ListView_SetItemText(hListStats, row, 3, (LPWSTR)total.c_str());
    }
}

void ExportCSVFile(HWND hwnd)
{
    OPENFILENAMEW ofn{};
    wchar_t fileName[MAX_PATH] = L"project_z_session.csv";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";

    if (!GetSaveFileNameW(&ofn))
        return;

    std::wofstream file(ofn.lpstrFile);
    file << g_session.exportCSV();
    file.close();

    MessageBoxW(hwnd, L"CSV exported successfully.", L"Export", MB_OK);
}

/* -------------------- Window Procedure -------------------- */

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"Add App:", WS_VISIBLE | WS_CHILD,
            10, 10, 60, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            80, 8, 180, 22, hwnd, (HMENU)ID_EDIT_APP, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Add", WS_VISIBLE | WS_CHILD,
            270, 8, 60, 22, hwnd, (HMENU)ID_BTN_ADD, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Remove Selected", WS_VISIBLE | WS_CHILD,
            340, 8, 130, 22, hwnd, (HMENU)ID_BTN_REMOVE, nullptr, nullptr);

        hListApps = CreateWindowW(WC_LISTVIEWW, L"",
            WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
            10, 40, 220, 150, hwnd, (HMENU)ID_LIST_APPS, nullptr, nullptr);

        ListView_SetExtendedListViewStyle(
            hListApps, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW colApps{ LVCF_TEXT | LVCF_WIDTH, 0, 200, (LPWSTR)L"Sensitive Apps" };
        ListView_InsertColumn(hListApps, 0, &colApps);

        hListStats = CreateWindowW(WC_LISTVIEWW, L"",
            WS_VISIBLE | WS_CHILD | LVS_REPORT,
            240, 40, 380, 150, hwnd, (HMENU)ID_LIST_STATS, nullptr, nullptr);

        LVCOLUMNW col{ LVCF_TEXT | LVCF_WIDTH };
        col.cx = 100; col.pszText = (LPWSTR)L"App";
        ListView_InsertColumn(hListStats, 0, &col);
        col.cx = 90; col.pszText = (LPWSTR)L"Foreground";
        ListView_InsertColumn(hListStats, 1, &col);
        col.cx = 90; col.pszText = (LPWSTR)L"Background";
        ListView_InsertColumn(hListStats, 2, &col);
        col.cx = 90; col.pszText = (LPWSTR)L"Total";
        ListView_InsertColumn(hListStats, 3, &col);

        CreateWindowW(L"BUTTON", L"Start Logging", WS_VISIBLE | WS_CHILD,
            10, 200, 120, 30, hwnd, (HMENU)ID_BTN_START, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Stop", WS_VISIBLE | WS_CHILD,
            140, 200, 80, 30, hwnd, (HMENU)ID_BTN_STOP, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"Export CSV", WS_VISIBLE | WS_CHILD,
            230, 200, 120, 30, hwnd, (HMENU)ID_BTN_EXPORT, nullptr, nullptr);

        LoadSensitiveAppsIntoUI();
        SetTimer(hwnd, ID_TIMER_TRACK, 1000, nullptr);
        break;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER_TRACK && g_logging)
        {
            g_session.update(GetForegroundProcessName());
            UpdateDashboard();
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BTN_ADD:
        {
            wchar_t buf[260]{};
            GetDlgItemTextW(hwnd, ID_EDIT_APP, buf, 260);
            std::wstring app = buf;
            if (!app.empty() && g_zone.addApp(app))
            {
                AddAppToListView(app);
                SetDlgItemTextW(hwnd, ID_EDIT_APP, L"");
            }
            break;
        }

        case ID_BTN_REMOVE:
        {
            std::wstring removed = RemoveSelectedAppFromListView();
            if (!removed.empty())
            {
                g_zone.removeApp(removed);
                g_session.removeApp(removed);
                int row = FindDashboardRow(removed);
                if (row != -1) ListView_DeleteItem(hListStats, row);
            }
            break;
        }

        case ID_BTN_START:
            g_logging = true;
            g_session.startSession();
            g_session.clear();
            ListView_DeleteAllItems(hListStats);
            for (const auto& app : g_zone.getAllApps())
                g_session.registerApp(app);
            break;

        case ID_BTN_STOP:
            g_logging = false;
            g_session.stopSession();
            MessageBoxW(hwnd,
                g_session.getSessionSummary().c_str(),
                L"Session Summary", MB_OK);
            break;

        case ID_BTN_EXPORT:
            ExportCSVFile(hwnd);
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

/* -------------------- Entry Point -------------------- */

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
        650, 300,
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
