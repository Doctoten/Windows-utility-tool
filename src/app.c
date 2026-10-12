#include "app.h"
#include "theme.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <uxtheme.h>
#include <stdio.h>
#include <stdarg.h>

HINSTANCE g_instance;
HFONT g_font;
BOOL g_vi = TRUE;
static HWND main_window, log_window, progress_window, timer_status;
static HFONT title_font;
static LONG running_job;
static CRITICAL_SECTION log_lock;
static wchar_t log_path[MAX_PATH];
static BOOL log_ready;
static BOOL smoke_mode;
static BOOL smoke_failed;
static DWORD windows_build;

enum {
    ID_SETUP = 101, ID_NETWORK, ID_BLOAT, ID_WIFI, ID_DRIVERS, ID_RESTORE,
    ID_DELAY, ID_UNIT, ID_SCHEDULE, ID_CANCEL, ID_LANGUAGE, ID_TITLE, ID_SUBTITLE,
    ID_ACCOUNT, ID_TIMER_LABEL, ID_LOG_LABEL, ID_OPEN_LOG, ID_SYSTEM_INFO, ID_THEME,
    ID_PC_SUMMARY, ID_PC_LABEL, ID_NAV_LABEL, ID_BACKUPS, ID_PROTECTION_NOTE, ID_BRAND,
    ID_SYSTEM_PROTECTION, ID_DETAILS
};

const wchar_t *tr(const wchar_t *vi, const wchar_t *en) { return g_vi ? vi : en; }
BOOL app_is_smoke_test(void) { return smoke_mode; }

static const wchar_t language_key[] = L"Software\\Doctoten\\WinAIOOptimize";
static void language_load(void) {
    DWORD value = 0, size = sizeof(value);
    g_vi = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_VIETNAMESE;
    if (RegGetValueW(HKEY_CURRENT_USER, language_key, L"Vietnamese", RRF_RT_REG_DWORD,
                     NULL, &value, &size) == ERROR_SUCCESS && value <= 1)
        g_vi = value != 0;
}
static void language_save(HWND owner) {
    if (smoke_mode) return;
    HKEY key;
    DWORD value = g_vi ? 1 : 0;
    LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, language_key, 0, NULL, 0,
                                   KEY_SET_VALUE, NULL, &key, NULL);
    if (error == ERROR_SUCCESS) {
        error = RegSetValueExW(key, L"Vietnamese", 0, REG_DWORD, (BYTE *)&value, sizeof(value));
        RegCloseKey(key);
    }
    if (error != ERROR_SUCCESS)
        app_error(owner, tr(L"Không lưu được ngôn ngữ", L"Could not save language preference"), (DWORD)error);
}
static void language_thread(void) {
    SetThreadUILanguage(g_vi ? MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT)
                            : MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
}

HWND app_control(HWND parent, const wchar_t *cls, const wchar_t *text, DWORD style,
                 int x, int y, int w, int h, int id) {
    if (!lstrcmpiW(cls, L"COMBOBOX")) style |= CBS_OWNERDRAWFIXED | CBS_HASSTRINGS;
    if (!lstrcmpiW(cls, L"STATIC")) style |= SS_NOPREFIX;
    if (!lstrcmpiW(cls, L"EDIT") || !wcscmp(cls, WC_LISTVIEWW)) style &= ~WS_BORDER;
    DWORD extended = 0;
    HWND control = CreateWindowExW(extended, cls, text, WS_CHILD | WS_VISIBLE | style,
                                  x, y, w, h, parent, (HMENU)(INT_PTR)id, g_instance, NULL);
    if (control) {
        SendMessageW(control, WM_SETFONT, (WPARAM)g_font, TRUE);
        SetWindowTheme(control, L"Explorer", NULL);
        theme_control(control);
        if (!lstrcmpiW(cls, L"COMBOBOX")) {
            SendMessageW(control, CB_SETITEMHEIGHT, (WPARAM)-1, 26);
            SendMessageW(control, CB_SETITEMHEIGHT, 0, 26);
        }
    }
    return control;
}

BOOL app_data_path(wchar_t *path, size_t capacity, const wchar_t *filename) {
    if (smoke_mode) return FALSE;
    wchar_t base[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, base))) return FALSE;
    if (FAILED(StringCchPrintfW(path, capacity, L"%s\\WinAIOOptimize", base))) return FALSE;
    if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return FALSE;
    if (filename && *filename) {
        if (FAILED(StringCchCatW(path, capacity, L"\\")) ||
            FAILED(StringCchCatW(path, capacity, filename))) return FALSE;
    }
    return TRUE;
}

void app_log(const wchar_t *format, ...) {
    wchar_t message[8192], line[8300];
    va_list args;
    SYSTEMTIME now;
    va_start(args, format);
    StringCchVPrintfW(message, ARRAYSIZE(message), format, args);
    va_end(args);
    GetLocalTime(&now);
    StringCchPrintfW(line, ARRAYSIZE(line), L"[%02u:%02u:%02u] %s\r\n",
                     now.wHour, now.wMinute, now.wSecond, message);
    if (log_ready) {
        EnterCriticalSection(&log_lock);
        HANDLE file = CreateFileW(log_path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE) {
            char encoded[33200];
            int bytes = WideCharToMultiByte(CP_UTF8, 0, line, -1, encoded, sizeof(encoded), NULL, NULL);
            DWORD written;
            if (bytes > 1) WriteFile(file, encoded, (DWORD)bytes - 1, &written, NULL);
            CloseHandle(file);
        }
        LeaveCriticalSection(&log_lock);
    }
    if (main_window) {
        size_t bytes = (wcslen(line) + 1) * sizeof(wchar_t);
        wchar_t *copy = malloc(bytes);
        if (copy) {
            memcpy(copy, line, bytes);
            if (!PostMessageW(main_window, WM_APP_LOG, 0, (LPARAM)copy)) free(copy);
        }
    }
}

void app_error(HWND owner, const wchar_t *context, DWORD error) {
    if (smoke_mode) {
        fprintf(stderr, "UI test error: %lu\n", error); fflush(stderr);
        smoke_failed = TRUE;
        return;
    }
    wchar_t detail[1024] = L"", message[2048];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error,
                   0, detail, ARRAYSIZE(detail), NULL);
    StringCchPrintfW(message, ARRAYSIZE(message), L"%s\n\n%lu (0x%08lX): %s", context, error, error, detail);
    app_log(L"%s", message);
    MessageBoxW(owner, message, tr(L"Không thể hoàn tất", L"Unable to complete"), MB_OK | MB_ICONERROR);
}

/* A scrollable review is necessary for long lists of selected tweaks/packages. */
enum { ID_PROTECT_STATUS = 703 };
typedef struct {
    const wchar_t *details;
    BOOL accepted, protect, registry, restore, working;
    DWORD error;
} Confirmation;
static void confirmation_prepare(void *argument) {
    Confirmation *state = argument;
    state->error = protection_prepare(state->registry, state->restore);
}
static LRESULT CALLBACK confirm_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    Confirmation *state = (Confirmation *)GetWindowLongPtrW(window, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE: {
        state = ((CREATESTRUCTW *)lp)->lpCreateParams;
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)state);
        theme_attach(window);
        app_control(window, L"STATIC", tr(L"Xem lại thay đổi trước khi đồng ý", L"Review the changes before you agree"),
                    0, 24, 18, 744, 30, 0);
        HWND details = app_control(window, L"EDIT", state->details,
                    ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP,
                    24, 58, 744, 315, 10);
        if (!details) return -1; /* Never accept changes if their review cannot be displayed. */
        SendMessageW(details, EM_SETLIMITTEXT, 131072, 0);
        SendMessageW(details, EM_SETSEL, 0, 0);
        if (state->protect) {
            app_control(window, L"STATIC", tr(
                L"Registry: lưu SYSTEM, SOFTWARE và HKCU vào thư mục dữ liệu ứng dụng.\r\n"
                L"System Protection phải được bật; Windows có thể dùng lại điểm khôi phục gần đây.\r\n"
                L"Bảo vệ đã chọn phải hoàn tất trước khi thay đổi. Không thay thế sao lưu file cá nhân.",
                L"Registry: save SYSTEM, SOFTWARE and HKCU in the app data folder.\r\n"
                L"System Protection must be enabled; Windows may reuse a recent restore point.\r\n"
                L"Selected protection must succeed before changes. This does not back up personal files."),
                0, 24, 391, 744, 78, 0);
            app_control(window, L"STATIC", L"", 0, 24, 482, 744, 70, ID_PROTECT_STATUS);
        }
        app_control(window, L"BUTTON", tr(L"Hủy", L"Cancel"), WS_TABSTOP | BS_DEFPUSHBUTTON,
                    476, state->protect ? 568 : 394, 130, 40, IDCANCEL);
        app_control(window, L"BUTTON", tr(L"Tôi đồng ý", L"I agree"), WS_TABSTOP,
                    618, state->protect ? 568 : 394, 150, 40, IDOK);
        SetFocus(GetDlgItem(window, IDCANCEL));
        return 0;
    }
    case WM_COMMAND:
        if (!state || state->working) return 0;
        if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
            if (LOWORD(wp) == IDOK && state->protect) {
                if (state->registry || state->restore) {
                    state->working = TRUE;
                    SetDlgItemTextW(window, ID_PROTECT_STATUS, tr(L"Đang tạo bản bảo vệ… Chưa áp dụng thay đổi. Vui lòng chờ.",
                        L"Preparing recovery options… No changes applied. Please wait."));
                    if (!app_start_job(window, confirmation_prepare, state)) {
                        state->working = FALSE;
                        SetDlgItemTextW(window, ID_PROTECT_STATUS, tr(
                            L"Không bắt đầu được bước bảo vệ. Chưa áp dụng thay đổi. Thử lại hoặc Hủy.",
                            L"Could not start recovery preparation. No changes applied. Retry or Cancel."));
                    }
                    return 0;
                }
            }
            state->accepted = LOWORD(wp) == IDOK;
            DestroyWindow(window);
        }
        return 0;
    case WM_APP_JOB_DONE:
        state->working = FALSE; app_finish_job(window);
        if (!state->error) { state->accepted = TRUE; DestroyWindow(window); }
        else {
            wchar_t text[512];
            StringCchPrintfW(text, ARRAYSIZE(text), tr(
                L"Bảo vệ thất bại (mã %lu). CHƯA áp dụng thay đổi. Kiểm tra nhật ký hoặc System Protection, rồi thử lại. Chọn Hủy để về Windows Setup và điều chỉnh lựa chọn bảo vệ.",
                L"Protection failed (code %lu). NO changes applied. Check the log or System Protection, then retry. Choose Cancel to return to Windows Setup and adjust recovery options."), state->error);
            SetDlgItemTextW(window, ID_PROTECT_STATUS, text); app_log(L"%s", text);
            SetFocus(GetDlgItem(window, IDCANCEL));
        }
        return 0;
    case WM_CLOSE: if (!state || !state->working) DestroyWindow(window); return 0;
    case DM_GETDEFID: return MAKELONG(IDCANCEL, DC_HASDEFID);
    }
    return DefWindowProcW(window, message, wp, lp);
}

static BOOL confirm_dialog(HWND owner, const wchar_t *details, BOOL registry, BOOL restore) {
    if (smoke_mode) return FALSE;
    WNDCLASSW cls = {0};
    cls.hInstance = g_instance; cls.lpfnWndProc = confirm_proc;
    cls.lpszClassName = L"WAIO.Confirm"; cls.hCursor = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassW(&cls);
    Confirmation state = {0}; state.details = details; state.protect = registry || restore;
    state.registry = registry; state.restore = restore;
    BOOL protect = state.protect;
    RECT area; GetWindowRect(owner, &area);
    RECT work; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int height = protect ? 660 : 488;
    int x = area.left + 24, y = area.top + 24;
    if (x + 812 > work.right) x = work.right - 812;
    if (y + height > work.bottom) y = work.bottom - height;
    if (x < work.left) x = work.left;
    if (y < work.top) y = work.top;
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, cls.lpszClassName,
                    tr(L"Xác nhận thay đổi", L"Confirm changes"), WS_CAPTION | WS_SYSMENU,
                    x, y, 812, height, owner, NULL, g_instance, &state);
    if (!dialog) return FALSE;
    HWND root_owner = GetAncestor(owner, GA_ROOTOWNER);
    BOOL owner_was_enabled = IsWindowEnabled(owner);
    BOOL root_was_enabled = root_owner && root_owner != owner && IsWindowEnabled(root_owner);
    EnableWindow(owner, FALSE);
    if (root_was_enabled) EnableWindow(root_owner, FALSE);
    ShowWindow(dialog, SW_SHOW);
    SetFocus(GetDlgItem(dialog, IDCANCEL));
    MSG msg;
    BOOL quit = FALSE;
    while (IsWindow(dialog)) {
        int result = (int)GetMessageW(&msg, NULL, 0, 0);
        if (result <= 0) {
            quit = TRUE;
            if (state.working) continue; /* Drain the completion before this stack context can go away. */
            break;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE &&
            (msg.hwnd == dialog || IsChild(dialog, msg.hwnd))) {
            SendMessageW(dialog, WM_COMMAND, IDCANCEL, 0);
        } else if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
    }
    if (IsWindow(dialog)) DestroyWindow(dialog);
    if (IsWindow(owner) && owner_was_enabled) EnableWindow(owner, TRUE);
    if (IsWindow(root_owner) && root_was_enabled) EnableWindow(root_owner, TRUE);
    if (IsWindow(owner)) SetForegroundWindow(owner);
    if (quit) PostQuitMessage(0);
    return state.accepted && IsWindow(owner) && !quit;
}
BOOL app_confirm(HWND owner, const wchar_t *details) { return confirm_dialog(owner, details, FALSE, FALSE); }
BOOL app_confirm_setup(HWND owner, const wchar_t *details, BOOL registry, BOOL restore) {
    return confirm_dialog(owner, details, registry, restore);
}

static BOOL confirmation_ui_test(HWND owner) {
    WNDCLASSW cls = {0}; cls.hInstance = g_instance; cls.lpfnWndProc = confirm_proc;
    cls.lpszClassName = L"WAIO.Confirm"; cls.hCursor = LoadCursorW(NULL, IDC_ARROW); RegisterClassW(&cls);
    Confirmation state = {0}; state.protect = TRUE; state.registry = TRUE; state.restore = TRUE;
    state.details = L"Read-only consent test";
    HWND window = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"", WS_CAPTION | WS_SYSMENU,
        0, 0, 812, 660, owner, NULL, g_instance, &state);
    if (!window) return FALSE;
    BOOL ok = state.registry && state.restore && LOWORD(SendMessageW(window, DM_GETDEFID, 0, 0)) == IDCANCEL;
    /* A failed preparatory job must leave the review open and unaccepted. */
    state.working = TRUE; state.error = ERROR_SERVICE_DISABLED;
    SendMessageW(window, WM_APP_JOB_DONE, 0, 0);
    ok = ok && IsWindow(window) && !state.working && !state.accepted;
    SendMessageW(window, WM_COMMAND, IDCANCEL, 0);
    return ok && !state.accepted && !state.working && !app_busy(NULL);
}

BOOL app_pick_folder(HWND owner, wchar_t *path, size_t capacity) {
    IFileOpenDialog *dialog = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                                  &IID_IFileOpenDialog, (void **)&dialog);
    if (FAILED(hr)) { app_error(owner, tr(L"Chọn thư mục", L"Folder picker"), (DWORD)hr); return FALSE; }
    DWORD flags = 0;
    IFileOpenDialog_GetOptions(dialog, &flags);
    IFileOpenDialog_SetOptions(dialog, flags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    hr = IFileOpenDialog_Show(dialog, owner);
    BOOL result = FALSE;
    if (SUCCEEDED(hr)) {
        IShellItem *item = NULL;
        hr = IFileOpenDialog_GetResult(dialog, &item);
        if (SUCCEEDED(hr)) {
            PWSTR name = NULL;
            hr = IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &name);
            if (SUCCEEDED(hr)) {
                result = SUCCEEDED(StringCchCopyW(path, capacity, name));
                CoTaskMemFree(name);
                if (!result) app_error(owner, tr(L"Đường dẫn thư mục", L"Folder path"), ERROR_FILENAME_EXCED_RANGE);
            }
            IShellItem_Release(item);
        }
    } else if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)) app_error(owner, tr(L"Chọn thư mục", L"Folder picker"), (DWORD)hr);
    IFileOpenDialog_Release(dialog);
    return result;
}

typedef struct { HWND owner; void (*work)(void *); void *context; } Work;
static DWORD WINAPI worker_proc(void *argument) {
    Work *work = argument;
    work->work(work->context);
    PostMessageW(work->owner, WM_APP_JOB_DONE, 0, 0);
    free(work);
    return 0;
}

BOOL app_start_job(HWND owner, void (*work)(void *), void *context) {
    if (smoke_mode) return FALSE;
    if (InterlockedCompareExchange(&running_job, 1, 0)) {
        MessageBoxW(owner, tr(L"Hãy chờ tác vụ hiện tại hoàn tất.", L"Please wait for the current task to finish."),
                    L"Win AIO Optimize", MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }
    Work *job = malloc(sizeof(*job));
    if (!job) { InterlockedExchange(&running_job, 0); app_error(owner, tr(L"Tác vụ nền", L"Worker"), ERROR_OUTOFMEMORY); return FALSE; }
    *job = (Work){owner, work, context};
    if (!SetPropW(owner, L"WAIOBusy", (HANDLE)1)) {
        free(job); InterlockedExchange(&running_job, 0); return FALSE;
    }
    EnableWindow(owner, FALSE);
    HANDLE thread = CreateThread(NULL, 0, worker_proc, job, 0, NULL);
    if (!thread) {
        DWORD error = GetLastError(); free(job); app_finish_job(owner); app_error(owner, tr(L"Tác vụ nền", L"Worker"), error); return FALSE;
    }
    CloseHandle(thread);
    if (progress_window) { ShowWindow(progress_window, SW_SHOW); SendMessageW(progress_window, PBM_SETMARQUEE, TRUE, 40); }
    return TRUE;
}

void app_finish_job(HWND owner) {
    RemovePropW(owner, L"WAIOBusy");
    InterlockedExchange(&running_job, 0);
    EnableWindow(owner, TRUE);
    if (progress_window) { SendMessageW(progress_window, PBM_SETMARQUEE, FALSE, 0); ShowWindow(progress_window, SW_HIDE); }
    SetForegroundWindow(owner);
}

BOOL app_busy(HWND owner) {
    return owner ? GetPropW(owner, L"WAIOBusy") != NULL : InterlockedCompareExchange(&running_job, 0, 0) != 0;
}

DWORD app_run_tool(const wchar_t *exe_name, const wchar_t *arguments) {
    if (smoke_mode) return ERROR_ACCESS_DENIED;
    if (!exe_name || !arguments) return ERROR_INVALID_PARAMETER;
    static const wchar_t *const allowed[] = {L"netsh.exe", L"ipconfig.exe", L"dism.exe", L"powercfg.exe", L"shutdown.exe"};
    BOOL found = FALSE;
    for (size_t i = 0; i < ARRAYSIZE(allowed); ++i) if (!_wcsicmp(exe_name, allowed[i])) found = TRUE;
    if (!found) return ERROR_ACCESS_DENIED;
    wchar_t path[MAX_PATH], command[32768];
    UINT length = GetSystemDirectoryW(path, ARRAYSIZE(path));
    if (!length || length >= ARRAYSIZE(path)) return ERROR_INSUFFICIENT_BUFFER;
    if (FAILED(StringCchCatW(path, ARRAYSIZE(path), L"\\")) ||
        FAILED(StringCchCatW(path, ARRAYSIZE(path), exe_name)) ||
        FAILED(StringCchPrintfW(command, ARRAYSIZE(command), L"\"%s\" %s", path, arguments))) return ERROR_INSUFFICIENT_BUFFER;
    SECURITY_ATTRIBUTES security = {sizeof(security), NULL, TRUE};
    HANDLE read_pipe = NULL, write_pipe = NULL, input = INVALID_HANDLE_VALUE;
    STARTUPINFOEXW start = {0};
    PROCESS_INFORMATION process = {0};
    DWORD code = ERROR_GEN_FAILURE;
    BOOL attributes_initialized = FALSE;
    if (!CreatePipe(&read_pipe, &write_pipe, &security, 0)) return GetLastError();
    if (!SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0)) { code = GetLastError(); goto cleanup; }
    input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, NULL);
    if (input == INVALID_HANDLE_VALUE) { code = GetLastError(); goto cleanup; }
    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &bytes);
    start.lpAttributeList = malloc(bytes);
    if (!start.lpAttributeList) { code = ERROR_OUTOFMEMORY; goto cleanup; }
    if (!InitializeProcThreadAttributeList(start.lpAttributeList, 1, 0, &bytes)) { code = GetLastError(); goto cleanup; }
    attributes_initialized = TRUE;
    HANDLE inherited[] = {write_pipe, input};
    if (!UpdateProcThreadAttribute(start.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   inherited, sizeof(inherited), NULL, NULL)) { code = GetLastError(); goto cleanup; }
    start.StartupInfo.cb = sizeof(start);
    start.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    start.StartupInfo.hStdInput = input;
    start.StartupInfo.hStdOutput = start.StartupInfo.hStdError = write_pipe;
    app_log(L"%s %s", exe_name, arguments);
    if (!CreateProcessW(path, command, NULL, NULL, TRUE, CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                        NULL, NULL, &start.StartupInfo, &process)) { code = GetLastError(); goto cleanup; }
    CloseHandle(write_pipe); write_pipe = NULL;
    char raw[2048];
    DWORD read = 0;
    while (ReadFile(read_pipe, raw, sizeof(raw), &read, NULL) && read) {
        wchar_t output[4096];
        int count = MultiByteToWideChar(CP_OEMCP, 0, raw, (int)read, output, ARRAYSIZE(output) - 1);
        if (count > 0) { output[count] = L'\0'; app_log(L"%s", output); }
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    if (!GetExitCodeProcess(process.hProcess, &code)) code = GetLastError();
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
cleanup:
    if (attributes_initialized) DeleteProcThreadAttributeList(start.lpAttributeList);
    free(start.lpAttributeList);
    if (read_pipe) CloseHandle(read_pipe);
    if (write_pipe) CloseHandle(write_pipe);
    if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
    app_log(tr(L"%s: mã thoát %lu", L"%s: exit code %lu"), exe_name, code);
    return code;
}

DWORD app_windows_build(void) {
    if (!windows_build) {
        typedef LONG (WINAPI *VersionFn)(OSVERSIONINFOW *);
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        VersionFn version = NULL;
        FARPROC address = ntdll ? GetProcAddress(ntdll, "RtlGetVersion") : NULL;
        memcpy(&version, &address, sizeof(version));
        OSVERSIONINFOW info = {0}; info.dwOSVersionInfoSize = sizeof(info);
        if (version && version(&info) == 0 && info.dwMajorVersion >= 10) windows_build = info.dwBuildNumber;
    }
    return windows_build;
}

static BOOL is_admin(void) {
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID group = NULL; BOOL result = FALSE;
    if (AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &group)) {
        CheckTokenMembership(NULL, group, &result); FreeSid(group);
    }
    return result;
}

static void update_labels(HWND window) {
    SetWindowTextW(window, L"Win AIO Optimize 6.0 — Doctoten");
    SetDlgItemTextW(window, ID_TITLE, tr(L"Tổng quan", L"Dashboard"));
    SetDlgItemTextW(window, ID_SUBTITLE, tr(L"Hiểu máy tính của bạn. Chủ động chọn từng thay đổi.", L"Know your computer. Choose every change."));
    SetDlgItemTextW(window, ID_BRAND, L"Win AIO\r\nOptimize");
    SetDlgItemTextW(window, ID_NAV_LABEL, tr(L"CÔNG CỤ  /  6.0", L"TOOLS  /  6.0"));
    SetDlgItemTextW(window, ID_SYSTEM_INFO, tr(L"Thông tin máy", L"System information"));
    SetDlgItemTextW(window, ID_DETAILS, tr(L"Xem đầy đủ thông tin", L"View all specifications"));
    SetDlgItemTextW(window, ID_PC_LABEL, tr(L"MÁY TÍNH CỦA BẠN", L"YOUR COMPUTER"));
    SetDlgItemTextW(window, ID_BACKUPS, tr(L"Mở thư mục sao lưu", L"Open backups"));
    SetDlgItemTextW(window, ID_SYSTEM_PROTECTION, tr(L"Bảo vệ hệ thống…", L"System Protection…"));
    SetDlgItemTextW(window, ID_PROTECTION_NOTE, tr(
        L"THIẾT LẬP WINDOWS\r\nChọn tối ưu theo từng nhóm và xem rõ ảnh hưởng trước khi áp dụng.\r\nCác lựa chọn sao lưu Registry và điểm khôi phục nằm trong Windows Setup.",
        L"WINDOWS SETUP\r\nChoose settings by category and review their effects before applying.\r\nRegistry backup and restore point options are available in Windows Setup."));
    wchar_t summary[768]; system_summary(summary, ARRAYSIZE(summary)); SetDlgItemTextW(window, ID_PC_SUMMARY, summary);
    HWND theme = GetDlgItem(window, ID_THEME);
    SendMessageW(theme, CB_RESETCONTENT, 0, 0);
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)tr(L"Theo Windows", L"Windows theme"));
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)tr(L"Sáng", L"Light"));
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)tr(L"Tối", L"Dark"));
    SendMessageW(theme, CB_SETCURSEL, g_theme.mode, 0);
    SetDlgItemTextW(window, ID_SETUP, tr(L"Thiết lập Windows", L"Windows Setup"));
    SetDlgItemTextW(window, ID_NETWORK, tr(L"Mạng & đổi DNS", L"Network & DNS"));
    SetDlgItemTextW(window, ID_BLOAT, tr(L"Gỡ ứng dụng cài sẵn", L"Remove bloatware"));
    SetDlgItemTextW(window, ID_WIFI, tr(L"Sao lưu WiFi", L"Back up WiFi"));
    SetDlgItemTextW(window, ID_RESTORE, tr(L"Khôi phục WiFi", L"Restore WiFi"));
    SetDlgItemTextW(window, ID_DRIVERS, tr(L"Sao lưu driver", L"Back up drivers"));
    SetDlgItemTextW(window, ID_TIMER_LABEL, tr(L"Hẹn giờ tắt máy", L"Shutdown timer"));
    SetDlgItemTextW(window, ID_SCHEDULE, tr(L"Đặt lịch…", L"Schedule…"));
    SetDlgItemTextW(window, ID_CANCEL, tr(L"Hủy lịch…", L"Cancel timer…"));
    SetDlgItemTextW(window, ID_LOG_LABEL, tr(L"Nhật ký hoạt động", L"Activity log"));
    SetDlgItemTextW(window, ID_OPEN_LOG, tr(L"Mở nhật ký", L"Open log"));
    HWND units = GetDlgItem(window, ID_UNIT);
    LRESULT selected = SendMessageW(units, CB_GETCURSEL, 0, 0);
    SendMessageW(units, CB_RESETCONTENT, 0, 0);
    SendMessageW(units, CB_ADDSTRING, 0, (LPARAM)tr(L"Phút", L"Minutes"));
    SendMessageW(units, CB_ADDSTRING, 0, (LPARAM)tr(L"Giờ", L"Hours"));
    SendMessageW(units, CB_SETCURSEL, selected == 1 ? 1 : 0, 0);
    wchar_t name[256] = L"?", account[512], status[512]; DWORD capacity = ARRAYSIZE(name);
    GetUserNameW(name, &capacity);
    StringCchPrintfW(account, ARRAYSIZE(account), tr(L"Tài khoản áp dụng: %s  •  Windows build %lu  •  Mọi tối ưu đều cần xác nhận",
                     L"Changes apply to account: %s  •  Windows build %lu  •  All optimizations require consent"), name, app_windows_build());
    SetDlgItemTextW(window, ID_ACCOUNT, account);
    if (smoke_mode) StringCchCopyW(status, ARRAYSIZE(status), tr(L"Kiểm tra giao diện chỉ đọc", L"Read-only UI smoke test"));
    else shutdown_status(status, ARRAYSIZE(status));
    SetWindowTextW(timer_status, status);
}

static void layout_main(HWND window) {
    RECT r; GetClientRect(window, &r);
    int width = r.right, height = r.bottom, x = 258, content = width - x - 28;
    MoveWindow(GetDlgItem(window, ID_BRAND), 24, 26, 185, 78, TRUE);
    MoveWindow(GetDlgItem(window, ID_NAV_LABEL), 24, 124, 186, 24, TRUE);
    int buttons[] = {ID_SYSTEM_INFO, ID_SETUP, ID_NETWORK, ID_BLOAT, ID_WIFI, ID_RESTORE, ID_DRIVERS};
    for (int i = 0; i < 7; ++i) MoveWindow(GetDlgItem(window, buttons[i]), 18, 166 + i * 52, 196, 42, TRUE);
    MoveWindow(GetDlgItem(window, ID_SYSTEM_PROTECTION), 18, height-116, 196, 40, TRUE);
    MoveWindow(GetDlgItem(window, ID_BACKUPS), 18, height-64, 196, 40, TRUE);
    MoveWindow(GetDlgItem(window, ID_TITLE), x, 24, content-360, 44, TRUE);
    MoveWindow(GetDlgItem(window, ID_THEME), width-366, 30, 160, 180, TRUE);
    MoveWindow(GetDlgItem(window, ID_LANGUAGE), width-194, 30, 166, 180, TRUE);
    MoveWindow(GetDlgItem(window, ID_SUBTITLE), x, 78, content, 28, TRUE);
    MoveWindow(GetDlgItem(window, ID_ACCOUNT), x, 112, content, 40, TRUE);
    MoveWindow(GetDlgItem(window, ID_PC_LABEL), x+20, 180, content-270, 26, TRUE);
    MoveWindow(GetDlgItem(window, ID_DETAILS), width-248, 171, 200, 36, TRUE);
    MoveWindow(GetDlgItem(window, ID_PC_SUMMARY), x+20, 220, content-40, 74, TRUE);
    MoveWindow(GetDlgItem(window, ID_PROTECTION_NOTE), x+20, 333, content-40, 68, TRUE);
    MoveWindow(GetDlgItem(window, ID_TIMER_LABEL), x+20, 442, content-40, 26, TRUE);
    MoveWindow(GetDlgItem(window, ID_DELAY), x+20, 480, 90, 32, TRUE);
    MoveWindow(GetDlgItem(window, ID_UNIT), x+122, 480, 116, 160, TRUE);
    MoveWindow(GetDlgItem(window, ID_SCHEDULE), x+254, 478, 154, 36, TRUE);
    MoveWindow(GetDlgItem(window, ID_CANCEL), x+420, 478, 154, 36, TRUE);
    MoveWindow(timer_status, x+20, 524, content-40, 26, TRUE);
    MoveWindow(GetDlgItem(window, ID_LOG_LABEL), x, 577, content-176, 28, TRUE);
    MoveWindow(GetDlgItem(window, ID_OPEN_LOG), width-178, 570, 150, 36, TRUE);
    MoveWindow(log_window, x, 618, content, height-646, TRUE);
    MoveWindow(progress_window, x, height-12, content, 5, TRUE);
    InvalidateRect(window, NULL, TRUE);
}

static void paint_dashboard(HWND window) {
    PAINTSTRUCT paint; HDC dc = BeginPaint(window, &paint);
    RECT r; GetClientRect(window, &r); FillRect(dc, &r, g_theme.background_brush);
    RECT side = {0, 0, 232, r.bottom}; FillRect(dc, &side, g_theme.surface_brush);
    HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
    HGDIOBJ old_pen = SelectObject(dc, pen), old_brush = SelectObject(dc, g_theme.surface_brush);
    MoveToEx(dc, 232, 0, NULL); LineTo(dc, 232, r.bottom);
    RoundRect(dc, 258, 158, r.right-28, 310, 16, 16);
    RoundRect(dc, 258, 322, r.right-28, 412, 16, 16);
    RoundRect(dc, 258, 426, r.right-28, 562, 16, 16);
    SelectObject(dc, old_pen); SelectObject(dc, old_brush); DeleteObject(pen); EndPaint(window, &paint);
}

static BOOL CALLBACK translate_owned(HWND window, LPARAM lp) {
    wchar_t cls[80] = L"";
    GetClassNameW(window, cls, ARRAYSIZE(cls));
    if (window != (HWND)lp && !wcsncmp(cls, L"WAIO", 4)) {
        SendMessageW(window, WM_APP_LANGUAGE, 0, 0);
        RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    return TRUE;
}

typedef struct { HWND root; BOOL active; } ModalSearch;
static BOOL CALLBACK find_owned_modal(HWND window, LPARAM argument) {
    ModalSearch *search = (ModalSearch *)argument;
    wchar_t cls[80] = L"";
    GetClassNameW(window, cls, ARRAYSIZE(cls));
    if (window != search->root && !IsWindowEnabled(window) &&
        (!wcsncmp(cls, L"WAIO", 4) || !wcscmp(cls, L"#32770"))) {
        search->active = TRUE;
        return FALSE;
    }
    return TRUE;
}

/* A common dialog may disable a secondary window while leaving the root enabled.
   Do not destroy its owner, change languages, or start another task in its nested loop. */
static BOOL owned_modal_active(HWND root) {
    if (!IsWindowEnabled(root)) return TRUE;
    ModalSearch search = {root, FALSE};
    EnumThreadWindows(GetCurrentThreadId(), find_owned_modal, (LPARAM)&search);
    return search.active;
}

static LRESULT CALLBACK main_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CREATE: {
        main_window = window;
        theme_attach(window);
        HWND heading = app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_TITLE);
        SendMessageW(heading, WM_SETFONT, (WPARAM)title_font, TRUE);
        HWND brand = app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_BRAND);
        SendMessageW(brand, WM_SETFONT, (WPARAM)title_font, TRUE); SetPropW(brand, L"WAIOCard", (HANDLE)1);
        int labels[] = {ID_PC_LABEL, ID_PC_SUMMARY, ID_NAV_LABEL, ID_PROTECTION_NOTE};
        for (unsigned i = 0; i < ARRAYSIZE(labels); ++i)
            SetPropW(app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, labels[i]), L"WAIOCard", (HANDLE)1);
        int extra[] = {ID_SYSTEM_INFO, ID_BACKUPS, ID_SYSTEM_PROTECTION, ID_DETAILS};
        for (unsigned i = 0; i < ARRAYSIZE(extra); ++i)
            SetPropW(app_control(window, L"BUTTON", L"", WS_TABSTOP, 0, 0, 0, 0, extra[i]), L"WAIOCard", (HANDLE)1);
        app_control(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 0, 0, 0, 0, ID_THEME);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_SUBTITLE);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_ACCOUNT);
        int buttons[] = {ID_SETUP, ID_NETWORK, ID_BLOAT, ID_WIFI, ID_RESTORE, ID_DRIVERS};
        for (int i = 0; i < 6; ++i) SetPropW(app_control(window, L"BUTTON", L"", WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, buttons[i]), L"WAIOCard", (HANDLE)1);
        HWND language = app_control(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 0, 0, 0, 0, ID_LANGUAGE);
        SendMessageW(language, CB_ADDSTRING, 0, (LPARAM)L"Tiếng Việt");
        SendMessageW(language, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(language, CB_SETCURSEL, g_vi ? 0 : 1, 0);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_TIMER_LABEL);
        HWND delay = app_control(window, L"EDIT", L"30", WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL, 0, 0, 0, 0, ID_DELAY);
        SendMessageW(delay, EM_SETLIMITTEXT, 7, 0);
        app_control(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 0, 0, 0, 0, ID_UNIT);
        app_control(window, L"BUTTON", L"", WS_TABSTOP, 0, 0, 0, 0, ID_SCHEDULE);
        app_control(window, L"BUTTON", L"", WS_TABSTOP, 0, 0, 0, 0, ID_CANCEL);
        timer_status = app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, 0);
        SetPropW(timer_status, L"WAIOCard", (HANDLE)1);
        int card[] = {ID_TIMER_LABEL, ID_DELAY, ID_UNIT, ID_SCHEDULE, ID_CANCEL};
        for (unsigned i = 0; i < ARRAYSIZE(card); ++i) SetPropW(GetDlgItem(window, card[i]), L"WAIOCard", (HANDLE)1);
        app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_LOG_LABEL);
        app_control(window, L"BUTTON", L"", WS_TABSTOP, 0, 0, 0, 0, ID_OPEN_LOG);
        log_window = app_control(window, L"EDIT", L"", WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                 0, 0, 0, 0, 0);
        SendMessageW(log_window, EM_SETLIMITTEXT, 250000, 0);
        progress_window = app_control(window, PROGRESS_CLASSW, L"", PBS_MARQUEE, 0, 0, 0, 0, 0);
        ShowWindow(progress_window, SW_HIDE);
        update_labels(window); layout_main(window);
        if (!smoke_mode) SetTimer(window, 1, 1000, NULL);
        app_log(L"Win AIO Optimize %s. %s", APP_VERSION, tr(L"Sẵn sàng. Chưa áp dụng thay đổi nào.", L"Ready. No changes have been applied."));
        return 0;
    }
    case WM_COMMAND:
        if (smoke_mode && LOWORD(wp) != ID_THEME && LOWORD(wp) != ID_LANGUAGE) return 0;
        if (owned_modal_active(window)) {
            if (LOWORD(wp) == ID_LANGUAGE && HIWORD(wp) == CBN_SELCHANGE)
                SendDlgItemMessageW(window, ID_LANGUAGE, CB_SETCURSEL, g_vi ? 0 : 1, 0);
            return 0;
        }
        switch (LOWORD(wp)) {
        case ID_SYSTEM_INFO:
        case ID_DETAILS: system_info_show(window); break;
        case ID_THEME:
            if (HIWORD(wp) == CBN_SELCHANGE) theme_set((int)SendDlgItemMessageW(window, ID_THEME, CB_GETCURSEL, 0, 0), !smoke_mode);
            break;
        case ID_BACKUPS: {
            wchar_t path[MAX_PATH];
            if (app_data_path(path, ARRAYSIZE(path), NULL)) ShellExecuteW(window, L"open", path, NULL, NULL, SW_SHOWNORMAL);
            break;
        }
        case ID_SYSTEM_PROTECTION: {
            wchar_t path[MAX_PATH];
            UINT n = GetSystemDirectoryW(path, ARRAYSIZE(path));
            if (n && n < ARRAYSIZE(path) && SUCCEEDED(StringCchCatW(path, ARRAYSIZE(path), L"\\SystemPropertiesProtection.exe")))
                if ((INT_PTR)ShellExecuteW(window, L"open", path, NULL, NULL, SW_SHOWNORMAL) <= 32)
                    app_error(window, L"System Protection", ERROR_OPEN_FAILED);
            break;
        }
        case ID_SETUP: optimizer_show(window); break;
        case ID_NETWORK: network_show(window); break;
        case ID_BLOAT: bloatware_show(window); break;
        case ID_WIFI: backup_wifi(window); break;
        case ID_RESTORE: restore_wifi(window); break;
        case ID_DRIVERS: backup_drivers(window); break;
        case ID_SCHEDULE: {
            wchar_t value[32]; GetDlgItemTextW(window, ID_DELAY, value, ARRAYSIZE(value));
            shutdown_schedule(window, value, SendDlgItemMessageW(window, ID_UNIT, CB_GETCURSEL, 0, 0) == 1); break;
        }
        case ID_CANCEL: shutdown_cancel(window); break;
        case ID_OPEN_LOG:
            if ((INT_PTR)ShellExecuteW(window, L"open", log_path, NULL, NULL, SW_SHOWNORMAL) <= 32)
                app_error(window, tr(L"Mở nhật ký", L"Open log"), ERROR_OPEN_FAILED);
            break;
        case ID_LANGUAGE:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                if (app_busy(NULL)) {
                    SendDlgItemMessageW(window, ID_LANGUAGE, CB_SETCURSEL, g_vi ? 0 : 1, 0);
                    break;
                }
                g_vi = SendDlgItemMessageW(window, ID_LANGUAGE, CB_GETCURSEL, 0, 0) == 0;
                language_thread();
                update_labels(window);
                EnumThreadWindows(GetCurrentThreadId(), translate_owned, (LPARAM)window);
                RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                language_save(window);
            }
            break;
        }
        return 0;
    case WM_APP_LOG: {
        wchar_t *text = (wchar_t *)lp;
        int length = GetWindowTextLengthW(log_window);
        if (length > 200000) { SetWindowTextW(log_window, L""); length = 0; }
        SendMessageW(log_window, EM_SETSEL, length, length);
        SendMessageW(log_window, EM_REPLACESEL, FALSE, (LPARAM)text);
        SendMessageW(log_window, EM_SCROLLCARET, 0, 0);
        free(text); return 0;
    }
    case WM_APP_JOB_DONE: app_finish_job(window); return 0;
    case WM_TIMER: {
        if (wp == 2) { PostMessageW(window, WM_CLOSE, 0, 0); return 0; }
        if (smoke_mode) return 0;
        wchar_t status[512]; shutdown_status(status, ARRAYSIZE(status));
        SetWindowTextW(timer_status, status);
        static unsigned ticks;
        if (++ticks % 30 == 0) { wchar_t summary[768]; system_summary(summary, ARRAYSIZE(summary)); SetDlgItemTextW(window, ID_PC_SUMMARY, summary); }
        return 0;
    }
    case WM_SIZE: if (log_window) layout_main(window); return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = 1040; ((MINMAXINFO *)lp)->ptMinTrackSize.y = 740; return 0;
    case WM_PAINT: paint_dashboard(window); return 0;
    case WM_CLOSE:
        if (app_busy(NULL)) {
            MessageBoxW(window, tr(L"Hãy chờ tác vụ hoàn tất trước khi đóng.", L"Wait for the current task to finish before closing."),
                        L"Win AIO Optimize", MB_OK | MB_ICONINFORMATION); return 0;
        }
        if (owned_modal_active(window)) return 0;
        DestroyWindow(window); return 0;
    case WM_DESTROY: KillTimer(window, 1); main_window = NULL; PostQuitMessage(smoke_failed ? 1 : 0); return 0;
    }
    return DefWindowProcW(window, message, wp, lp);
}

static BOOL elevation_arguments(wchar_t *text, size_t capacity, BOOL vietnamese, int theme) {
    const wchar_t *appearance = theme == 1 ? L" --theme=light" : theme == 2 ? L" --theme=dark" : L"";
    return SUCCEEDED(StringCchPrintfW(text, capacity, L"--lang=%s%s", vietnamese ? L"vi" : L"en", appearance));
}

static int self_test(void) {
    unsigned checks = 0, failed = 0; DWORD seconds = 0;
    wchar_t arguments[64];
#define CHECK(x) do { ++checks; if (!(x)) { ++failed; fprintf(stderr, "FAIL: %s\n", #x); } } while (0)
    CHECK(backup_valid_delay(L"1", FALSE, &seconds) && seconds == 60);
    CHECK(backup_valid_delay(L"2", TRUE, &seconds) && seconds == 7200);
    CHECK(!backup_valid_delay(L"0", FALSE, &seconds));
    CHECK(!backup_valid_delay(L"-1", FALSE, &seconds));
    CHECK(!backup_valid_delay(L"1.5", FALSE, &seconds));
    CHECK(!backup_valid_delay(L"1 & whoami", FALSE, &seconds));
    CHECK(!backup_valid_delay(L"99999999999999999999999", TRUE, &seconds));
    CHECK(!backup_valid_delay(L"", FALSE, &seconds));
    CHECK(backup_valid_delay(L"10080", FALSE, &seconds) && seconds == 604800);
    CHECK(backup_valid_delay(L"168", TRUE, &seconds) && seconds == 604800);
    CHECK(!backup_valid_delay(L"10081", FALSE, &seconds));
    CHECK(!backup_valid_delay(L"169", TRUE, &seconds));
    CHECK(!backup_valid_delay(L" 2", TRUE, &seconds));
    CHECK(!backup_valid_delay(NULL, TRUE, &seconds));
    CHECK(!backup_valid_delay(L"1", FALSE, NULL));
    CHECK(network_valid_dns(L"1.1.1.1", 2));
    CHECK(network_valid_dns(L"2606:4700:4700::1111", 23));
    CHECK(!network_valid_dns(L"300.1.1.1", 2));
    CHECK(!network_valid_dns(L"1.1.1.1 & calc", 2));
    CHECK(!network_valid_dns(L"", 2));
    CHECK(!network_valid_dns(L"2001:::1", 23));
    CHECK(network_self_test());
    CHECK(optimizer_self_test());
    CHECK(protection_self_test());
    CHECK(system_info_self_test());
    CHECK(elevation_arguments(arguments, ARRAYSIZE(arguments), FALSE, 1) && !wcscmp(arguments, L"--lang=en --theme=light"));
    CHECK(elevation_arguments(arguments, ARRAYSIZE(arguments), TRUE, 2) && !wcscmp(arguments, L"--lang=vi --theme=dark"));
    CHECK(elevation_arguments(arguments, ARRAYSIZE(arguments), TRUE, -1) && !wcscmp(arguments, L"--lang=vi"));
    CHECK(!elevation_arguments(arguments, 4, FALSE, 1));
    CHECK(app_run_tool(L"cmd.exe", L"/c exit 0") == ERROR_ACCESS_DENIED);
    CHECK(app_run_tool(L"..\\netsh.exe", L"") == ERROR_ACCESS_DENIED);
    CHECK(app_run_tool(L"netsh.exe", L"interface ipv4 reset") == ERROR_ACCESS_DENIED);
    CHECK(protection_prepare(TRUE, TRUE) == ERROR_ACCESS_DENIED);
#undef CHECK
    printf("Read-only self-test: %u checks, %u failures. No system settings changed.\n", checks, failed);
    return failed ? 1 : 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command, int show) {
    (void)previous; (void)command;
    g_instance = instance;
    int argc = 0; wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    BOOL test = FALSE, smoke = FALSE, inventory = FALSE, preview = FALSE, preview_setup = FALSE;
    int language_override = -1;
    int theme_override = -1;
    for (int i = 1; argv && i < argc; ++i) {
        if (!wcscmp(argv[i], L"--self-test")) test = TRUE;
        else if (!wcscmp(argv[i], L"--smoke-test")) smoke = TRUE;
        else if (!wcscmp(argv[i], L"--preview")) { preview = TRUE; smoke = TRUE; }
        else if (!wcscmp(argv[i], L"--preview=setup")) { preview = TRUE; smoke = TRUE; preview_setup = TRUE; }
        else if (!wcscmp(argv[i], L"--inventory-test")) inventory = TRUE;
        else if (!wcscmp(argv[i], L"--theme=light")) theme_override = 1;
        else if (!wcscmp(argv[i], L"--theme=dark")) theme_override = 2;
        else if (!wcscmp(argv[i], L"--english") || !wcscmp(argv[i], L"--lang=en")) language_override = 0;
        else if (!wcscmp(argv[i], L"--lang=vi")) language_override = 1;
        else { LocalFree(argv); return ERROR_INVALID_PARAMETER; }
    }
    if (argv) LocalFree(argv);
    if (!test && !smoke && !inventory) language_load();
    if (language_override >= 0) g_vi = language_override != 0;
    language_thread();
    /* Every diagnostic entry point uses the same side-effect guards. */
    smoke_mode = test || smoke || inventory;
    if (test) return self_test();
    if (inventory) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr)) return 1;
        hr = protection_com_security();
        int code = FAILED(hr) ? 1 : system_info_inventory_test();
        CoUninitialize(); return code;
    }
    if (app_windows_build() < 10240) {
        MessageBoxW(NULL, tr(L"Ứng dụng yêu cầu Windows 10 hoặc Windows 11.", L"Windows 10 or Windows 11 is required."), L"Win AIO Optimize", MB_OK | MB_ICONERROR); return 1;
    }
    if (!smoke && !is_admin()) {
        wchar_t path[MAX_PATH];
        DWORD length = GetModuleFileNameW(NULL, path, ARRAYSIZE(path));
        if (!length || length >= ARRAYSIZE(path)) return ERROR_FILENAME_EXCED_RANGE;
        wchar_t parameters[64];
        if (!elevation_arguments(parameters, ARRAYSIZE(parameters), g_vi, theme_override)) return ERROR_INSUFFICIENT_BUFFER;
        SHELLEXECUTEINFOW execute = {0}; execute.cbSize = sizeof(execute);
        execute.lpVerb = L"runas"; execute.lpFile = path;
        execute.lpParameters = parameters; execute.nShow = SW_SHOWNORMAL;
        if (!ShellExecuteExW(&execute)) { app_error(NULL, tr(L"Cần quyền quản trị", L"Administrator rights are required"), GetLastError()); return 1; }
        return 0;
    }
    HANDLE mutex = CreateMutexW(NULL, TRUE, smoke ? NULL : L"Local\\Doctoten.WinAIOOptimize.6");
    if (!mutex) return (int)GetLastError();
    if (!smoke && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"WAIO.Main", NULL);
        if (existing) { ShowWindow(existing, SW_RESTORE); SetForegroundWindow(existing); }
        CloseHandle(mutex); return 0;
    }
    HRESULT com = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com)) { CloseHandle(mutex); return 1; }
    HRESULT security = protection_com_security();
    if (FAILED(security)) {
        app_error(NULL, tr(L"Khởi tạo bảo mật COM", L"Initialize COM security"), (DWORD)security);
        CoUninitialize(); CloseHandle(mutex); return 1;
    }
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&controls);
    /* Manifest GDI scaling keeps fixed-layout utility dialogs readable at high DPI. */
    InitializeCriticalSection(&log_lock);
    if (!smoke) {
        log_ready = app_data_path(log_path, ARRAYSIZE(log_path), L"activity.log");
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (log_ready && GetFileAttributesExW(log_path, GetFileExInfoStandard, &info) &&
            (info.nFileSizeHigh || info.nFileSizeLow > 4 * 1024 * 1024)) {
            wchar_t old[MAX_PATH];
            if (app_data_path(old, ARRAYSIZE(old), L"activity.previous.log")) MoveFileExW(log_path, old, MOVEFILE_REPLACE_EXISTING);
        }
    }
    g_font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    title_font = CreateFontW(-30, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    theme_init();
    if (theme_override >= 0) theme_set(theme_override, FALSE);
    WNDCLASSW cls = {0}; cls.lpfnWndProc = main_proc; cls.hInstance = instance;
    cls.hCursor = LoadCursorW(NULL, IDC_ARROW); cls.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    cls.lpszClassName = L"WAIO.Main";
    RegisterClassW(&cls);
    HWND window = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"Win AIO Optimize", WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 1180, 840, NULL, NULL, instance, NULL);
    int result = 1;
    if (window) {
        ShowWindow(window, smoke && !preview ? SW_HIDE : show); UpdateWindow(window);
        if (preview) SetTimer(window, 2, 60000, NULL);
        if (preview_setup) optimizer_preview(window);
        if (smoke && !preview) {
            BOOL original = g_vi, ok = TRUE;
            SetDlgItemTextW(window, ID_DELAY, L"42");
            SendDlgItemMessageW(window, ID_UNIT, CB_SETCURSEL, 1, 0);
            for (int lang = 0; lang < 2; ++lang) {
                wchar_t text[80]; g_vi = lang != 0; update_labels(window);
                GetDlgItemTextW(window, ID_SETUP, text, ARRAYSIZE(text));
                if (wcscmp(text, tr(L"Thiết lập Windows", L"Windows Setup"))) ok = FALSE;
                GetDlgItemTextW(window, ID_DELAY, text, ARRAYSIZE(text));
                if (wcscmp(text, L"42") || SendDlgItemMessageW(window, ID_UNIT, CB_GETCURSEL, 0, 0) != 1) ok = FALSE;
            }
            printf("Main window translation/state: %s\n", ok ? "PASS" : "FAIL"); fflush(stdout);
            BOOL network_ok = network_ui_test(window);
            printf("Network translation/state: %s\n", network_ok ? "PASS" : "FAIL"); fflush(stdout);
            BOOL optimizer_ok = optimizer_ui_test(window);
            printf("Optimizer translation/state: %s\n", optimizer_ok ? "PASS" : "FAIL"); fflush(stdout);
            BOOL info_ok = system_info_ui_test(window);
            printf("System information translation/state: %s\n", info_ok ? "PASS" : "FAIL"); fflush(stdout);
            BOOL consent_ok = confirmation_ui_test(window);
            printf("Recovery consent failure/cancel: %s\n", consent_ok ? "PASS" : "FAIL"); fflush(stdout);
            ok = ok && network_ok && optimizer_ok && info_ok && consent_ok && !smoke_failed;
            g_vi = original; smoke_failed = !ok;
            printf("Read-only UI/language test: %s. No settings changed.\n", ok ? "PASS" : "FAIL");
            PostMessageW(window, WM_CLOSE, 0, 0);
        }
        MSG msg; int status;
        while ((status = (int)GetMessageW(&msg, NULL, 0, 0)) > 0) {
            HWND root = GetAncestor(msg.hwnd, GA_ROOT);
            if (!root || !IsDialogMessageW(root, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        }
        result = status < 0 ? 1 : (int)msg.wParam;
    }
    theme_destroy(); DeleteObject(title_font); DeleteObject(g_font);
    log_ready = FALSE; DeleteCriticalSection(&log_lock); CoUninitialize();
    ReleaseMutex(mutex); CloseHandle(mutex);
    return result;
}
