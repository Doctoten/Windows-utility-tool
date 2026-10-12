#include "app.h"
#include "theme.h"
#include <wbemidl.h>
#include <oleauto.h>
#include <stdio.h>
#include <dxgi.h>

typedef struct { const wchar_t *key, *vi, *en; unsigned units; } Field;
typedef struct { const wchar_t *query, *vi, *en; const Field *fields; unsigned count; } Category;
#define F(key,vi,en) {L##key,L##vi,L##en,0}
#define B(key,vi,en) {L##key,L##vi,L##en,1}
#define K(key,vi,en) {L##key,L##vi,L##en,2}
static const Field computer[] = {F("Name","Tên máy","Computer name"), F("Manufacturer","Hãng sản xuất","Manufacturer"),
    F("Model","Model","Model"), F("SystemType","Kiến trúc hệ thống","System architecture"),
    F("UserName","Tài khoản đăng nhập","Signed-in account"), F("Domain","Miền / nhóm làm việc","Domain / workgroup"),
    B("TotalPhysicalMemory","RAM vật lý","Physical RAM"), F("HypervisorPresent","Có hypervisor","Hypervisor present")};
static const Field os[] = {F("Caption","Hệ điều hành","Operating system"), F("Version","Phiên bản","Version"),
    F("BuildNumber","Build","Build"), F("OSArchitecture","Kiến trúc","Architecture"), F("InstallDate","Ngày cài (DMTF)","Installed (DMTF)"),
    F("LastBootUpTime","Khởi động gần nhất (DMTF)","Last boot (DMTF)"), F("WindowsDirectory","Thư mục Windows","Windows directory"),
    K("TotalVisibleMemorySize","RAM Windows sử dụng được","Usable RAM"), K("FreePhysicalMemory","RAM còn trống","Available RAM")};
static const Field cpu[] = {F("Name","Bộ xử lý","Processor"), F("Manufacturer","Hãng sản xuất","Manufacturer"),
    F("NumberOfCores","Số nhân","Cores"), F("NumberOfLogicalProcessors","Luồng xử lý","Logical processors"),
    F("MaxClockSpeed","Xung tối đa (MHz)","Maximum clock (MHz)"), F("SocketDesignation","Socket","Socket"),
    F("VirtualizationFirmwareEnabled","Ảo hóa trong firmware","Firmware virtualization enabled")};
static const Field ram[] = {F("DeviceLocator","Khe RAM","Memory slot"), F("Manufacturer","Hãng sản xuất","Manufacturer"),
    F("PartNumber","Mã linh kiện","Part number"), B("Capacity","Dung lượng","Capacity"),
    F("Speed","Tốc độ công bố (MHz)","Rated speed (MHz)"), F("ConfiguredClockSpeed","Tốc độ cấu hình (MHz)","Configured speed (MHz)"),
    F("SMBIOSMemoryType","Loại RAM (mã SMBIOS)","Memory type (SMBIOS code)")};
static const Field gpu[] = {F("Name","GPU","GPU"), F("VideoProcessor","Bộ xử lý đồ họa","Video processor"),
    F("DriverVersion","Phiên bản driver","Driver version"), F("VideoModeDescription","Chế độ hiển thị","Display mode"),
    F("CurrentRefreshRate","Tần số quét (Hz)","Refresh rate (Hz)"), F("PNPDeviceID","Định danh thiết bị","Device identity")};
static const Field disk[] = {F("Model","Model ổ đĩa","Drive model"), F("SerialNumber","Số sê-ri","Serial number"),
    B("Size","Dung lượng","Capacity"), F("InterfaceType","Giao tiếp","Interface"), F("MediaType","Loại thiết bị","Media type"),
    F("Partitions","Số phân vùng","Partitions"), F("Status","Trạng thái Windows báo cáo","Windows-reported status")};
static const Field volume[] = {F("DeviceID","Ổ đĩa","Volume"), F("VolumeName","Nhãn","Label"), F("FileSystem","Hệ thống file","File system"),
    B("Size","Dung lượng","Capacity"), B("FreeSpace","Còn trống","Free space")};
static const Field board[] = {F("Manufacturer","Hãng sản xuất","Manufacturer"), F("Product","Bo mạch chủ","Motherboard"),
    F("Version","Phiên bản","Version"), F("SerialNumber","Số sê-ri","Serial number")};
static const Field bios[] = {F("Manufacturer","Hãng firmware","Firmware vendor"), F("SMBIOSBIOSVersion","Phiên bản BIOS","BIOS version"),
    F("ReleaseDate","Ngày phát hành (DMTF)","Release date (DMTF)"), F("SerialNumber","Số sê-ri máy","System serial number")};
static const Field network[] = {F("Description","Card mạng","Adapter"), F("MACAddress","Địa chỉ MAC","MAC address"),
    F("IPAddress","Địa chỉ IP","IP addresses"), F("IPSubnet","Mạng con","Subnets"), F("DefaultIPGateway","Gateway","Gateway"),
    F("DNSServerSearchOrder","Máy chủ DNS","DNS servers"), F("DHCPEnabled","DHCP","DHCP enabled")};
static const Field battery[] = {F("Name","Pin","Battery"), F("EstimatedChargeRemaining","Dung lượng còn lại (%)","Charge remaining (%)"),
    F("EstimatedRunTime","Thời gian còn lại (phút; 71582788 = nguồn AC)","Runtime (minutes; 71582788 = AC power)"),
    F("BatteryStatus","Trạng thái (mã Windows)","Status (Windows code)")};
static const Field directx[] = {F("Description","GPU (DirectX)","GPU (DirectX)"), B("DedicatedVideoMemory","VRAM riêng","Dedicated video memory"),
    B("SharedSystemMemory","RAM có thể chia sẻ","Shared system memory"), F("VendorId","Mã hãng (PCI)","Vendor ID (PCI)")};
static const Field firmware[] = {F("FirmwareType","Chế độ firmware","Firmware mode"), F("SecureBoot","Secure Boot","Secure Boot")};
#define C(query,vi,en,fields) {L##query,L##vi,L##en,fields,ARRAYSIZE(fields)}
static const Category categories[] = {
    C("SELECT * FROM Win32_ComputerSystem","Tổng quan","Overview",computer),
    C("SELECT * FROM Win32_OperatingSystem","Windows","Windows",os),
    C("SELECT * FROM Win32_Processor","CPU","CPU",cpu),
    C("SELECT * FROM Win32_PhysicalMemory","RAM","RAM",ram),
    C("SELECT * FROM Win32_VideoController","Đồ họa","Graphics",gpu),
    C("SELECT * FROM Win32_DiskDrive","Ổ đĩa vật lý","Physical disks",disk),
    C("SELECT * FROM Win32_LogicalDisk WHERE DriveType=3","Phân vùng","Volumes",volume),
    C("SELECT * FROM Win32_BaseBoard","Bo mạch chủ","Motherboard",board),
    C("SELECT * FROM Win32_BIOS","BIOS / firmware","BIOS / firmware",bios),
    C("SELECT * FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled=TRUE","Mạng đang cấu hình IP","IP-enabled networks",network),
    C("SELECT * FROM Win32_Battery","Pin","Battery",battery),
    C("","Bộ nhớ đồ họa","Graphics memory",directx),
    C("","Khởi động và bảo mật","Boot and security",firmware)
};
#undef F
#undef B
#undef K
#undef C

#define INFO_MAX 2048
typedef struct { unsigned category, instance; int field; wchar_t value[1024]; } InfoRow;
typedef struct { InfoRow rows[INFO_MAX]; unsigned count, errors; BOOL truncated; } Inventory;
typedef struct { HWND window, list, group, detail; Inventory *inventory, *pending; } InfoWindow;
enum { INFO_LIST=4201, INFO_GROUP, INFO_REFRESH, INFO_COPY, INFO_DETAIL, INFO_STATUS, INFO_HEADING, INFO_NOTE };
static HWND info_window;

static void variant_text(VARIANT *value, unsigned units, wchar_t *text, size_t capacity) {
    text[0] = L'\0';
    if (V_VT(value) == VT_NULL || V_VT(value) == VT_EMPTY) return;
    if (V_VT(value) == (VT_ARRAY | VT_BSTR)) {
        LONG low, high;
        if (FAILED(SafeArrayGetLBound(V_ARRAY(value), 1, &low)) || FAILED(SafeArrayGetUBound(V_ARRAY(value), 1, &high))) return;
        for (LONG i = low; i <= high && i - low < 64; ++i) {
            BSTR part = NULL;
            if (SUCCEEDED(SafeArrayGetElement(V_ARRAY(value), &i, &part)) && part) {
                if (text[0]) StringCchCatW(text, capacity, L", ");
                StringCchCatW(text, capacity, part); SysFreeString(part);
            }
        }
        return;
    }
    if (V_VT(value) == VT_BOOL) { StringCchCopyW(text, capacity, V_BOOL(value) ? L"@yes" : L"@no"); return; }
    VARIANT converted; VariantInit(&converted);
    if (SUCCEEDED(VariantChangeType(&converted, value, 0, VT_BSTR)) && V_BSTR(&converted)) {
        if (units) {
            ULONGLONG bytes = _wcstoui64(V_BSTR(&converted), NULL, 10);
            double gib = (double)bytes / (units == 2 ? 1048576.0 : 1073741824.0);
            StringCchPrintfW(text, capacity, L"%.2f GiB", gib);
        } else StringCchCopyW(text, capacity, V_BSTR(&converted));
    }
    VariantClear(&converted);
}
static const wchar_t *display_value(const InfoRow *row) {
    if (!wcscmp(row->value, L"@yes")) return tr(L"Có", L"Yes");
    if (!wcscmp(row->value, L"@no")) return tr(L"Không", L"No");
    if (!row->value[0]) return tr(L"Không có thiết bị hoặc Windows không cung cấp", L"No device or not reported by Windows");
    return row->value;
}
static void query_error(Inventory *inventory, unsigned category, HRESULT error) {
    ++inventory->errors;
    if (inventory->count >= INFO_MAX) { inventory->truncated = TRUE; return; }
    InfoRow *row = &inventory->rows[inventory->count++]; row->category = category; row->field = -1;
    StringCchPrintfW(row->value, ARRAYSIZE(row->value), L"WMI: 0x%08lX", (DWORD)error);
}
static void collect_native(Inventory *inventory) {
    const unsigned graphics = 11, boot = 12;
    IDXGIFactory1 *factory = NULL;
    HRESULT result = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory);
    if (FAILED(result)) query_error(inventory, graphics, result);
    else {
        for (unsigned i = 0; i < 32; ++i) {
            IDXGIAdapter1 *adapter = NULL;
            result = IDXGIFactory1_EnumAdapters1(factory, i, &adapter);
            if (result == DXGI_ERROR_NOT_FOUND) break;
            if (FAILED(result)) { query_error(inventory, graphics, result); break; }
            DXGI_ADAPTER_DESC1 description;
            result = IDXGIAdapter1_GetDesc1(adapter, &description); IDXGIAdapter1_Release(adapter);
            if (FAILED(result)) { query_error(inventory, graphics, result); continue; }
            if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            for (unsigned f = 0; f < 4; ++f) {
                if (inventory->count >= INFO_MAX) { inventory->truncated = TRUE; break; }
                InfoRow *row = &inventory->rows[inventory->count++]; row->category = graphics; row->instance = i+1; row->field = (int)f;
                if (f == 0) StringCchCopyW(row->value, ARRAYSIZE(row->value), description.Description);
                else if (f == 3) StringCchPrintfW(row->value, ARRAYSIZE(row->value), L"0x%04X", description.VendorId);
                else StringCchPrintfW(row->value, ARRAYSIZE(row->value), L"%.2f GiB",
                    (double)(f == 1 ? description.DedicatedVideoMemory : description.SharedSystemMemory) / 1073741824.0);
            }
        }
        IDXGIFactory1_Release(factory);
    }
    for (int f = 0; f < 2 && inventory->count < INFO_MAX; ++f) {
        InfoRow *row = &inventory->rows[inventory->count++]; row->category = boot; row->instance = 1; row->field = f;
        if (f == 0) {
            FIRMWARE_TYPE type;
            if (GetFirmwareType(&type) && (type == FirmwareTypeUefi || type == FirmwareTypeBios))
                StringCchCopyW(row->value, ARRAYSIZE(row->value), type == FirmwareTypeUefi ? L"UEFI" : L"BIOS");
        } else {
            DWORD enabled = 0, size = sizeof(enabled);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State",
                L"UEFISecureBootEnabled", RRF_RT_REG_DWORD, NULL, &enabled, &size) == ERROR_SUCCESS)
                StringCchCopyW(row->value, ARRAYSIZE(row->value), enabled ? L"@yes" : L"@no");
        }
    }
}
static void collect_inventory(Inventory *inventory) {
    collect_native(inventory);
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) { query_error(inventory, 0, hr); return; }
    IWbemLocator *locator = NULL; IWbemServices *services = NULL;
    hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator);
    BSTR space = SysAllocString(L"ROOT\\CIMV2");
    if (SUCCEEDED(hr)) hr = space ? IWbemLocator_ConnectServer(locator, space, NULL, NULL, NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &services) : E_OUTOFMEMORY;
    SysFreeString(space);
    if (SUCCEEDED(hr)) hr = CoSetProxyBlanket((IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr)) query_error(inventory, 0, hr);
    ULONGLONG deadline = GetTickCount64() + 25000;
    for (unsigned c = 0; SUCCEEDED(hr) && c < ARRAYSIZE(categories); ++c) {
        if (!categories[c].query[0]) continue;
        if (GetTickCount64() >= deadline) {
            for (unsigned missing = c; missing < ARRAYSIZE(categories); ++missing)
                if (categories[missing].query[0]) query_error(inventory, missing, HRESULT_FROM_WIN32(ERROR_TIMEOUT));
            break;
        }
        BSTR language = SysAllocString(L"WQL"), query = SysAllocString(categories[c].query);
        IEnumWbemClassObject *objects = NULL;
        HRESULT result = language && query ? IWbemServices_ExecQuery(services, language, query,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &objects) : E_OUTOFMEMORY;
        SysFreeString(language); SysFreeString(query);
        unsigned instance = 0;
        while (SUCCEEDED(result)) {
            IWbemClassObject *object = NULL; ULONG returned = 0;
            result = IEnumWbemClassObject_Next(objects, 2000, 1, &object, &returned);
            if (!returned || !object) {
                if (result == WBEM_S_TIMEDOUT) result = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                break;
            }
            ++instance;
            for (unsigned f = 0; f < categories[c].count; ++f) {
                if (inventory->count >= INFO_MAX) { inventory->truncated = TRUE; break; }
                InfoRow *row = &inventory->rows[inventory->count++];
                row->category = c; row->instance = instance; row->field = (int)f;
                VARIANT value; VariantInit(&value);
                if (SUCCEEDED(IWbemClassObject_Get(object, categories[c].fields[f].key, 0, &value, NULL, NULL)))
                    variant_text(&value, categories[c].fields[f].units, row->value, ARRAYSIZE(row->value));
                VariantClear(&value);
            }
            IWbemClassObject_Release(object);
            if (inventory->truncated) break;
            if (GetTickCount64() >= deadline) { result = HRESULT_FROM_WIN32(ERROR_TIMEOUT); break; }
        }
        if (FAILED(result)) query_error(inventory, c, result);
        else if (!instance && inventory->count < INFO_MAX) {
            InfoRow *row = &inventory->rows[inventory->count++]; row->category = c; row->field = -1;
        }
        if (objects) IEnumWbemClassObject_Release(objects);
        if (inventory->truncated) break;
    }
    if (services) IWbemServices_Release(services);
    if (locator) IWbemLocator_Release(locator);
    CoUninitialize();
}

void system_summary(wchar_t *text, size_t capacity) {
    wchar_t cpu_name[256] = L"CPU", name[128] = L"PC"; DWORD bytes = sizeof(cpu_name), length = ARRAYSIZE(name);
    RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"ProcessorNameString", RRF_RT_REG_SZ, NULL, cpu_name, &bytes);
    GetComputerNameW(name, &length);
    MEMORYSTATUSEX memory = {0}; memory.dwLength = sizeof(memory); GlobalMemoryStatusEx(&memory);
    StringCchPrintfW(text, capacity, tr(L"%s  •  Windows %s / build %lu\r\n%s\r\nRAM %.1f GiB  •  Khả dụng %.1f GiB",
        L"%s  •  Windows %s / build %lu\r\n%s\r\nRAM %.1f GiB  •  Available %.1f GiB"),
        name, app_windows_build() >= 22000 ? L"11" : L"10", app_windows_build(), cpu_name,
        (double)memory.ullTotalPhys / 1073741824.0, (double)memory.ullAvailPhys / 1073741824.0);
}
static void info_layout(InfoWindow *state) {
    RECT r; GetClientRect(state->window, &r); int width = r.right, height = r.bottom;
    MoveWindow(GetDlgItem(state->window, INFO_HEADING), 24, 18, width-48, 28, TRUE);
    MoveWindow(GetDlgItem(state->window, INFO_NOTE), 24, 50, width-48, 44, TRUE);
    MoveWindow(state->group, 24, 104, 285, 400, TRUE);
    MoveWindow(GetDlgItem(state->window, INFO_REFRESH), width-320, 102, 130, 34, TRUE);
    MoveWindow(GetDlgItem(state->window, INFO_COPY), width-178, 102, 154, 34, TRUE);
    MoveWindow(state->list, 24, 150, width-48, height-296, TRUE);
    MoveWindow(state->detail, 24, height-134, width-48, 76, TRUE);
    MoveWindow(GetDlgItem(state->window, INFO_STATUS), 24, height-46, width-48, 32, TRUE);
    ListView_SetColumnWidth(state->list, 0, 210); ListView_SetColumnWidth(state->list, 1, 265);
    ListView_SetColumnWidth(state->list, 2, width > 990 ? width-540 : 450);
}
static void render_inventory(InfoWindow *state) {
    int selected = ListView_GetNextItem(state->list, -1, LVNI_SELECTED);
    LPARAM identity = -1;
    if (selected >= 0) { LVITEMW item = {0}; item.mask = LVIF_PARAM; item.iItem = selected; ListView_GetItem(state->list, &item); identity = item.lParam; }
    SendMessageW(state->list, WM_SETREDRAW, FALSE, 0); ListView_DeleteAllItems(state->list);
    SetWindowTextW(state->detail, L"");
    int filter = (int)SendMessageW(state->group, CB_GETCURSEL, 0, 0) - 1;
    Inventory *inventory = state->inventory;
    for (unsigned i = 0; inventory && i < inventory->count; ++i) {
        InfoRow *row = &inventory->rows[i];
        if (filter >= 0 && (unsigned)filter != row->category) continue;
        const Category *category = &categories[row->category]; wchar_t label[180];
        if (row->instance) StringCchPrintfW(label, ARRAYSIZE(label), L"%s / %u", tr(category->vi, category->en), row->instance);
        else StringCchCopyW(label, ARRAYSIZE(label), tr(category->vi, category->en));
        LVITEMW item = {0}; item.mask = LVIF_TEXT | LVIF_PARAM; item.pszText = label; item.iItem = ListView_GetItemCount(state->list); item.lParam = i;
        int index = ListView_InsertItem(state->list, &item);
        const wchar_t *field = row->field < 0 ? tr(L"Trạng thái", L"Status") : tr(category->fields[row->field].vi, category->fields[row->field].en);
        ListView_SetItemText(state->list, index, 1, (wchar_t *)field);
        ListView_SetItemText(state->list, index, 2, (wchar_t *)display_value(row));
        if ((LPARAM)i == identity) ListView_SetItemState(state->list, index, LVIS_SELECTED, LVIS_SELECTED);
    }
    SendMessageW(state->list, WM_SETREDRAW, TRUE, 0); InvalidateRect(state->list, NULL, TRUE);
    wchar_t status[256];
    if (!inventory) StringCchCopyW(status, ARRAYSIZE(status), tr(L"Chọn Làm mới để đọc thông tin máy.", L"Choose Refresh to read this computer's information."));
    else StringCchPrintfW(status, ARRAYSIZE(status), tr(L"%u mục  •  %u truy vấn lỗi  •  %s", L"%u entries  •  %u query errors  •  %s"),
        inventory->count, inventory->errors, inventory->truncated ? tr(L"Đã đạt giới hạn hiển thị", L"Display limit reached") : tr(L"Dữ liệu tại lần làm mới gần nhất", L"Snapshot from last refresh"));
    SetDlgItemTextW(state->window, INFO_STATUS, status);
}
static void translate_info(InfoWindow *state) {
    SetWindowTextW(state->window, tr(L"Thông tin máy — Win AIO Optimize", L"System information — Win AIO Optimize"));
    SetDlgItemTextW(state->window, INFO_HEADING, tr(L"Thông tin phần cứng & Windows", L"Hardware & Windows information"));
    SetDlgItemTextW(state->window, INFO_NOTE, tr(L"Đọc trực tiếp từ Windows. Chọn nhóm để lọc, chọn dòng để xem đầy đủ. Không có thiết bị hoặc dữ liệu sẽ được ghi rõ.",
        L"Read directly from Windows. Filter by category; select a row for its full value. Missing devices or data are identified."));
    SetDlgItemTextW(state->window, INFO_REFRESH, tr(L"Làm mới", L"Refresh"));
    SetDlgItemTextW(state->window, INFO_COPY, tr(L"Sao chép dòng", L"Copy row"));
    LRESULT selected = SendMessageW(state->group, CB_GETCURSEL, 0, 0);
    SendMessageW(state->group, CB_RESETCONTENT, 0, 0);
    SendMessageW(state->group, CB_ADDSTRING, 0, (LPARAM)tr(L"Tất cả thành phần", L"All components"));
    for (unsigned i = 0; i < ARRAYSIZE(categories); ++i) SendMessageW(state->group, CB_ADDSTRING, 0, (LPARAM)tr(categories[i].vi, categories[i].en));
    SendMessageW(state->group, CB_SETCURSEL, selected < 0 ? 0 : selected, 0);
    const wchar_t *columns[] = {tr(L"Thành phần", L"Component"), tr(L"Thông tin", L"Property"), tr(L"Giá trị", L"Value")};
    for (int i = 0; i < 3; ++i) { LVCOLUMNW column = {0}; column.mask = LVCF_TEXT; column.pszText = (wchar_t *)columns[i]; ListView_SetColumn(state->list, i, &column); }
    render_inventory(state);
}
static void inventory_worker(void *argument) { InfoWindow *state = argument; collect_inventory(state->pending); }
static void refresh_info(InfoWindow *state) {
    if (app_busy(NULL) || app_is_smoke_test()) return;
    state->pending = calloc(1, sizeof(*state->pending));
    if (!state->pending) { app_error(state->window, tr(L"Đọc thông tin máy", L"Read system information"), ERROR_OUTOFMEMORY); return; }
    if (!app_start_job(state->window, inventory_worker, state)) { free(state->pending); state->pending = NULL; return; }
    SetDlgItemTextW(state->window, INFO_STATUS, tr(L"Đang đọc phần cứng và Windows…", L"Reading hardware and Windows…"));
}
static void copy_row(InfoWindow *state) {
    int index = ListView_GetNextItem(state->list, -1, LVNI_SELECTED);
    if (index < 0) return;
    wchar_t field[256], value[1024], text[1400];
    ListView_GetItemText(state->list, index, 1, field, ARRAYSIZE(field));
    ListView_GetItemText(state->list, index, 2, value, ARRAYSIZE(value));
    StringCchPrintfW(text, ARRAYSIZE(text), L"%s: %s", field, value);
    size_t bytes = (wcslen(text)+1) * sizeof(wchar_t);
    HGLOBAL block = GlobalAlloc(GMEM_MOVEABLE, bytes); if (!block) return;
    void *buffer = GlobalLock(block); if (!buffer) { GlobalFree(block); return; }
    memcpy(buffer, text, bytes); GlobalUnlock(block);
    if (!OpenClipboard(state->window)) { GlobalFree(block); return; }
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, block)) GlobalFree(block);
    CloseClipboard();
}
static LRESULT CALLBACK info_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    InfoWindow *state = (InfoWindow *)GetWindowLongPtrW(window, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE: {
        state = calloc(1, sizeof(*state)); if (!state) return -1;
        state->window = window; SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)state); theme_attach(window);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, INFO_HEADING);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, INFO_NOTE);
        state->group = app_control(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 0, 0, 0, 0, INFO_GROUP);
        app_control(window, L"BUTTON", L"", WS_TABSTOP, 0, 0, 0, 0, INFO_REFRESH);
        app_control(window, L"BUTTON", L"", WS_TABSTOP, 0, 0, 0, 0, INFO_COPY);
        state->list = app_control(window, WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, 0, 0, 0, INFO_LIST);
        ListView_SetExtendedListViewStyle(state->list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        for (int i = 0; i < 3; ++i) { LVCOLUMNW column = {0}; column.mask = LVCF_WIDTH; column.cx = 200; ListView_InsertColumn(state->list, i, &column); }
        state->detail = app_control(window, L"EDIT", L"", WS_TABSTOP | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 0, 0, 0, 0, INFO_DETAIL);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, INFO_STATUS);
        translate_info(state); info_layout(state);
        if (!app_is_smoke_test()) PostMessageW(window, WM_COMMAND, INFO_REFRESH, 0);
        return 0;
    }
    case WM_COMMAND:
        if (!state || app_busy(NULL)) return 0;
        if (LOWORD(wp) == INFO_REFRESH) refresh_info(state);
        if (LOWORD(wp) == INFO_GROUP && HIWORD(wp) == CBN_SELCHANGE) render_inventory(state);
        if (LOWORD(wp) == INFO_COPY && !app_is_smoke_test()) copy_row(state);
        return 0;
    case WM_NOTIFY:
        if (state && ((NMHDR *)lp)->idFrom == INFO_LIST && ((NMHDR *)lp)->code == LVN_ITEMCHANGED) {
            NMLISTVIEW *event = (NMLISTVIEW *)lp;
            if (event->iItem >= 0 && (event->uNewState & LVIS_SELECTED)) {
                wchar_t text[1024]; ListView_GetItemText(state->list, event->iItem, 2, text, ARRAYSIZE(text)); SetWindowTextW(state->detail, text);
            }
        }
        break;
    case WM_APP_JOB_DONE:
        app_finish_job(window); free(state->inventory); state->inventory = state->pending; state->pending = NULL; render_inventory(state); return 0;
    case WM_APP_LANGUAGE: if (state) translate_info(state); return 0;
    case WM_SIZE: if (state && wp != SIZE_MINIMIZED) info_layout(state); return 0;
    case WM_GETMINMAXINFO: ((MINMAXINFO *)lp)->ptMinTrackSize.x = 920; ((MINMAXINFO *)lp)->ptMinTrackSize.y = 640; return 0;
    case WM_CLOSE: if (!app_busy(window) && IsWindowEnabled(window)) DestroyWindow(window); return 0;
    case WM_NCDESTROY:
        if (state) { free(state->inventory); free(state->pending); free(state); }
        SetWindowLongPtrW(window, GWLP_USERDATA, 0); info_window = NULL; break;
    }
    return DefWindowProcW(window, message, wp, lp);
}
void system_info_show(HWND owner) {
    if (IsWindow(info_window)) { ShowWindow(info_window, SW_RESTORE); SetForegroundWindow(info_window); return; }
    WNDCLASSW cls = {0}; cls.hInstance = g_instance; cls.lpfnWndProc = info_proc;
    cls.lpszClassName = L"WAIO.SystemInfo"; cls.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassW(&cls);
    info_window = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 760, owner, NULL, g_instance, NULL);
    if (info_window) ShowWindow(info_window, app_is_smoke_test() ? SW_HIDE : SW_SHOW);
}
BOOL system_info_self_test(void) {
    VARIANT value; VariantInit(&value); V_VT(&value) = VT_BSTR; V_BSTR(&value) = SysAllocString(L"34359738368");
    wchar_t text[80]; variant_text(&value, 1, text, ARRAYSIZE(text)); VariantClear(&value);
    if (wcscmp(text, L"32.00 GiB")) return FALSE;
    V_VT(&value) = VT_NULL; variant_text(&value, 0, text, ARRAYSIZE(text));
    if (text[0]) return FALSE;
    for (unsigned c = 0; c < ARRAYSIZE(categories); ++c)
        for (unsigned f = 0; f < categories[c].count; ++f)
            if (!*categories[c].fields[f].vi || !*categories[c].fields[f].en) return FALSE;
    return TRUE;
}
int system_info_inventory_test(void) {
    Inventory *inventory = calloc(1, sizeof(*inventory)); if (!inventory) return 1;
    collect_inventory(inventory);
    printf("Read-only WMI inventory: %u entries, %u query errors, truncated=%d. No identifiers printed.\n", inventory->count, inventory->errors, inventory->truncated);
    BOOL ok = inventory->count > 20 && inventory->errors == 0 && !inventory->truncated;
    free(inventory); return ok ? 0 : 1;
}
BOOL system_info_ui_test(HWND owner) {
    if (!app_is_smoke_test()) return FALSE;
    system_info_show(owner); if (!info_window) return FALSE;
    InfoWindow *state = (InfoWindow *)GetWindowLongPtrW(info_window, GWLP_USERDATA);
    state->inventory = calloc(1, sizeof(*state->inventory)); if (!state->inventory) { DestroyWindow(info_window); return FALSE; }
    state->inventory->count = 1; state->inventory->rows[0].instance = 1;
    StringCchCopyW(state->inventory->rows[0].value, 1024, L"TEST-PC"); render_inventory(state);
    ListView_SetItemState(state->list, 0, LVIS_SELECTED, LVIS_SELECTED);
    BOOL ok = TRUE, original = g_vi;
    for (int lang = 0; lang < 2; ++lang) {
        g_vi = lang != 0; SendMessageW(info_window, WM_APP_LANGUAGE, 0, 0);
        wchar_t text[80]; ListView_GetItemText(state->list, 0, 1, text, ARRAYSIZE(text));
        if (wcscmp(text, tr(L"Tên máy", L"Computer name")) || ListView_GetNextItem(state->list, -1, LVNI_SELECTED) != 0) ok = FALSE;
    }
    g_vi = original; DestroyWindow(info_window); return ok;
}
