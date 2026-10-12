#ifndef WAIO_THEME_H
#define WAIO_THEME_H
#include "app.h"
typedef struct {
    COLORREF background, surface, text, muted, accent, border, hover;
    HBRUSH background_brush, surface_brush;
    BOOL dark, high_contrast;
    int mode; /* 0 Windows, 1 light, 2 dark */
} AppTheme;
extern AppTheme g_theme;
void theme_init(void);
void theme_destroy(void);
void theme_set(int mode, BOOL save);
void theme_refresh(void);
void theme_attach(HWND window);
void theme_control(HWND control);
void theme_card(HDC dc, const RECT *rect);
#endif
