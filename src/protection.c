#include "app.h"
#include <aclapi.h>
#include <sddl.h>
#include <srrestoreptapi.h>
#include <stdio.h>

/* Process-wide COM security, before any proxy is created. Local system services
   need callback access for SRSetRestorePoint; no remote callers are granted access. */
HRESULT protection_com_security(void) {
    const WELL_KNOWN_SID_TYPE kinds[] = {WinBuiltinAdministratorsSid, WinLocalServiceSid,
        WinNetworkServiceSid, WinSelfSid, WinLocalSystemSid};
    union { ULONGLONG align; BYTE bytes[SECURITY_MAX_SID_SIZE]; } sids[5];
    EXPLICIT_ACCESSW entries[5] = {0};
    SECURITY_DESCRIPTOR descriptor;
    PACL acl = NULL;
    for (unsigned i = 0; i < ARRAYSIZE(kinds); ++i) {
        DWORD size = sizeof(sids[i]);
        if (!CreateWellKnownSid(kinds[i], NULL, sids[i].bytes, &size)) return HRESULT_FROM_WIN32(GetLastError());
        entries[i].grfAccessPermissions = COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL;
        entries[i].grfAccessMode = SET_ACCESS;
        entries[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        entries[i].Trustee.ptstrName = (LPWSTR)sids[i].bytes;
    }
    DWORD error = SetEntriesInAclW(ARRAYSIZE(entries), entries, NULL, &acl);
    if (error) return HRESULT_FROM_WIN32(error);
    InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorOwner(&descriptor, sids[0].bytes, FALSE);
    SetSecurityDescriptorGroup(&descriptor, sids[0].bytes, FALSE);
    SetSecurityDescriptorDacl(&descriptor, TRUE, acl, FALSE);
    HRESULT hr = CoInitializeSecurity(&descriptor, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_DISABLE_AAA | EOAC_NO_CUSTOM_MARSHAL, NULL);
    LocalFree(acl);
    return hr;
}

static DWORD registry_backup(void) {
    if (app_is_smoke_test()) return ERROR_ACCESS_DENIED;
    wchar_t base[MAX_PATH], folder[MAX_PATH], path[MAX_PATH], guid[40];
    GUID id;
    if (!app_data_path(base, ARRAYSIZE(base), NULL)) return ERROR_PATH_NOT_FOUND;
    wchar_t volume[MAX_PATH];
    DWORD flags = 0;
    if (!GetVolumePathNameW(base, volume, ARRAYSIZE(volume)) ||
        !GetVolumeInformationW(volume, NULL, 0, NULL, NULL, &flags, NULL, 0)) return GetLastError();
    if (!(flags & FILE_PERSISTENT_ACLS)) return ERROR_NOT_SUPPORTED;
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(&id, guid, ARRAYSIZE(guid))) return ERROR_GEN_FAILURE;
    if (FAILED(StringCchPrintfW(folder, ARRAYSIZE(folder), L"%s\\Registry-%s", base, guid))) return ERROR_FILENAME_EXCED_RANGE;
    PSECURITY_DESCRIPTOR sd = NULL;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;FA;;;OW)", SDDL_REVISION_1, &sd, NULL)) return GetLastError();
    SECURITY_ATTRIBUTES security = {sizeof(security), sd, FALSE};
    BOOL created = CreateDirectoryW(folder, &security);
    DWORD error = created ? ERROR_SUCCESS : GetLastError();
    LocalFree(sd);
    if (error) return error;
    HANDLE token = NULL;
    TOKEN_PRIVILEGES request = {0}, previous = {0};
    DWORD size = sizeof(previous);
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return GetLastError();
    request.PrivilegeCount = 1;
    if (!LookupPrivilegeValueW(NULL, SE_BACKUP_NAME, &request.Privileges[0].Luid)) {
        error = GetLastError(); CloseHandle(token); return error;
    }
    request.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    BOOL enabled = AdjustTokenPrivileges(token, FALSE, &request, sizeof(previous), &previous, &size);
    error = GetLastError();
    if (!enabled || error) { CloseHandle(token); return error ? error : ERROR_PRIVILEGE_NOT_HELD; }
    const struct { HKEY root; const wchar_t *key, *name; } hives[] = {
        {HKEY_LOCAL_MACHINE, L"SYSTEM", L"HKLM-SYSTEM.hiv"},
        {HKEY_LOCAL_MACHINE, L"SOFTWARE", L"HKLM-SOFTWARE.hiv"},
        {HKEY_CURRENT_USER, L"", L"HKCU.hiv"}
    };
    app_log(tr(L"Đang sao lưu Registry: %s", L"Creating registry backup: %s"), folder);
    for (unsigned i = 0; !error && i < ARRAYSIZE(hives); ++i) {
        HKEY key;
        error = (DWORD)RegOpenKeyExW(hives[i].root, hives[i].key, 0, KEY_READ | KEY_WOW64_64KEY, &key);
        if (error) break;
        if (FAILED(StringCchPrintfW(path, ARRAYSIZE(path), L"%s\\%s", folder, hives[i].name))) error = ERROR_FILENAME_EXCED_RANGE;
        else {
            error = (DWORD)RegSaveKeyExW(key, path, NULL, REG_LATEST_FORMAT);
            if (error && error != ERROR_ALREADY_EXISTS && error != ERROR_FILE_EXISTS)
                DeleteFileW(path); /* Only our incomplete file, never a pre-existing destination. */
        }
        RegCloseKey(key);
    }
    AdjustTokenPrivileges(token, FALSE, &previous, 0, NULL, NULL);
    CloseHandle(token);
    if (error) {
        app_log(tr(L"Sao lưu Registry chưa hoàn tất (mã %lu). Dữ liệu một phần: %s. Chưa áp dụng thay đổi.",
            L"Registry backup incomplete (code %lu). Partial files: %s. No changes applied."), error, folder);
        return error;
    }
    /* A completion marker distinguishes a complete backup from partial output. */
    if (FAILED(StringCchPrintfW(path, ARRAYSIZE(path), L"%s\\BACKUP-COMPLETE.txt", folder)))
        return ERROR_FILENAME_EXCED_RANGE;
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return GetLastError();
    const char note[] = "Win AIO Optimize registry backup\r\n"
        "Contains HKLM SYSTEM, HKLM SOFTWARE and the running account's HKCU.\r\n"
        "Binary .hiv files; not .reg files and not a full system-state backup.\r\n"
        "Other users, SAM/SECURITY, volatile keys and files are not included.\r\n"
        "Keep private. For app changes use Restore last settings first.\r\n"
        "Full hive recovery requires an experienced administrator and usually Windows Recovery Environment.\r\n"
        "Do not import or replace live system hives blindly. No automatic hive restore is performed.\r\n";
    DWORD written = 0;
    if (!WriteFile(file, note, sizeof(note)-1, &written, NULL) || written != sizeof(note)-1 || !FlushFileBuffers(file))
        error = ERROR_WRITE_FAULT;
    CloseHandle(file);
    if (error) { DeleteFileW(path); return error; }
    app_log(tr(L"Đã sao lưu Registry (SYSTEM, SOFTWARE, HKCU): %s", L"Registry backup saved (SYSTEM, SOFTWARE, HKCU): %s"), folder);
    return ERROR_SUCCESS;
}

static DWORD restore_point(void) {
    if (app_is_smoke_test()) return ERROR_ACCESS_DENIED;
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return (DWORD)hr;
    HMODULE library = LoadLibraryExW(L"srclient.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    DWORD error = library ? ERROR_SUCCESS : GetLastError();
    typedef BOOL (WINAPI *RestoreFn)(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
    RestoreFn set_restore = NULL;
    if (library) {
        FARPROC address = GetProcAddress(library, "SRSetRestorePointW");
        memcpy(&set_restore, &address, sizeof(set_restore));
        if (!set_restore) error = ERROR_PROC_NOT_FOUND;
    }
    if (!error) {
        RESTOREPOINTINFOW point = {0};
        STATEMGRSTATUS status = {0};
        point.dwEventType = BEGIN_SYSTEM_CHANGE;
        point.dwRestorePtType = MODIFY_SETTINGS;
        StringCchCopyW(point.szDescription, ARRAYSIZE(point.szDescription), L"Win AIO Optimize - before system changes");
        app_log(tr(L"Đang yêu cầu điểm khôi phục Windows…", L"Requesting a Windows restore point…"));
        if (!set_restore(&point, &status) || status.nStatus) error = status.nStatus ? status.nStatus : ERROR_GEN_FAILURE;
        else {
            point.dwEventType = END_SYSTEM_CHANGE;
            point.llSequenceNumber = status.llSequenceNumber;
            if (!set_restore(&point, &status) || status.nStatus) error = status.nStatus ? status.nStatus : ERROR_GEN_FAILURE;
            else if (!point.llSequenceNumber) error = ERROR_INVALID_DATA;
            else app_log(tr(L"Windows xác nhận điểm khôi phục #%lld. Windows có thể dùng lại điểm gần đây theo giới hạn tần suất.",
                L"Windows confirmed restore point #%lld. Windows may reuse a recent point under its frequency limit."), point.llSequenceNumber);
        }
    }
    if (library) FreeLibrary(library);
    CoUninitialize();
    return error;
}

typedef DWORD (*ProtectionStep)(void);
static DWORD prepare(BOOL registry, BOOL restore, ProtectionStep backup_step, ProtectionStep restore_step) {
    DWORD error = registry ? backup_step() : ERROR_SUCCESS;
    if (!error && restore) error = restore_step();
    return error;
}
DWORD protection_prepare(BOOL registry, BOOL restore) {
    if (app_is_smoke_test()) return ERROR_ACCESS_DENIED;
    return prepare(registry, restore, registry_backup, restore_point);
}

static unsigned test_calls;
static DWORD test_backup_ok(void) { test_calls = test_calls * 10 + 1; return 0; }
static DWORD test_backup_fail(void) { test_calls = test_calls * 10 + 1; return ERROR_ACCESS_DENIED; }
static DWORD test_restore_ok(void) { test_calls = test_calls * 10 + 2; return 0; }
static DWORD test_restore_fail(void) { test_calls = test_calls * 10 + 2; return ERROR_SERVICE_DISABLED; }
BOOL protection_self_test(void) {
    test_calls = 0;
    if (prepare(TRUE, TRUE, test_backup_fail, test_restore_ok) != ERROR_ACCESS_DENIED || test_calls != 1) return FALSE;
    test_calls = 0;
    if (prepare(TRUE, TRUE, test_backup_ok, test_restore_fail) != ERROR_SERVICE_DISABLED || test_calls != 12) return FALSE;
    test_calls = 0;
    if (prepare(TRUE, TRUE, test_backup_ok, test_restore_ok) || test_calls != 12) return FALSE;
    test_calls = 0;
    if (prepare(FALSE, TRUE, test_backup_ok, test_restore_ok) || test_calls != 2) return FALSE;
    test_calls = 0;
    return !prepare(FALSE, FALSE, test_backup_fail, test_restore_fail) && !test_calls;
}
