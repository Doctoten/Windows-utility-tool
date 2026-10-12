#ifndef WAIO_PACKAGES_H
#define WAIO_PACKAGES_H
#include "app.h"
#define PACKAGE_ITEMS_MAX 96
typedef struct {
    wchar_t display[120];
    wchar_t full_name[256];
    wchar_t family[160];
    BOOL selected;
} PackageItem;
/* Current user only; exact package-family allowlist, no framework/system packages. */
DWORD packages_enumerate(PackageItem *items, unsigned capacity, unsigned *count,
                         const wchar_t *only_family);
HRESULT packages_remove(const PackageItem *item);
BOOL packages_self_test(void);
const wchar_t *packages_display_name(const wchar_t *family);
#endif
