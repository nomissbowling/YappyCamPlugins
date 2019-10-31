// Clock_yap.cpp --- PluginFramework Plugin #1
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "../Plugin.h"
#include "../mregkey.hpp"
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <cassert>
#include <strsafe.h>
#include "resource.h"

enum ALIGN
{
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};
enum VALIGN
{
    VALIGN_TOP,
    VALIGN_MIDDLE,
    VALIGN_BOTTOM
};

static HINSTANCE s_hinstDLL;
static HWND s_hwnd;
static std::string s_strCaption;
static double s_eScale;
static INT s_nAlign;
static INT s_nVAlign;
static INT s_nMargin;
static INT s_nWindowX;
static INT s_nWindowY;
static BOOL s_bDialogInit = FALSE;

LPTSTR LoadStringDx(INT nID)
{
    static UINT s_index = 0;
    const UINT cchBuffMax = 1024;
    static TCHAR s_sz[2][cchBuffMax];

    TCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % ARRAYSIZE(s_sz);
    pszBuff[0] = 0;
    if (!::LoadString(s_hinstDLL, nID, pszBuff, cchBuffMax))
        assert(0);
    return pszBuff;
}

LPSTR ansi_from_wide(LPCWSTR pszWide)
{
    static char s_buf[256];
    WideCharToMultiByte(CP_ACP, 0, pszWide, -1, s_buf, ARRAYSIZE(s_buf), NULL, NULL);
    return s_buf;
}

LPWSTR wide_from_ansi(LPCSTR pszAnsi)
{
    static WCHAR s_buf[256];
    MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, s_buf, ARRAYSIZE(s_buf));
    return s_buf;
}

std::string DoGetCaption(const char *fmt, const SYSTEMTIME& st)
{
    std::string ret;

    char buf[32];
    for (const char *pch = fmt; *pch; ++pch)
    {
        if (*pch != '&')
        {
            ret += *pch;
            continue;
        }

        ++pch;
        switch (*pch)
        {
        case '&':
            ret += '&';
            break;
        case 'y':
            StringCbPrintfA(buf, sizeof(buf), "%04u", st.wYear);
            ret += buf;
            break;
        case 'M':
            StringCbPrintfA(buf, sizeof(buf), "%02u", st.wMonth);
            ret += buf;
            break;
        case 'd':
            StringCbPrintfA(buf, sizeof(buf), "%02u", st.wDay);
            ret += buf;
            break;
        case 'h':
            StringCbPrintfA(buf, sizeof(buf), "%02u", st.wHour);
            ret += buf;
            break;
        case 'm':
            StringCbPrintfA(buf, sizeof(buf), "%02u", st.wMinute);
            ret += buf;
            break;
        case 's':
            StringCbPrintfA(buf, sizeof(buf), "%02u", st.wSecond);
            ret += buf;
            break;
        case 'f':
            StringCbPrintfA(buf, sizeof(buf), "%03u", st.wMilliseconds);
            ret += buf;
            break;
        default:
            ret += '&';
            ret += *pch;
            break;
        }
    }

    return ret;
}

extern "C" {

static LRESULT DoLoadSettings(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
{
    s_nMargin = 3;
    s_nAlign = ALIGN_RIGHT;
    s_nVAlign = VALIGN_TOP;
    s_eScale = 0.2;
    s_strCaption = "&h:&m:&s.&f";
    s_nWindowX = CW_USEDEFAULT;
    s_nWindowY = CW_USEDEFAULT;

    MRegKey hkeyCompany(HKEY_CURRENT_USER,
                        TEXT("Software\\Katayama Hirofumi MZ"),
                        FALSE);
    if (!hkeyCompany)
        return FALSE;

    MRegKey hkeyApp(hkeyCompany, TEXT("Clock_yap"), FALSE);
    if (!hkeyApp)
        return FALSE;

    hkeyApp.QueryDword(TEXT("Margin"), (DWORD&)s_nMargin);
    hkeyApp.QueryDword(TEXT("Align"), (DWORD&)s_nAlign);
    hkeyApp.QueryDword(TEXT("VAlign"), (DWORD&)s_nVAlign);
    hkeyApp.QueryDword(TEXT("WindowX"), (DWORD&)s_nWindowX);
    hkeyApp.QueryDword(TEXT("WindowY"), (DWORD&)s_nWindowY);

    DWORD dwValue;
    TCHAR szText[64];

    if (!hkeyApp.QueryDword(TEXT("Scale"), (DWORD&)dwValue))
    {
        s_eScale = dwValue / 100.0;
    }

    if (!hkeyApp.QuerySz(TEXT("Caption"), szText, ARRAYSIZE(szText)))
    {
        s_strCaption = ansi_from_wide(szText);
    }

    return TRUE;
}

static LRESULT DoSaveSettings(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
{
    MRegKey hkeyCompany(HKEY_CURRENT_USER,
                        TEXT("Software\\Katayama Hirofumi MZ"),
                        TRUE);
    if (!hkeyCompany)
        return FALSE;

    MRegKey hkeyApp(hkeyCompany, TEXT("Clock_yap"), TRUE);
    if (!hkeyApp)
        return FALSE;

    hkeyApp.SetDword(TEXT("Margin"), s_nMargin);
    hkeyApp.SetDword(TEXT("Align"), s_nAlign);
    hkeyApp.SetDword(TEXT("VAlign"), s_nVAlign);
    hkeyApp.SetDword(TEXT("WindowX"), s_nWindowX);
    hkeyApp.SetDword(TEXT("WindowY"), s_nWindowY);

    DWORD dwValue = DWORD(s_eScale * 100);
    hkeyApp.SetDword(TEXT("Scale"), (DWORD&)dwValue);

    TCHAR szText[64];
    hkeyApp.SetSz(TEXT("Caption"), wide_from_ansi(s_strCaption.c_str()));

    return TRUE;
}

// API Name: Plugin_Load
// Purpose: The framework want to load the plugin component.
// TODO: Load the plugin component.
BOOL APIENTRY
Plugin_Load(PLUGIN *pi, LPARAM lParam)
{
    if (!pi)
    {
        assert(0);
        return FALSE;
    }
    if (pi->framework_version < FRAMEWORK_VERSION)
    {
        assert(0);
        return FALSE;
    }
    if (lstrcmpi(pi->framework_name, FRAMEWORK_NAME) != 0)
    {
        assert(0);
        return FALSE;
    }
    if (pi->framework_instance == NULL)
    {
        assert(0);
        return FALSE;
    }

    pi->plugin_version = 1;
    StringCbCopy(pi->plugin_product_name, sizeof(pi->plugin_product_name), TEXT("Clock"));
    StringCbCopy(pi->plugin_filename, sizeof(pi->plugin_filename), TEXT("Clock.yap"));
    StringCbCopy(pi->plugin_company, sizeof(pi->plugin_company), TEXT("Katayama Hirofumi MZ"));
    StringCbCopy(pi->plugin_copyright, sizeof(pi->plugin_copyright), TEXT("Copyright (C) 2019 Katayama Hirofumi MZ"));
    pi->plugin_instance = s_hinstDLL;
    pi->plugin_window = NULL;
    pi->p_user_data = NULL;
    pi->l_user_data = 0;
    pi->dwFlags = PLUGIN_FLAG_PICREADER | PLUGIN_FLAG_PICWRITER;
    pi->bEnabled = TRUE;

    DoLoadSettings(pi, 0, 0);
    return TRUE;
}

// API Name: Plugin_Unload
// Purpose: The framework want to unload the plugin component.
// TODO: Unload the plugin component.
BOOL APIENTRY
Plugin_Unload(PLUGIN *pi, LPARAM lParam)
{
    DoSaveSettings(pi, 0, 0);
    return TRUE;
}

void DoDrawText(cv::Mat& mat, const char *text, double scale, int thickness,
                cv::Scalar& color)
{
    int font = cv::FONT_HERSHEY_SIMPLEX;
    cv::Size screen_size(mat.cols, mat.rows);

    scale *= screen_size.height * 0.01;

    int baseline;
    cv::Size text_size = cv::getTextSize(text, font, scale, thickness, &baseline);

    cv::Point pt;

    switch (s_nAlign)
    {
    case ALIGN_LEFT:
        pt.x = 0;
        pt.x += s_nMargin * screen_size.height / 100;
        break;
    case ALIGN_CENTER:
        pt.x = (screen_size.width - text_size.width + thickness) / 2;
        break;
    case ALIGN_RIGHT:
        pt.x = screen_size.width - text_size.width;
        pt.x -= s_nMargin * screen_size.height / 100;
        pt.x += thickness;
        break;
    default:
        assert(0);
    }

    switch (s_nVAlign)
    {
    case VALIGN_TOP:
        pt.y = 0;
        pt.y += s_nMargin * screen_size.height / 100;
        break;
    case VALIGN_MIDDLE:
        pt.y = (screen_size.height - text_size.height - thickness) / 2;
        break;
    case VALIGN_BOTTOM:
        pt.y = screen_size.height - text_size.height - thickness / 2;
        pt.y -= s_nMargin * screen_size.height / 100;
        pt.y -= baseline;
        pt.y += thickness;
        break;
    default:
        assert(0);
    }
    pt.y += text_size.height - 1;

    cv::putText(mat, text, pt, font, scale,
                color, thickness, cv::LINE_AA, false);
}

inline LRESULT Plugin_PicRead(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
{
    const cv::Mat *pmat = (const cv::Mat *)wParam;
    return 0;
}

static LRESULT Plugin_PicWrite(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
{
    cv::Mat *pmat = (cv::Mat *)wParam;
    if (!pmat || !pmat->data)
        return 0;

    cv::Mat& mat = *pmat;

    SYSTEMTIME st;
    GetLocalTime(&st);

    std::string strText = DoGetCaption(s_strCaption.c_str(), st);
    puts(strText.c_str());

    cv::Scalar black(0, 0, 0);
    cv::Scalar white(255, 255, 255);

    DoDrawText(mat, strText.c_str(), s_eScale, 6, black);
    DoDrawText(mat, strText.c_str(), s_eScale, 2, white);
    return 0;
}

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    s_hwnd = hwnd;

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    ComboBox_AddString(hCmb1, TEXT("&h:&m"));
    ComboBox_AddString(hCmb1, TEXT("&h:&m:&s"));
    ComboBox_AddString(hCmb1, TEXT("&h:&m:&s.%f"));
    ComboBox_AddString(hCmb1, TEXT("&y.&M.&d &h:&m:&s"));
    ComboBox_AddString(hCmb1, TEXT("&y.&M.&d &h:&m:&s.%f"));
    ComboBox_AddString(hCmb1, TEXT("&y.&M.&d"));
    SetDlgItemTextA(hwnd, cmb1, s_strCaption.c_str());

    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    ComboBox_AddString(hCmb2, LoadStringDx(IDS_LEFT));
    ComboBox_AddString(hCmb2, LoadStringDx(IDS_CENTER));
    ComboBox_AddString(hCmb2, LoadStringDx(IDS_RIGHT));
    switch (s_nAlign)
    {
    case ALIGN_LEFT:
        ComboBox_SetCurSel(hCmb2, 0);
        break;
    case ALIGN_CENTER:
        ComboBox_SetCurSel(hCmb2, 1);
        break;
    case ALIGN_RIGHT:
        ComboBox_SetCurSel(hCmb2, 2);
        break;
    }

    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    ComboBox_AddString(hCmb3, LoadStringDx(IDS_TOP));
    ComboBox_AddString(hCmb3, LoadStringDx(IDS_MIDDLE));
    ComboBox_AddString(hCmb3, LoadStringDx(IDS_BOTTOM));
    switch (s_nVAlign)
    {
    case VALIGN_TOP:
        ComboBox_SetCurSel(hCmb3, 0);
        break;
    case VALIGN_MIDDLE:
        ComboBox_SetCurSel(hCmb3, 1);
        break;
    case VALIGN_BOTTOM:
        ComboBox_SetCurSel(hCmb3, 2);
        break;
    }

    DWORD dwValue = DWORD(s_eScale * 100);
    SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELONG(100, 0));
    SendDlgItemMessage(hwnd, scr1, UDM_SETPOS, 0, MAKELONG(dwValue, 0));

    SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELONG(300, 0));
    SendDlgItemMessage(hwnd, scr2, UDM_SETPOS, 0, MAKELONG(s_nMargin, 0));

    s_bDialogInit = TRUE;
    return TRUE;
}

static void OnEdt1(HWND hwnd)
{
    if (!s_bDialogInit)
        return;

    BOOL bTranslated = FALSE;
    INT nValue = GetDlgItemInt(hwnd, edt1, &bTranslated, TRUE);
    if (bTranslated)
    {
        s_eScale = nValue / 100.0;
    }
}

static void OnEdt2(HWND hwnd)
{
    if (!s_bDialogInit)
        return;

    BOOL bTranslated = FALSE;
    INT nValue = GetDlgItemInt(hwnd, edt2, &bTranslated, TRUE);
    if (bTranslated)
    {
        s_nMargin = nValue;
    }
}

static void OnCmb1(HWND hwnd)
{
    if (!s_bDialogInit)
        return;

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);

    TCHAR szText[MAX_PATH];
    INT iItem = ComboBox_GetCurSel(hCmb1);
    if (iItem == CB_ERR)
    {
        ComboBox_GetText(hCmb1, szText, ARRAYSIZE(szText));
    }
    else
    {
        ComboBox_GetLBText(hCmb1, iItem, szText);
    }

    s_strCaption = ansi_from_wide(szText);
}

static void OnCmb2(HWND hwnd)
{
    if (!s_bDialogInit)
        return;

    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    switch (ComboBox_GetCurSel(hCmb2))
    {
    case 0:
        s_nAlign = ALIGN_LEFT;
        break;
    case 1:
        s_nAlign = ALIGN_CENTER;
        break;
    case 2:
        s_nAlign = ALIGN_RIGHT;
        break;
    case CB_ERR:
    default:
        assert(0);
        break;
    }
}

static void OnCmb3(HWND hwnd)
{
    if (!s_bDialogInit)
        return;

    HWND hCmb3 = GetDlgItem(hwnd, cmb3);
    switch (ComboBox_GetCurSel(hCmb3))
    {
    case 0:
        s_nVAlign = VALIGN_TOP;
        break;
    case 1:
        s_nVAlign = VALIGN_MIDDLE;
        break;
    case 2:
        s_nVAlign = VALIGN_BOTTOM;
        break;
    case CB_ERR:
    default:
        assert(0);
        break;
    }
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        DestroyWindow(hwnd);
        break;
    case edt1:
        if (codeNotify == EN_CHANGE)
        {
            OnEdt1(hwnd);
        }
        break;
    case edt2:
        if (codeNotify == EN_CHANGE)
        {
            OnEdt2(hwnd);
        }
        break;
    case cmb1:
        switch (codeNotify)
        {
        case CBN_DROPDOWN:
        case CBN_CLOSEUP:
        case CBN_SETFOCUS:
        case CBN_EDITUPDATE:
        case CBN_SELENDCANCEL:
            break;
        default:
            OnCmb1(hwnd);
            break;
        }
        break;
    case cmb2:
        if (codeNotify == CBN_SELCHANGE)
        {
            OnCmb2(hwnd);
        }
        break;
    case cmb3:
        if (codeNotify == CBN_SELCHANGE)
        {
            OnCmb3(hwnd);
        }
        break;
    }
}

static void OnDestroy(HWND hwnd)
{
    s_hwnd = NULL;
    s_bDialogInit = FALSE;
}

static void OnMove(HWND hwnd, int x, int y)
{
    if (IsMinimized(hwnd) || IsMaximized(hwnd))
        return;

    RECT rc;
    GetWindowRect(hwnd, &rc);
    s_nWindowX = rc.left;
    s_nWindowY = rc.top;
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
    }
    return 0;
}

static LRESULT Plugin_ShowDialog(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
{
    HWND hMainWnd = (HWND)wParam;
    BOOL bShowOrHide = (BOOL)lParam;

    if (bShowOrHide)
    {
        if (IsWindow(pi->plugin_window))
        {
            HWND hPlugin = pi->plugin_window;
            ShowWindow(hPlugin, SW_RESTORE);
            PostMessage(hPlugin, DM_REPOSITION, 0, 0);
            SetForegroundWindow(hPlugin);
            return TRUE;
        }
        else
        {
            CreateDialog(s_hinstDLL, MAKEINTRESOURCE(IDD_CONFIG), hMainWnd, DialogProc);
            if (s_hwnd)
            {
                ShowWindow(s_hwnd, SW_SHOWNORMAL);
                UpdateWindow(s_hwnd);
                return TRUE;
            }
        }
    }
    else
    {
        PostMessage(pi->plugin_window, WM_CLOSE, 0, 0);
        return TRUE;
    }

    return FALSE;
}

// API Name: Plugin_Act
// Purpose: Act something on the plugin.
// TODO: Act something on the plugin.
LRESULT APIENTRY
Plugin_Act(PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam)
{
    switch (uAction)
    {
    case PLUGIN_ACTION_STARTREC:
    case PLUGIN_ACTION_PAUSE:
    case PLUGIN_ACTION_ENDREC:
        break;
    case PLUGIN_ACTION_PICREAD:
        return Plugin_PicRead(pi, wParam, lParam);
    case PLUGIN_ACTION_PICWRITE:
        return Plugin_PicWrite(pi, wParam, lParam);
    case PLUGIN_ACTION_SHOWDIALOG:
        return Plugin_ShowDialog(pi, wParam, lParam);
    }
    return 0;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        s_hinstDLL = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

} // extern "C"
