
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <commctrl.h>
#include "../core/SensitiveZoneManager.h"
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

HWND hListApps = nullptr;
HWND hListStats = nullptr;
SensitiveZoneManager g_zone;

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
    {
        AddAppToListView(app);
    }
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // Label
        CreateWindowW(
            L"STATIC", L"Add App:",
            WS_VISIBLE | WS_CHILD,
            10, 10, 60, 20,
            hwnd, nullptr, nullptr, nullptr
        );

        // Edit box
        CreateWindowW(
            L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            80, 8, 180, 22,
            hwnd, (HMENU)ID_EDIT_APP, nullptr, nullptr
        );

        // Add button
        CreateWindowW(
            L"BUTTON", L"Add",
            WS_VISIBLE | WS_CHILD,
            270, 8, 60, 22,
            hwnd, (HMENU)ID_BTN_ADD, nullptr, nullptr
        );

        // Remove button
        CreateWindowW(
            L"BUTTON", L"Remove Selected",
            WS_VISIBLE | WS_CHILD,
            340, 8, 130, 22,
            hwnd, (HMENU)ID_BTN_REMOVE, nullptr, nullptr
        );

        // Sensitive Apps ListView
      hListApps = CreateWindowW(
    WC_LISTVIEWW, L"",
    WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
    10, 40, 220, 150,
    hwnd, (HMENU)ID_LIST_APPS, nullptr, nullptr
);
ListView_SetExtendedListViewStyle(
    hListApps,
    LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES
);

        // Column for Sensitive Apps
        LVCOLUMNW colApps{};
        colApps.mask = LVCF_TEXT | LVCF_WIDTH;
        colApps.cx = 200;
        colApps.pszText = (LPWSTR)L"Sensitive Apps";
        ListView_InsertColumn(hListApps, 0, &colApps);

        // Dashboard ListView
        hListStats = CreateWindowW(
            WC_LISTVIEWW, L"",
            WS_VISIBLE | WS_CHILD | LVS_REPORT,
            240, 40, 380, 150,
            hwnd, (HMENU)ID_LIST_STATS, nullptr, nullptr
        );

        // Dashboard columns
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        col.cx = 100;
        col.pszText = (LPWSTR)L"App";
        ListView_InsertColumn(hListStats, 0, &col);

        col.cx = 90;
        col.pszText = (LPWSTR)L"Foreground";
        ListView_InsertColumn(hListStats, 1, &col);

        col.cx = 90;
        col.pszText = (LPWSTR)L"Background";
        ListView_InsertColumn(hListStats, 2, &col);

        col.cx = 90;
        col.pszText = (LPWSTR)L"Total";
        ListView_InsertColumn(hListStats, 3, &col);

        // Start Logging button
        CreateWindowW(
            L"BUTTON", L"Start Logging",
            WS_VISIBLE | WS_CHILD,
            10, 200, 120, 30,
            hwnd, (HMENU)ID_BTN_START, nullptr, nullptr
        );

        // Stop button
        CreateWindowW(
            L"BUTTON", L"Stop",
            WS_VISIBLE | WS_CHILD,
            140, 200, 80, 30,
            hwnd, (HMENU)ID_BTN_STOP, nullptr, nullptr
        );

        // Export button
        CreateWindowW(
            L"BUTTON", L"Export CSV",
            WS_VISIBLE | WS_CHILD,
            230, 200, 120, 30,
            hwnd, (HMENU)ID_BTN_EXPORT, nullptr, nullptr
        );
        LoadSensitiveAppsIntoUI();

        break;
    }





 case WM_COMMAND:
{
    switch (LOWORD(wParam))
    {
    case ID_BTN_ADD:
    {
        wchar_t buffer[260]{};
        GetDlgItemTextW(hwnd, ID_EDIT_APP, buffer, 260);

        std::wstring app = buffer;
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
        }
        break;
    }

    }
    break;
}


    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int RunMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
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
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
