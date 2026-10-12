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
    g_theme.background = g_theme.dark ? RGB(32, 32, 32) : RGB(243, 243, 243);
    g_theme.surface = g_theme.dark ? RGB(43, 43, 43) : RGB(255, 255, 255);
    g_theme.text = g_theme.dark ? RGB(245, 245, 245) : RGB(26, 26, 26);
    g_theme.muted = g_theme.dark ? RGB(166, 166, 166) : RGB(100, 100, 100);
    g_theme.accent = g_theme.dark ? RGB(96, 205, 255) : RGB(0, 103, 192);
    g_theme.border = g_theme.dark ? RGB(66, 66, 66) : RGB(220, 220, 220);
    g_theme.hover = g_theme.dark ? RGB(55, 55, 55) : RGB(246, 246, 246);
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

void theme_card(HDC dc, const RECT *rect) {
    HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
    HGDIOBJ previous_pen = SelectObject(dc, pen), previous_brush = SelectObject(dc, g_theme.surface_brush);
    RoundRect(dc, rect->left, rect->top, rect->right, rect->bottom, 12, 12);
    SelectObject(dc, previous_pen); SelectObject(dc, previous_brush); DeleteObject(pen);
}

static void round_control(HWND window) {
    wchar_t cls[64] = L""; GetClassNameW(window, cls, ARRAYSIZE(cls));
    if (lstrcmpiW(cls, L"EDIT") && lstrcmpiW(cls, L"COMBOBOX") && wcscmp(cls, WC_LISTVIEWW)) return;
    RECT r; GetWindowRect(window, &r);
    if (r.right <= r.left || r.bottom <= r.top) return;
    HRGN region = CreateRoundRectRgn(0, 0, r.right-r.left+1, r.bottom-r.top+1, 10, 10);
    if (region && !SetWindowRgn(window, region, TRUE)) DeleteObject(region);
}

static void paint_frame(HWND window, HDC dc) {
    RECT bounds; GetWindowRect(window, &bounds);
    RECT r = {0, 0, bounds.right-bounds.left, bounds.bottom-bounds.top};
    LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    if ((style & WS_BORDER) || GetPropW(window,L"WAIOFrame")) {
        RECT edges[] = {{0,0,r.right,2},{0,r.bottom-2,r.right,r.bottom},{0,0,2,r.bottom},{r.right-2,0,r.right,r.bottom}};
        for (unsigned i=0;i<ARRAYSIZE(edges);++i) FillRect(dc,&edges[i],g_theme.background_brush);
        HPEN pen=CreatePen(PS_SOLID,1,GetFocus()==window?g_theme.accent:g_theme.border);
        HGDIOBJ previous_pen=SelectObject(dc,pen),previous_brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
        RoundRect(dc,0,0,r.right,r.bottom,10,10);
        SelectObject(dc,previous_pen); SelectObject(dc,previous_brush); DeleteObject(pen);
    }
    for (int axis=0;axis<2;++axis) {
        if (!(style & (axis?WS_HSCROLL:WS_VSCROLL))) continue;
        SCROLLBARINFO bar={0}; bar.cbSize=sizeof(bar);
        if (!GetScrollBarInfo(window,axis?OBJID_HSCROLL:OBJID_VSCROLL,&bar) ||
            (bar.rgstate[0]&STATE_SYSTEM_INVISIBLE)) continue;
        RECT track=bar.rcScrollBar; OffsetRect(&track,-bounds.left,-bounds.top);
        FillRect(dc,&track,g_theme.surface_brush);
        if (bar.xyThumbBottom<=bar.xyThumbTop) continue;
        RECT thumb=track;
        if (axis) {
            thumb.left=track.left+bar.xyThumbTop; thumb.right=track.left+bar.xyThumbBottom;
            thumb.top+=5; thumb.bottom-=5;
        } else {
            thumb.top=track.top+bar.xyThumbTop; thumb.bottom=track.top+bar.xyThumbBottom;
            thumb.left+=5; thumb.right-=5;
        }
        HBRUSH brush=CreateSolidBrush(g_theme.muted);
        HGDIOBJ old_brush=SelectObject(dc,brush),old_pen=SelectObject(dc,GetStockObject(NULL_PEN));
        RoundRect(dc,thumb.left,thumb.top,thumb.right,thumb.bottom,6,6);
        SelectObject(dc,old_brush); SelectObject(dc,old_pen); DeleteObject(brush);
    }
}

static void paint_combo(HWND window,HDC dc) {
    RECT r; GetClientRect(window,&r);
    FillRect(dc,&r,g_theme.background_brush);
    theme_card(dc,&r);
    HGDIOBJ font=SelectObject(dc,(HFONT)SendMessageW(window,WM_GETFONT,0,0));
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,IsWindowEnabled(window)?g_theme.text:g_theme.muted);
    wchar_t text[256]=L"";
    LRESULT selected=SendMessageW(window,CB_GETCURSEL,0,0);
    if (selected>=0 && SendMessageW(window,CB_GETLBTEXTLEN,selected,0)<256)
        SendMessageW(window,CB_GETLBTEXT,selected,(LPARAM)text);
    RECT label=r; label.left+=10; label.right-=30;
    DrawTextW(dc,text,-1,&label,DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    HPEN pen=CreatePen(PS_SOLID,1,g_theme.text); HGDIOBJ previous=SelectObject(dc,pen);
    int x=r.right-16,y=r.bottom/2;
    MoveToEx(dc,x-4,y-2,NULL); LineTo(dc,x,y+2); LineTo(dc,x+4,y-2);
    SelectObject(dc,previous); DeleteObject(pen); SelectObject(dc,font);
}

static void paint_button(HWND window, HDC dc) {
    RECT r; GetClientRect(window, &r);
    wchar_t text[512]; GetWindowTextW(window, text, ARRAYSIZE(text));
    DWORD type = (DWORD)GetWindowLongPtrW(window, GWL_STYLE) & BS_TYPEMASK;
    BOOL check = type == BS_AUTOCHECKBOX || type == BS_CHECKBOX;
    BOOL group = type == BS_GROUPBOX;
    BOOL primary = GetPropW(window, L"WAIOPrimary") && IsWindowEnabled(window);
    LRESULT state = SendMessageW(window, BM_GETSTATE, 0, 0);
    BOOL hover = GetPropW(window, L"WAIOHover") != NULL;
    FillRect(dc, &r, GetPropW(window, L"WAIOCard") ? g_theme.surface_brush : g_theme.background_brush);
    HGDIOBJ font = SelectObject(dc, (HFONT)SendMessageW(window, WM_GETFONT, 0, 0));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, primary ? (g_theme.high_contrast ? GetSysColor(COLOR_HIGHLIGHTTEXT) : g_theme.dark ? RGB(0, 0, 0) : RGB(255, 255, 255)) : IsWindowEnabled(window) ? g_theme.text : g_theme.muted);
    HPEN pen = CreatePen(PS_SOLID, 1, (state & BST_FOCUS) ? g_theme.accent : g_theme.border);
    HBRUSH brush = CreateSolidBrush(primary ? g_theme.accent : (hover || (state & BST_PUSHED)) ? g_theme.hover : g_theme.surface);
    HGDIOBJ old_pen = SelectObject(dc, pen), old_brush = SelectObject(dc, brush);
    if (check) {
        int y = (r.bottom - 18) / 2;
        RoundRect(dc, 2, y, 21, y + 19, 5, 5);
        if (SendMessageW(window, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            HPEN tick = CreatePen(PS_SOLID, 2, g_theme.accent);
            SelectObject(dc, tick); MoveToEx(dc, 6, y + 9, NULL); LineTo(dc, 10, y + 13); LineTo(dc, 17, y + 5);
            SelectObject(dc, pen); DeleteObject(tick);
        }
        r.left += 30;
        if (GetWindowLongPtrW(window, GWL_STYLE) & BS_MULTILINE) {
            RECT measured = r; DrawTextW(dc, text, -1, &measured, DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
            r.top += max(0, (r.bottom-r.top-(measured.bottom-measured.top))/2);
            DrawTextW(dc, text, -1, &r, DT_WORDBREAK | DT_NOPREFIX);
        } else DrawTextW(dc, text, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
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
    if (!lstrcmpiW(cls,L"EDIT") || !wcscmp(cls,WC_LISTVIEWW)) {
        SetPropW(control,L"WAIOFrame",(HANDLE)1);
    }
    round_control(control);
    if (!wcscmp(cls, WC_LISTVIEWW)) {
        ListView_SetBkColor(control, g_theme.surface);
        ListView_SetTextBkColor(control, g_theme.surface);
        ListView_SetTextColor(control, g_theme.text);
        HWND header = ListView_GetHeader(control);
        if (header) SetWindowSubclass(header, themed_proc, 1, 0);
    } else if (!lstrcmpiW(cls, L"COMBOBOX")) {
        SetWindowTheme(control, L"", L"");
    } else if (!lstrcmpiW(cls, L"EDIT")) {
        SetWindowTheme(control, L"", L"");
        SendMessageW(control,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(8,8));
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
    case WM_SIZE: {
        LRESULT result = DefSubclassProc(window, message, wp, lp);
        round_control(window);
        return result;
    }
    case WM_SETTEXT:
    case WM_ENABLE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case BM_SETCHECK:
    case BM_SETSTATE:
    case BM_SETSTYLE:
    case CB_SETCURSEL:
    case CB_RESETCONTENT:
    case WM_UPDATEUISTATE: {
        LRESULT result = DefSubclassProc(window, message, wp, lp);
        /* Native buttons can paint directly during these messages. Repaint the
           whole custom surface, including the former label, after they return. */
        if (button || !lstrcmpiW(cls,L"COMBOBOX")) RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        return result;
    }
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        if (!(GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD)) theme_refresh();
        break;
    case WM_ERASEBKGND:
        if (button) return 1;
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
            HDC dc = GetDC(window); paint_combo(window,dc); ReleaseDC(window,dc);
            return result;
        }
        if (GetPropW(window,L"WAIOFrame")) {
            LRESULT result=DefSubclassProc(window,message,wp,lp);
            HDC dc=GetWindowDC(window); paint_frame(window,dc); ReleaseDC(window,dc);
            return result;
        }
        break;
    case WM_NCPAINT:
        if (!lstrcmpiW(cls, L"EDIT") || !wcscmp(cls, WC_LISTVIEWW) || !wcscmp(cls,L"WAIO.SetupCards")) {
            LRESULT result = DefSubclassProc(window, message, wp, lp);
            HDC dc = GetWindowDC(window); paint_frame(window,dc); ReleaseDC(window, dc);
            return result;
        }
        break;
    case WM_PRINT:
        if (button) { paint_button(window,(HDC)wp); return 0; }
        if (!lstrcmpiW(cls,L"COMBOBOX")) { paint_combo(window,(HDC)wp); return 0; }
        if (!wcscmp(cls,WC_HEADERW)) { paint_header(window,(HDC)wp); return 0; }
        if (!lstrcmpiW(cls,L"EDIT") || !wcscmp(cls,WC_LISTVIEWW) || !wcscmp(cls,L"WAIO.SetupCards")) {
            LRESULT result=DefSubclassProc(window,message,wp,lp);
            paint_frame(window,(HDC)wp);
            return result;
        }
        break;
    case WM_PRINTCLIENT:
        if (button) { paint_button(window, (HDC)wp); return 0; }
        if (!lstrcmpiW(cls,L"COMBOBOX")) { paint_combo(window,(HDC)wp); return 0; }
        if (GetPropW(window,L"WAIOFrame")) {
            LRESULT result=DefSubclassProc(window,message,wp,lp);
            paint_frame(window,(HDC)wp); return result;
        }
        break;
    case WM_NCDESTROY:
        RemovePropW(window, L"WAIOHover"); RemovePropW(window, L"WAIOCard"); RemovePropW(window, L"WAIOPrimary"); RemovePropW(window,L"WAIOFrame"); RemoveWindowSubclass(window, themed_proc, 1); break;
    }
    return DefSubclassProc(window, message, wp, lp);
}
