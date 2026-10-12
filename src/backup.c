#include "app.h"
#include <commdlg.h>
#include <wlanapi.h>
#include <sddl.h>
#include <reason.h>
#include <limits.h>

#define BACKUP_PATH 32768
#define PICK_BUFFER 65536
#define XML_LIMIT (4u * 1024u * 1024u)
#define MAX_DELAY_SECONDS (7u * 24u * 60u * 60u)

typedef struct {
    wchar_t folder[BACKUP_PATH];
    BOOL plaintext;
} ExportJob;

typedef struct {
    GUID adapter;
    wchar_t *files;
} ImportJob;

static DWORD open_wlan(HANDLE *client, WLAN_INTERFACE_INFO_LIST **interfaces)
{
    DWORD version = 0;
    DWORD error = WlanOpenHandle(2, NULL, &version, client);
    if (error == ERROR_SUCCESS) {
        error = WlanEnumInterfaces(*client, NULL, interfaces);
        if (error == ERROR_SUCCESS && (*interfaces)->dwNumberOfItems == 0)
            error = ERROR_NOT_FOUND;
        if (error != ERROR_SUCCESS) {
            if (*interfaces) WlanFreeMemory(*interfaces);
            *interfaces = NULL;
            WlanCloseHandle(*client, NULL);
            *client = NULL;
        }
    }
    return error;
}

/* Plaintext exports must have a protected DACL on an ACL-capable volume. */
static BOOL private_security(PSECURITY_DESCRIPTOR *descriptor)
{
    HANDLE token = NULL;
    DWORD needed = 0, error = ERROR_SUCCESS;
    TOKEN_USER *user = NULL;
    LPWSTR sid = NULL;
    wchar_t sddl[256];
    BOOL ok = FALSE;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return FALSE;
    GetTokenInformation(token, TokenUser, NULL, 0, &needed);
    user = (TOKEN_USER *)malloc(needed);
    if (!user) { error = ERROR_NOT_ENOUGH_MEMORY; goto done; }
    if (!GetTokenInformation(token, TokenUser, user, needed, &needed) ||
        !ConvertSidToStringSidW(user->User.Sid, &sid)) {
        error = GetLastError(); goto done;
    }
    if (FAILED(StringCchPrintfW(sddl, ARRAYSIZE(sddl),
            L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;%s)", sid))) {
        error = ERROR_INSUFFICIENT_BUFFER; goto done;
    }
    ok = ConvertStringSecurityDescriptorToSecurityDescriptorW(
        sddl, SDDL_REVISION_1, descriptor, NULL);
    if (!ok) error = GetLastError();
done:
    LocalFree(sid);
    free(user);
    CloseHandle(token);
    if (!ok) SetLastError(error);
    return ok;
}

static BOOL timestamp_folder(wchar_t *path, size_t capacity,
                             const wchar_t *parent, const wchar_t *prefix,
                             BOOL protect)
{
    SYSTEMTIME now;
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), NULL, FALSE};
    PSECURITY_DESCRIPTOR descriptor = NULL;
    DWORD error = ERROR_SUCCESS;
    BOOL ok = FALSE;
    GetLocalTime(&now);
    if (protect) {
        wchar_t volume[BACKUP_PATH];
        DWORD flags = 0;
        if (!GetVolumePathNameW(parent, volume, ARRAYSIZE(volume)) ||
            !GetVolumeInformationW(volume, NULL, 0, NULL, NULL, &flags, NULL, 0))
            return FALSE;
        if (!(flags & FILE_PERSISTENT_ACLS)) {
            SetLastError(ERROR_NOT_SUPPORTED);
            return FALSE;
        }
        if (!private_security(&descriptor)) return FALSE;
        attributes.lpSecurityDescriptor = descriptor;
    }
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        if (FAILED(StringCchPrintfW(path, capacity,
                L"%s\\%s-%04u%02u%02u-%02u%02u%02u-%03u-%02u",
                parent, prefix, now.wYear, now.wMonth, now.wDay,
                now.wHour, now.wMinute, now.wSecond, now.wMilliseconds, attempt))) {
            error = ERROR_FILENAME_EXCED_RANGE; break;
        }
        if (CreateDirectoryW(path, protect ? &attributes : NULL)) { ok = TRUE; break; }
        error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) break;
    }
    LocalFree(descriptor);
    if (!ok) SetLastError(error);
    return ok;
}

static DWORD write_profile(const wchar_t *path, const wchar_t *xml)
{
    DWORD written = 0, error = ERROR_SUCCESS;
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, xml, -1,
                                    NULL, 0, NULL, NULL);
    if (!length) return GetLastError();
    char *bytes = (char *)malloc((size_t)length);
    if (!bytes) return ERROR_NOT_ENOUGH_MEMORY;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, xml, -1,
                            bytes, length, NULL, NULL)) {
        error = GetLastError(); goto done;
    }
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) { error = GetLastError(); goto done; }
    if (!WriteFile(file, bytes, (DWORD)(length - 1), &written, NULL))
        error = GetLastError();
    else if (written != (DWORD)(length - 1)) error = ERROR_WRITE_FAULT;
    if (error == ERROR_SUCCESS && !FlushFileBuffers(file)) error = GetLastError();
    CloseHandle(file);
    if (error != ERROR_SUCCESS) DeleteFileW(path);
done:
    SecureZeroMemory(bytes, (size_t)length);
    free(bytes);
    return error;
}

static void export_wifi_worker(void *context)
{
    ExportJob *job = (ExportJob *)context;
    HANDLE client = NULL;
    WLAN_INTERFACE_INFO_LIST *interfaces = NULL;
    DWORD error = open_wlan(&client, &interfaces);
    unsigned int total = 0, success = 0, failures = 0;
    if (error != ERROR_SUCCESS) {
        app_log(tr(L"Không mở được WLAN (mã %lu). Kiểm tra dịch vụ WLAN AutoConfig và card WiFi.",
                   L"Cannot open WLAN (code %lu). Check WLAN AutoConfig and the WiFi adapter."), error);
        goto done;
    }
    for (DWORD i = 0; i < interfaces->dwNumberOfItems; ++i) {
        WLAN_PROFILE_INFO_LIST *profiles = NULL;
        GUID *id = &interfaces->InterfaceInfo[i].InterfaceGuid;
        error = WlanGetProfileList(client, id, NULL, &profiles);
        if (error != ERROR_SUCCESS) {
            ++failures;
            app_log(tr(L"Không đọc được danh sách WiFi của card %lu (mã %lu).",
                       L"Cannot read WiFi profile list for adapter %lu (code %lu)."), i + 1, error);
            continue;
        }
        for (DWORD p = 0; p < profiles->dwNumberOfItems; ++p) {
            wchar_t *xml = NULL;
            DWORD flags = job->plaintext ? WLAN_PROFILE_GET_PLAINTEXT_KEY : 0;
            DWORD access = 0;
            wchar_t path[BACKUP_PATH];
            ++total;
            error = WlanGetProfile(client, id, profiles->ProfileInfo[p].strProfileName,
                                   NULL, &xml, &flags, &access);
            if (error == ERROR_SUCCESS) {
                if (FAILED(StringCchPrintfW(path, ARRAYSIZE(path),
                        L"%s\\adapter-%03lu-profile-%03lu.xml", job->folder, i + 1, p + 1)))
                    error = ERROR_FILENAME_EXCED_RANGE;
                else error = write_profile(path, xml);
            }
            if (xml) {
                SecureZeroMemory(xml, wcslen(xml) * sizeof(*xml));
                WlanFreeMemory(xml);
            }
            if (error == ERROR_SUCCESS) ++success;
            else {
                ++failures;
                app_log(tr(L"Sao lưu WiFi %u thất bại (mã %lu).",
                           L"WiFi backup %u failed (code %lu)."), total, error);
            }
        }
        WlanFreeMemory(profiles);
    }
    app_log(tr(L"Sao lưu WiFi: %u/%u hồ sơ; %u lỗi. Thư mục: %s",
               L"WiFi backup: %u/%u profiles; %u errors. Folder: %s"), success, total, failures, job->folder);
    if (total == 0)
        app_log(tr(L"Không tìm thấy hồ sơ WiFi đã lưu.", L"No saved WiFi profiles were found."));
    if (job->plaintext)
        app_log(tr(L"Giữ thư mục sao lưu riêng tư: XML có thể chứa mật khẩu WiFi.",
                   L"Keep the backup folder private: XML files may contain WiFi passwords."));
done:
    if (interfaces) WlanFreeMemory(interfaces);
    if (client) WlanCloseHandle(client, NULL);
    free(job);
}

void backup_wifi(HWND owner)
{
    if (app_busy(NULL)) return;
    wchar_t parent[BACKUP_PATH];
    if (!app_pick_folder(owner, parent, ARRAYSIZE(parent))) return;
    int choice = MessageBoxW(owner, tr(
        L"Có xuất mật khẩu WiFi để dùng bản sao lưu trên máy khác không?\n\n"
        L"Có: XML chứa mật khẩu đọc được. Chỉ lưu trên ổ hỗ trợ quyền NTFS và giữ riêng tư.\n"
        L"Không: giữ khóa được Windows mã hóa; có thể không khôi phục được trên máy khác.\n"
        L"Hủy: không sao lưu. Chứng chỉ và thông tin đăng nhập doanh nghiệp không được xuất.",
        L"Export WiFi passwords so the backup can be used on another PC?\n\n"
        L"Yes: XML contains readable passwords. Use an ACL-capable drive and keep it private.\n"
        L"No: keep Windows-encrypted keys; restoring on another PC may fail.\n"
        L"Cancel: do not back up. Enterprise certificates and credentials are not exported."),
        tr(L"Quyền sao lưu WiFi", L"WiFi backup permission"),
        MB_YESNOCANCEL | MB_ICONWARNING | MB_DEFBUTTON2);
    if (choice != IDYES && choice != IDNO) return;
    ExportJob *job = (ExportJob *)calloc(1, sizeof(*job));
    if (!job) { app_error(owner, L"WiFi", ERROR_NOT_ENOUGH_MEMORY); return; }
    job->plaintext = choice == IDYES;
    if (!timestamp_folder(job->folder, ARRAYSIZE(job->folder), parent, L"WiFi", job->plaintext)) {
        app_error(owner, tr(L"Không tạo được thư mục sao lưu an toàn (mật khẩu yêu cầu ổ hỗ trợ ACL)",
                            L"Cannot create backup folder (password exports require an ACL-capable drive)"), GetLastError());
        free(job); return;
    }
    if (!app_start_job(owner, export_wifi_worker, job)) free(job);
}

/* Accept the UTF-8 and UTF-16 XML formats produced by Windows/netsh. */
static wchar_t *read_profile(const wchar_t *path, DWORD *error)
{
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) { *error = GetLastError(); return NULL; }
    LARGE_INTEGER size;
    unsigned char *bytes = NULL;
    wchar_t *xml = NULL;
    DWORD received = 0;
    size_t characters = 0;
    if (!GetFileSizeEx(file, &size)) { *error = GetLastError(); goto done; }
    if (size.QuadPart <= 0 || size.QuadPart > XML_LIMIT) { *error = ERROR_FILE_TOO_LARGE; goto done; }
    bytes = (unsigned char *)malloc((size_t)size.QuadPart);
    if (!bytes) { *error = ERROR_NOT_ENOUGH_MEMORY; goto done; }
    if (!ReadFile(file, bytes, (DWORD)size.QuadPart, &received, NULL)) { *error = GetLastError(); goto done; }
    if (received != (DWORD)size.QuadPart) { *error = ERROR_HANDLE_EOF; goto done; }
    if (received >= 2 && ((bytes[0] == 0xff && bytes[1] == 0xfe) ||
                          (bytes[0] == 0xfe && bytes[1] == 0xff))) {
        if (received % 2) { *error = ERROR_INVALID_DATA; goto done; }
        characters = (received - 2) / 2;
        xml = (wchar_t *)calloc(characters + 1, sizeof(*xml));
        if (!xml) { *error = ERROR_NOT_ENOUGH_MEMORY; goto done; }
        for (size_t i = 0; i < characters; ++i) {
            size_t b = i * 2 + 2;
            xml[i] = (wchar_t)(bytes[0] == 0xff ?
                bytes[b] | ((unsigned int)bytes[b + 1] << 8) :
                bytes[b + 1] | ((unsigned int)bytes[b] << 8));
        }
    } else {
        DWORD offset = received >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf ? 3 : 0;
        int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        (const char *)bytes + offset, (int)(received - offset), NULL, 0);
        if (!length) { *error = GetLastError(); goto done; }
        characters = (size_t)length;
        xml = (wchar_t *)calloc(characters + 1, sizeof(*xml));
        if (!xml) { *error = ERROR_NOT_ENOUGH_MEMORY; goto done; }
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char *)bytes + offset,
                                 (int)(received - offset), xml, length)) { *error = GetLastError(); goto done; }
    }
    if (wcslen(xml) != characters || !wcsstr(xml, L"WLANProfile") ||
        wcsstr(xml, L"<!DOCTYPE") || wcsstr(xml, L"<!ENTITY")) {
        *error = ERROR_INVALID_DATA; goto done;
    }
    *error = ERROR_SUCCESS;
done:
    CloseHandle(file);
    if (bytes) { SecureZeroMemory(bytes, (size_t)size.QuadPart); free(bytes); }
    if (*error != ERROR_SUCCESS && xml) {
        SecureZeroMemory(xml, (characters + 1) * sizeof(*xml)); free(xml); xml = NULL;
    }
    return xml;
}

static void import_wifi_worker(void *context)
{
    ImportJob *job = (ImportJob *)context;
    HANDLE client = NULL;
    DWORD version = 0, error = WlanOpenHandle(2, NULL, &version, &client);
    unsigned int total = 0, success = 0;
    if (error != ERROR_SUCCESS) {
        app_log(tr(L"Không mở được WLAN (mã %lu).", L"Cannot open WLAN (code %lu)."), error);
        goto done;
    }
    const wchar_t *entry = job->files + wcslen(job->files) + 1;
    BOOL multiple = *entry != L'\0';
    if (!multiple) entry = job->files;
    do {
        wchar_t path[BACKUP_PATH];
        wchar_t *xml = NULL;
        DWORD reason = 0;
        ++total;
        HRESULT result = multiple ? StringCchPrintfW(path, ARRAYSIZE(path), L"%s\\%s", job->files, entry)
                                  : StringCchCopyW(path, ARRAYSIZE(path), entry);
        if (FAILED(result)) error = ERROR_FILENAME_EXCED_RANGE;
        else xml = read_profile(path, &error);
        if (xml) {
            error = WlanSetProfile(client, &job->adapter, WLAN_PROFILE_USER, xml, NULL, TRUE, NULL, &reason);
            SecureZeroMemory(xml, (wcslen(xml) + 1) * sizeof(*xml));
            free(xml);
        }
        if (error == ERROR_SUCCESS) ++success;
        else app_log(tr(L"Khôi phục WiFi %u thất bại (mã %lu, lý do WLAN %lu). Hồ sơ toàn máy có cùng tên sẽ không bị ghi đè.",
                        L"WiFi restore %u failed (code %lu, WLAN reason %lu). Existing all-user profiles with the same name are not overwritten."),
                     total, error, reason);
        if (!multiple) break;
        entry += wcslen(entry) + 1;
    } while (*entry);
    app_log(tr(L"Khôi phục WiFi: %u/%u hồ sơ cho người dùng hiện tại.",
               L"WiFi restore: %u/%u profiles for the current user."), success, total);
done:
    if (client) WlanCloseHandle(client, NULL);
    free(job->files);
    free(job);
}

void restore_wifi(HWND owner)
{
    if (app_busy(NULL)) return;
    HANDLE client = NULL;
    WLAN_INTERFACE_INFO_LIST *interfaces = NULL;
    DWORD error = open_wlan(&client, &interfaces);
    if (error != ERROR_SUCCESS) { app_error(owner, L"WLAN AutoConfig / WiFi", error); return; }
    TASKDIALOG_BUTTON *options = (TASKDIALOG_BUTTON *)calloc(interfaces->dwNumberOfItems, sizeof(*options));
    if (!options) {
        WlanFreeMemory(interfaces); WlanCloseHandle(client, NULL);
        app_error(owner, L"WiFi", ERROR_NOT_ENOUGH_MEMORY); return;
    }
    for (DWORD i = 0; i < interfaces->dwNumberOfItems; ++i) {
        options[i].nButtonID = 100 + (int)i;
        options[i].pszButtonText = interfaces->InterfaceInfo[i].strInterfaceDescription;
    }
    TASKDIALOGCONFIG dialog = {0};
    dialog.cbSize = sizeof(dialog);
    dialog.hwndParent = owner;
    dialog.hInstance = g_instance;
    dialog.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
    dialog.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
    dialog.pszWindowTitle = tr(L"Khôi phục WiFi", L"Restore WiFi");
    dialog.pszMainInstruction = tr(L"Chọn card WiFi đích", L"Select the target WiFi adapter");
    dialog.pszContent = tr(L"Các hồ sơ sẽ được thêm cho người dùng Windows hiện tại.",
                           L"Profiles will be added for the current Windows user.");
    dialog.cRadioButtons = interfaces->dwNumberOfItems;
    dialog.pRadioButtons = options;
    dialog.nDefaultRadioButton = 100;
    int button = IDCANCEL, radio = 100;
    HRESULT result = TaskDialogIndirect(&dialog, &button, &radio, NULL);
    GUID adapter = {0};
    if (SUCCEEDED(result) && button == IDOK && radio >= 100 &&
        (DWORD)(radio - 100) < interfaces->dwNumberOfItems)
        adapter = interfaces->InterfaceInfo[radio - 100].InterfaceGuid;
    else button = IDCANCEL;
    free(options); WlanFreeMemory(interfaces); WlanCloseHandle(client, NULL);
    if (FAILED(result)) { app_error(owner, tr(L"Chọn card WiFi", L"WiFi adapter selection"), (DWORD)result); return; }
    if (button != IDOK) return;
    ImportJob *job = (ImportJob *)calloc(1, sizeof(*job));
    if (!job) { app_error(owner, L"WiFi", ERROR_NOT_ENOUGH_MEMORY); return; }
    job->adapter = adapter;
    job->files = (wchar_t *)calloc(PICK_BUFFER, sizeof(wchar_t));
    if (!job->files) { free(job); app_error(owner, L"WiFi", ERROR_NOT_ENOUGH_MEMORY); return; }
    OPENFILENAMEW picker = {0};
    picker.lStructSize = sizeof(picker);
    picker.hwndOwner = owner;
    picker.lpstrFilter = L"WiFi XML (*.xml)\0*.xml\0\0";
    picker.lpstrFile = job->files;
    picker.nMaxFile = PICK_BUFFER;
    picker.lpstrTitle = tr(L"Chọn bản sao lưu WiFi XML", L"Select WiFi XML backups");
    picker.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST |
                   OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&picker)) {
        error = CommDlgExtendedError();
        if (error) app_error(owner, tr(L"Chọn file XML WiFi", L"WiFi XML selection"), error);
        free(job->files); free(job); return;
    }
    if (!app_confirm(owner, tr(
            L"Nhập các tệp XML đã chọn vào card WiFi đã chọn?\n\n"
            L"Hồ sơ WiFi trùng tên của người dùng hiện tại sẽ bị thay thế. "
            L"Chỉ nhập tệp từ nguồn bạn tin cậy: hồ sơ có thể tự kết nối mạng.",
            L"Import the selected XML files to the selected WiFi adapter?\n\n"
            L"Existing current-user WiFi profiles with the same names will be replaced. "
            L"Only import trusted backups: profiles may connect to networks automatically."))) {
        free(job->files); free(job); return;
    }
    if (!app_start_job(owner, import_wifi_worker, job)) { free(job->files); free(job); }
}

static void export_drivers_worker(void *context)
{
    ExportJob *job = (ExportJob *)context;
    wchar_t arguments[BACKUP_PATH + 128];
    if (FAILED(StringCchPrintfW(arguments, ARRAYSIZE(arguments),
            L"/Online /Export-Driver /Destination:\"%s\"", job->folder))) {
        app_log(tr(L"Đường dẫn sao lưu driver quá dài.", L"Driver backup path is too long."));
        free(job); return;
    }
    DWORD result = app_run_tool(L"dism.exe", arguments);
    if (result == ERROR_SUCCESS || result == ERROR_SUCCESS_REBOOT_REQUIRED)
        app_log(tr(L"Đã xuất driver bên thứ ba (INF) vào: %s%s",
                   L"Third-party drivers (INF) exported to: %s%s"), job->folder,
                result == ERROR_SUCCESS_REBOOT_REQUIRED ? tr(L" — Windows yêu cầu khởi động lại.", L" — Windows requests a restart.") : L"");
    else app_log(tr(L"Sao lưu driver thất bại (mã %lu). Thư mục có thể chỉ chứa một phần dữ liệu: %s",
                    L"Driver backup failed (code %lu). The folder may contain a partial export: %s"), result, job->folder);
    free(job);
}

void backup_drivers(HWND owner)
{
    if (app_busy(NULL)) return;
    wchar_t parent[BACKUP_PATH];
    if (!app_pick_folder(owner, parent, ARRAYSIZE(parent))) return;
    if (!app_confirm(owner, tr(
            L"Xuất các gói driver bên thứ ba (INF) vào thư mục mới có dấu thời gian?\n\n"
            L"Không bao gồm driver tích hợp Windows hay trình cài EXE/MSI của nhà sản xuất. Có thể mất vài phút.",
            L"Export third-party driver packages (INF) into a new timestamped folder?\n\n"
            L"Windows built-in drivers and vendor EXE/MSI installers are not included. This may take several minutes."))) return;
    ExportJob *job = (ExportJob *)calloc(1, sizeof(*job));
    if (!job) { app_error(owner, L"DISM", ERROR_NOT_ENOUGH_MEMORY); return; }
    if (!timestamp_folder(job->folder, ARRAYSIZE(job->folder), parent, L"Drivers", FALSE)) {
        app_error(owner, tr(L"Thư mục sao lưu driver", L"Driver backup folder"), GetLastError());
        free(job); return;
    }
    if (!app_start_job(owner, export_drivers_worker, job)) free(job);
}

BOOL backup_valid_delay(const wchar_t *text, BOOL hours, DWORD *seconds)
{
    if (!text || !*text || !seconds) return FALSE;
    DWORD value = 0;
    DWORD multiplier = hours ? 3600 : 60;
    DWORD maximum = MAX_DELAY_SECONDS / multiplier;
    for (const wchar_t *p = text; *p; ++p) {
        if (*p < L'0' || *p > L'9') return FALSE;
        DWORD digit = (DWORD)(*p - L'0');
        if (value > maximum / 10 ||
            (value == maximum / 10 && digit > maximum % 10)) return FALSE;
        value = value * 10 + digit;
    }
    if (value == 0) return FALSE;
    *seconds = value * multiplier;
    return TRUE;
}

typedef struct {
    DWORD magic;
    DWORD version;
    ULONGLONG start_tick;
    ULONGLONG end_tick;
    ULONGLONG boot_time;
} ShutdownState;

static ShutdownState timer_state;
static BOOL timer_loaded;

static ULONGLONG current_filetime(void)
{
    FILETIME now;
    ULARGE_INTEGER value;
    GetSystemTimeAsFileTime(&now);
    value.LowPart = now.dwLowDateTime;
    value.HighPart = now.dwHighDateTime;
    return value.QuadPart;
}

static void clear_timer_file(void)
{
    wchar_t path[BACKUP_PATH];
    if (app_data_path(path, ARRAYSIZE(path), L"shutdown.bin")) DeleteFileW(path);
    ZeroMemory(&timer_state, sizeof(timer_state));
}

static void load_timer(void)
{
    if (timer_loaded) return;
    timer_loaded = TRUE;
    wchar_t path[BACKUP_PATH];
    if (!app_data_path(path, ARRAYSIZE(path), L"shutdown.bin")) return;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD received = 0;
    BOOL ok = ReadFile(file, &timer_state, sizeof(timer_state), &received, NULL);
    CloseHandle(file);
    ULONGLONG tick = GetTickCount64();
    ULONGLONG boot = current_filetime() - tick * 10000;
    ULONGLONG difference = boot > timer_state.boot_time ? boot - timer_state.boot_time : timer_state.boot_time - boot;
    /* Five minutes tolerates ordinary clock synchronization; tick checks catch reboot.
       A large manual clock change is treated as an unverified old request, never replayed. */
    if (!ok || received != sizeof(timer_state) || timer_state.magic != 0x57555453 ||
        timer_state.version != 1 || timer_state.end_tick <= timer_state.start_tick ||
        timer_state.end_tick - timer_state.start_tick > (ULONGLONG)MAX_DELAY_SECONDS * 1000 ||
        tick < timer_state.start_tick || tick >= timer_state.end_tick ||
        difference > 300ULL * 10000000ULL) clear_timer_file();
}

static BOOL enable_shutdown_privilege(HANDLE *token, TOKEN_PRIVILEGES *previous)
{
    TOKEN_PRIVILEGES privileges = {0};
    DWORD size = sizeof(*previous), error;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, token)) return FALSE;
    if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid)) {
        error = GetLastError(); CloseHandle(*token); *token = NULL; SetLastError(error); return FALSE;
    }
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    BOOL ok = AdjustTokenPrivileges(*token, FALSE, &privileges, sizeof(*previous), previous, &size);
    error = GetLastError();
    if (!ok || error != ERROR_SUCCESS) {
        CloseHandle(*token); *token = NULL; SetLastError(error); return FALSE;
    }
    return TRUE;
}

static void restore_shutdown_privilege(HANDLE token, TOKEN_PRIVILEGES *previous)
{
    AdjustTokenPrivileges(token, FALSE, previous, 0, NULL, NULL);
    CloseHandle(token);
}

void shutdown_schedule(HWND owner, const wchar_t *value, BOOL hours)
{
    if (app_busy(NULL)) return;
    DWORD seconds = 0;
    if (!backup_valid_delay(value, hours, &seconds)) {
        MessageBoxW(owner, tr(L"Nhập số nguyên dương: 1–10080 phút hoặc 1–168 giờ (tối đa 7 ngày).",
                              L"Enter a positive whole number: 1–10080 minutes or 1–168 hours (up to 7 days)."),
                    tr(L"Thời gian không hợp lệ", L"Invalid delay"), MB_OK | MB_ICONWARNING);
        return;
    }
    wchar_t confirmation[768];
    StringCchPrintfW(confirmation, ARRAYSIZE(confirmation), tr(
        L"Yêu cầu Windows tắt máy sau %lu phút?\n\n"
        L"Windows sẽ buộc đóng ứng dụng khi hết giờ; dữ liệu chưa lưu có thể bị mất. "
        L"Hãy lưu công việc. Lịch vẫn hoạt động khi đóng tiện ích này; bạn có thể hủy trước khi hết giờ.",
        L"Ask Windows to shut down in %lu minutes?\n\n"
        L"Windows will force applications to close when the timer expires; unsaved work may be lost. "
        L"Save your work. The request remains active after this utility closes; you can cancel before the deadline."), seconds / 60);
    if (!app_confirm(owner, confirmation)) return;
    load_timer();
    wchar_t path[BACKUP_PATH], temporary[BACKUP_PATH];
    if (!app_data_path(path, ARRAYSIZE(path), L"shutdown.bin") ||
        !app_data_path(temporary, ARRAYSIZE(temporary), L"shutdown.pending")) {
        app_error(owner, tr(L"Đường dẫn lưu lịch tắt máy", L"Shutdown state path"), ERROR_PATH_NOT_FOUND); return;
    }
    ShutdownState state = {0x57555453, 1, 0, 0, 0};
    state.start_tick = GetTickCount64();
    state.end_tick = state.start_tick + (ULONGLONG)seconds * 1000;
    state.boot_time = current_filetime() - state.start_tick * 10000;
    HANDLE file = CreateFileW(temporary, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) { app_error(owner, tr(L"Trạng thái lịch tắt máy", L"Shutdown state"), GetLastError()); return; }
    DWORD written = 0;
    BOOL saved = WriteFile(file, &state, sizeof(state), &written, NULL);
    DWORD error = saved ? ERROR_SUCCESS : GetLastError();
    if (saved && written != sizeof(state)) { saved = FALSE; error = ERROR_WRITE_FAULT; }
    if (saved && !FlushFileBuffers(file)) { saved = FALSE; error = GetLastError(); }
    CloseHandle(file);
    if (!saved) { DeleteFileW(temporary); app_error(owner, tr(L"Trạng thái lịch tắt máy", L"Shutdown state"), error); return; }
    HANDLE token = NULL;
    TOKEN_PRIVILEGES previous = {0};
    if (!enable_shutdown_privilege(&token, &previous)) {
        error = GetLastError(); DeleteFileW(temporary); app_error(owner, tr(L"Quyền tắt máy", L"Shutdown privilege"), error); return;
    }
    wchar_t message[160]; StringCchCopyW(message, ARRAYSIZE(message), tr(L"Win AIO Optimize: sắp tắt máy theo lịch. Hãy lưu công việc.", L"Win AIO Optimize: scheduled shutdown. Save your work."));
    if (!InitiateSystemShutdownExW(NULL, message, seconds, TRUE, FALSE,
                                   SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED)) {
        error = GetLastError(); DeleteFileW(temporary);
        restore_shutdown_privilege(token, &previous);
        app_error(owner, tr(L"Windows không chấp nhận lịch tắt máy", L"Windows did not accept the shutdown request"), error);
        return;
    }
    timer_state = state;
    if (!MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = GetLastError();
        BOOL aborted = AbortSystemShutdownW(NULL);
        if (aborted) clear_timer_file();
        DeleteFileW(temporary);
        restore_shutdown_privilege(token, &previous);
        app_error(owner, tr(L"Không lưu được lịch tắt máy", L"Could not persist the shutdown request"), error);
        app_log(aborted ? tr(L"Đã hủy lịch vì không lưu được dữ liệu.", L"The request was cancelled because its state could not be saved.") :
                              tr(L"Lịch tắt máy vẫn hoạt động; dùng Hủy lịch để dừng. Không thể phục hồi đồng hồ sau khi mở lại ứng dụng.",
                                 L"Shutdown is still scheduled; use Cancel shutdown to stop it. Countdown cannot be recovered after reopening."));
        return;
    }
    restore_shutdown_privilege(token, &previous);
    app_log(tr(L"Windows đã chấp nhận lịch tắt máy sau %lu phút. Lịch vẫn chạy khi đóng tiện ích.",
               L"Windows accepted a shutdown request for %lu minutes from now. It remains active after the utility closes."), seconds / 60);
}

void shutdown_cancel(HWND owner)
{
    if (app_busy(NULL)) return;
    if (!app_confirm(owner, tr(L"Hủy yêu cầu tắt máy đang chờ của Windows? Yêu cầu này có thể được tạo bởi ứng dụng khác.",
                               L"Cancel the pending Windows shutdown request? It may have been created by another application."))) return;
    HANDLE token = NULL;
    TOKEN_PRIVILEGES previous = {0};
    if (!enable_shutdown_privilege(&token, &previous)) { app_error(owner, tr(L"Quyền tắt máy", L"Shutdown privilege"), GetLastError()); return; }
    BOOL cancelled = AbortSystemShutdownW(NULL);
    DWORD error = cancelled ? ERROR_SUCCESS : GetLastError();
    restore_shutdown_privilege(token, &previous);
    if (!cancelled) {
        app_error(owner, tr(L"Windows không xác nhận hủy lịch tắt máy", L"Windows did not confirm shutdown cancellation"), error);
        return;
    }
    timer_loaded = TRUE;
    clear_timer_file();
    app_log(tr(L"Windows đã hủy yêu cầu tắt máy.", L"Windows cancelled the shutdown request."));
}

void shutdown_status(wchar_t *text, size_t capacity)
{
    if (!text || !capacity) return;
    load_timer();
    ULONGLONG now = GetTickCount64();
    if (timer_state.magic && now >= timer_state.end_tick) clear_timer_file();
    if (!timer_state.magic) {
        StringCchCopyW(text, capacity, tr(L"Không có lịch do tiện ích lưu.", L"No shutdown request saved by this utility."));
        return;
    }
    ULONGLONG left = (timer_state.end_tick - now + 999) / 1000;
    StringCchPrintfW(text, capacity, tr(L"Đã yêu cầu tắt máy: còn %llu:%02llu:%02llu (có thể bị hủy bên ngoài).",
                                      L"Requested shutdown: %llu:%02llu:%02llu remaining (may be cancelled externally)."),
                    left / 3600, (left / 60) % 60, left % 60);
}
