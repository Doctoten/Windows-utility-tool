#include "theme.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <windowsx.h>

AppTheme g_theme;
static LRESULT CALLBACK themed_proc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

static void colors(void) {
    DWORD light = 1, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light, &size);
    HIGHCONTRASTW contrast = {sizeof(contrast), 0, NULL};
    SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
    g_theme.high_contrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
    g_theme.dark = g_theme.mode == 2 || (g_theme.mode == 0 && !light);
    g_theme.background = g_theme.dark ? RGB(17, 21, 29) : RGB(242, 245, 250);
    g_theme.surface = g_theme.dark ? RGB(28, 34, 45) : RGB(255, 255, 255);
    g_theme.text = g_theme.dark ? RGB(232, 238, 247) : RGB(27, 38, 59);
    g_theme.muted = g_theme.dark ? RGB(158, 172, 193) : RGB(86, 102, 126);
    g_theme.accent = g_theme.dark ? RGB(112, 179, 255) : RGB(30, 98, 206);
    g_theme.border = g_theme.dark ? RGB(60, 72, 91) : RGB(209, 220, 235);
    g_theme.hover = g_theme.dark ? RGB(42, 57, 79) : RGB(229, 239, 255);
    if (g_theme.high_contrast) {
        g_theme.background = g_theme.surface = GetSysColor(COLOR_WINDOW);
        g_theme.text = GetSysColor(COLOR_WINDOWTEXT);
        g_theme.muted = GetSysColor(COLOR_WINDOWTEXT);
        g_theme.accent = GetSysColor(COLOR_HIGHLIGHT);
        g_theme.border = GetSysColor(COLOR_WINDOWTEXT);
        g_theme.hover = GetSysColor(COLOR_BTNFACE);
    }
    if (g_theme.background_brush) DeleteObject(g_theme.background_brush);
    if (g_theme.surface_brush) DeleteObject(g_theme.surface_brush);
    g_theme.background_brush = CreateSolidBrush(g_theme.background);
    g_theme.surface_brush = CreateSolidBrush(g_theme.surface);
}

static void paint_button(HWND window, HDC dc) {
    RECT r; GetClientRect(window, &r);
    wchar_t text[512]; GetWindowTextW(window, text, ARRAYSIZE(text));
    DWORD type = (DWORD)GetWindowLongPtrW(window, GWL_STYLE) & BS_TYPEMASK;
    BOOL check = type == BS_AUTOCHECKBOX || type == BS_CHECKBOX;
    BOOL group = type == BS_GROUPBOX;
    LRESULT state = SendMessageW(window, BM_GETSTATE, 0, 0);
    BOOL hover = GetPropW(window, L"WAIOHover") != NULL;
    FillRect(dc, &r, GetPropW(window, L"WAIOCard") ? g_theme.surface_brush : g_theme.background_brush);
    HGDIOBJ font = SelectObject(dc, (HFONT)SendMessageW(window, WM_GETFONT, 0, 0));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, IsWindowEnabled(window) ? g_theme.text : g_theme.muted);
    HPEN pen = CreatePen(PS_SOLID, 1, (state & BST_FOCUS) ? g_theme.accent : g_theme.border);
    HBRUSH brush = CreateSolidBrush((hover || (state & BST_PUSHED)) ? g_theme.hover : g_theme.surface);
    HGDIOBJ old_pen = SelectObject(dc, pen), old_brush = SelectObject(dc, brush);
    if (check) {
        int y = (r.bottom - 18) / 2;
        RoundRect(dc, 2, y, 21, y + 19, 5, 5);
        if (SendMessageW(window, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            HPEN tick = CreatePen(PS_SOLID, 2, g_theme.accent);
            SelectObject(dc, tick); MoveToEx(dc, 6, y + 9, NULL); LineTo(dc, 10, y + 13); LineTo(dc, 17, y + 5);
            SelectObject(dc, pen); DeleteObject(tick);
        }
        r.left += 30; DrawTextW(dc, text, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    } else if (group) {
        SelectObject(dc, GetStockObject(HOLLOW_BRUSH)); RoundRect(dc, 0, 10, r.right-1, r.bottom-1, 12, 12);
        r.left += 14; r.top = 0; r.bottom = 24;
        SetBkMode(dc, OPAQUE); SetBkColor(dc, g_theme.background);
        DrawTextW(dc, text, -1, &r, DT_SINGLELINE | DT_NOPREFIX);
    } else {
        RoundRect(dc, 0, 0, r.right-1, r.bottom-1, 12, 12);
        InflateRect(&r, -12, -4);
        DrawTextW(dc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    if ((state & BST_FOCUS) && !(SendMessageW(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
        RECT focus; GetClientRect(window, &focus); InflateRect(&focus, -4, -4); DrawFocusRect(dc, &focus);
    }
    SelectObject(dc, old_pen); SelectObject(dc, old_brush); SelectObject(dc, font);
    DeleteObject(pen); DeleteObject(brush);
}

static void paint_header(HWND window, HDC dc) {
    RECT client; GetClientRect(window, &client); FillRect(dc, &client, g_theme.surface_brush);
    HGDIOBJ font = SelectObject(dc, g_font); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, g_theme.muted);
    for (int i = 0; i < Header_GetItemCount(window); ++i) {
        wchar_t text[160] = L""; HDITEMW item = {0}; item.mask = HDI_TEXT; item.pszText = text; item.cchTextMax = ARRAYSIZE(text);
        RECT rect; Header_GetItemRect(window, i, &rect); Header_GetItem(window, i, &item);
        rect.left += 10; DrawTextW(dc, text, -1, &rect, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    SelectObject(dc, font);
}

void theme_control(HWND control) {
    wchar_t cls[64]; GetClassNameW(control, cls, ARRAYSIZE(cls));
    SetWindowSubclass(control, themed_proc, 1, 0);
    if (!wcscmp(cls, WC_LISTVIEWW)) {
        ListView_SetBkColor(control, g_theme.surface);
        ListView_SetTextBkColor(control, g_theme.surface);
        ListView_SetTextColor(control, g_theme.text);
        HWND header = ListView_GetHeader(control);
        if (header) SetWindowSubclass(header, themed_proc, 1, 0);
    } else if (!lstrcmpiW(cls, L"COMBOBOX")) {
        SetWindowTheme(control, L"", L"");
    } else if (!wcscmp(cls, PROGRESS_CLASSW)) {
        SendMessageW(control, PBM_SETBKCOLOR, 0, g_theme.surface);
        SendMessageW(control, PBM_SETBARCOLOR, 0, g_theme.accent);
    }
    InvalidateRect(control, NULL, TRUE);
}
static BOOL CALLBACK apply_child(HWND window, LPARAM value) { (void)value; theme_control(window); return TRUE; }
void theme_attach(HWND window) {
    SetWindowSubclass(window, themed_proc, 1, 0);
    if (app_windows_build() >= 22000) {
        BOOL dark = g_theme.dark && !g_theme.high_contrast;
        DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
        DWORD rounded = 2; DwmSetWindowAttribute(window, 33, &rounded, sizeof(rounded));
    }
    EnumChildWindows(window, apply_child, 0);
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}
static BOOL CALLBACK apply_window(HWND window, LPARAM value) {
    (void)value;
    wchar_t cls[64]; GetClassNameW(window, cls, ARRAYSIZE(cls));
    if (!wcsncmp(cls, L"WAIO", 4)) theme_attach(window);
    return TRUE;
}
void theme_refresh(void) { colors(); EnumThreadWindows(GetCurrentThreadId(), apply_window, 0); }
void theme_init(void) {
    DWORD value = 0, size = sizeof(value);
    if (!app_is_smoke_test() && RegGetValueW(HKEY_CURRENT_USER, L"Software\\Doctoten\\WinAIOOptimize",
        L"Theme", RRF_RT_REG_DWORD, NULL, &value, &size) == ERROR_SUCCESS && value <= 2) g_theme.mode = (int)value;
    colors();
}
void theme_set(int mode, BOOL save) {
    if (mode < 0 || mode > 2) return;
    g_theme.mode = mode; theme_refresh();
    if (save && !app_is_smoke_test()) {
        DWORD value = (DWORD)mode;
        HKEY key;
        LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Doctoten\\WinAIOOptimize", 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
        if (!error) { error = RegSetValueExW(key, L"Theme", 0, REG_DWORD, (BYTE *)&value, sizeof(value)); RegCloseKey(key); }
        if (error) app_log(tr(L"Không lưu được giao diện (mã %lu).", L"Could not save theme (code %lu)."), (DWORD)error);
    }
}
void theme_destroy(void) {
    DeleteObject(g_theme.background_brush); DeleteObject(g_theme.surface_brush);
    g_theme.background_brush = g_theme.surface_brush = NULL;
}

static LRESULT CALLBACK themed_proc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;
    wchar_t cls[64]; GetClassNameW(window, cls, ARRAYSIZE(cls));
    BOOL button = !wcscmp(cls, L"Button");
    switch (message) {
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        if (!(GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD)) theme_refresh();
        break;
    case WM_ERASEBKGND:
        if (!wcsncmp(cls, L"WAIO", 4)) { RECT r; GetClientRect(window, &r); FillRect((HDC)wp, &r, g_theme.background_brush); return 1; }
        break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC dc = (HDC)wp;
        wchar_t child[64]; GetClassNameW((HWND)lp, child, ARRAYSIZE(child));
        BOOL surface = GetPropW((HWND)lp, L"WAIOCard") || !wcscmp(child, L"Edit") || !wcscmp(child, L"ComboBox") || message == WM_CTLCOLORLISTBOX;
        SetTextColor(dc, g_theme.text); SetBkColor(dc, surface ? g_theme.surface : g_theme.background);
        return (LRESULT)(surface ? g_theme.surface_brush : g_theme.background_brush);
    }
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT *item = (MEASUREITEMSTRUCT *)lp;
        if (item->CtlType == ODT_COMBOBOX) { item->itemHeight = 26; return TRUE; }
        break;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *item = (DRAWITEMSTRUCT *)lp;
        if (item->CtlType == ODT_COMBOBOX) {
            BOOL selected = (item->itemState & ODS_SELECTED) != 0;
            HBRUSH brush = CreateSolidBrush(selected ? g_theme.hover : g_theme.surface);
            FillRect(item->hDC, &item->rcItem, brush); DeleteObject(brush);
            SetTextColor(item->hDC, (item->itemState & ODS_DISABLED) ? g_theme.muted : g_theme.text);
            SetBkMode(item->hDC, TRANSPARENT);
            wchar_t text[256] = L"";
            if (item->itemID != (UINT)-1 && SendMessageW(item->hwndItem, CB_GETLBTEXTLEN, item->itemID, 0) < 256)
                SendMessageW(item->hwndItem, CB_GETLBTEXT, item->itemID, (LPARAM)text);
            RECT r = item->rcItem; r.left += 8; DrawTextW(item->hDC, text, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
            return TRUE;
        }
        break;
    }
    case WM_MOUSEMOVE:
        if (button && !GetPropW(window, L"WAIOHover")) {
            SetPropW(window, L"WAIOHover", (HANDLE)1); TRACKMOUSEEVENT track = {sizeof(track), TME_LEAVE, window, 0};
            TrackMouseEvent(&track); InvalidateRect(window, NULL, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        if (button) { RemovePropW(window, L"WAIOHover"); InvalidateRect(window, NULL, FALSE); }
        break;
    case WM_PAINT:
        if (button || !wcscmp(cls, WC_HEADERW)) {
            PAINTSTRUCT paint; HDC dc = BeginPaint(window, &paint);
            if (button) paint_button(window, dc); else paint_header(window, dc);
            EndPaint(window, &paint); return 0;
        }
        if (!lstrcmpiW(cls, L"COMBOBOX")) {
            LRESULT result = DefSubclassProc(window, message, wp, lp);
            HDC dc = GetDC(window); RECT r; GetClientRect(window, &r);
            RECT arrow = {r.right-23, 1, r.right-1, r.bottom-1};
            FillRect(dc, &arrow, g_theme.surface_brush);
            HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
            HGDIOBJ old_pen = SelectObject(dc, pen), old_brush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
            Rectangle(dc, 0, 0, r.right, r.bottom);
            HPEN accent = CreatePen(PS_SOLID, 2, g_theme.text); SelectObject(dc, accent);
            int x = r.right-13, y = r.bottom/2;
            MoveToEx(dc, x-4, y-2, NULL); LineTo(dc, x, y+2); LineTo(dc, x+4, y-2);
            SelectObject(dc, old_pen); SelectObject(dc, old_brush); DeleteObject(pen); DeleteObject(accent); ReleaseDC(window, dc);
            return result;
        }
        break;
    case WM_NCPAINT:
        if (!lstrcmpiW(cls, L"EDIT") || !wcscmp(cls, WC_LISTVIEWW)) {
            LRESULT result = DefSubclassProc(window, message, wp, lp);
            HDC dc = GetWindowDC(window); RECT r; GetWindowRect(window, &r); OffsetRect(&r, -r.left, -r.top);
            HBRUSH border = CreateSolidBrush(g_theme.border); FrameRect(dc, &r, border); DeleteObject(border); ReleaseDC(window, dc);
            return result;
        }
        break;
    case WM_PRINTCLIENT:
        if (button) { paint_button(window, (HDC)wp); return 0; }
        break;
    case WM_NCDESTROY:
        RemovePropW(window, L"WAIOHover"); RemovePropW(window, L"WAIOCard"); RemoveWindowSubclass(window, themed_proc, 1); break;
    }
    return DefSubclassProc(window, message, wp, lp);
}
