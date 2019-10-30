// Clock_yap.cpp --- PluginFramework Plugin #1
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "../Plugin.h"
#include "../mregkey.hpp"
#include <string>
#include <cassert>
#include <strsafe.h>

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
static INT s_nMargin;
static INT s_nAlign;
static INT s_nVAlign;
static double s_eScale;
static std::string s_strCaption;

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

static LRESULT Plugin_Init(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
{
    s_nMargin = 3;
    s_nAlign = ALIGN_RIGHT;
    s_nVAlign = VALIGN_TOP;
    s_eScale = 0.2;
    s_strCaption = "&h:&m:&s.&f";

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

    DWORD dwValue;
    TCHAR szText[64];

    if (!hkeyApp.QueryDword(TEXT("Scale"), (DWORD&)dwValue))
    {
        s_eScale = dwValue / 1000.0;
    }

    if (!hkeyApp.QuerySz(TEXT("Caption"), szText, ARRAYSIZE(szText)))
    {
        s_strCaption = ansi_from_wide(szText);
    }

    return TRUE;
}

static LRESULT Plugin_Uninit(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
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

    DWORD dwValue = DWORD(s_eScale * 1000);
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
    pi->dwFlags = PLUGIN_FLAG_PICREADER | PLUGIN_FLAG_PICWRITER;
    pi->bEnabled = TRUE;
    return TRUE;
}

// API Name: Plugin_Unload
// Purpose: The framework want to unload the plugin component.
// TODO: Unload the plugin component.
BOOL APIENTRY
Plugin_Unload(PLUGIN *pi, LPARAM lParam)
{
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

static LRESULT Plugin_PicRead(PLUGIN *pi, WPARAM wParam, LPARAM lParam)
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

// API Name: Plugin_Act
// Purpose: Act something on the plugin.
// TODO: Act something on the plugin.
LRESULT APIENTRY
Plugin_Act(PLUGIN *pi, UINT uAction, WPARAM wParam, LPARAM lParam)
{
    switch (uAction)
    {
    case PLUGIN_ACTION_INIT:
        return Plugin_Init(pi, wParam, lParam);
    case PLUGIN_ACTION_UNINIT:
        return Plugin_Uninit(pi, wParam, lParam);
    case PLUGIN_ACTION_STARTREC:
    case PLUGIN_ACTION_PAUSE:
    case PLUGIN_ACTION_ENDREC:
        break;
    case PLUGIN_ACTION_PICREAD:
        return Plugin_PicRead(pi, wParam, lParam);
    case PLUGIN_ACTION_PICWRITE:
        return Plugin_PicWrite(pi, wParam, lParam);
    case PLUGIN_ACTION_SETTINGS:
        break;
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
