#include "packages.h"
#include <appmodel.h>
#include <roapi.h>
#include <winstring.h>
#include <asyncinfo.h>
#include <windows.management.deployment.h>

typedef __x_ABI_CWindows_CManagement_CDeployment_CIPackageManager NativePackageManager;
typedef __x_ABI_CWindows_CManagement_CDeployment_CIDeploymentResult NativeDeploymentResult;
#ifdef __MINGW32__
typedef __FIAsyncOperationWithProgress_2_Windows__CManagement__CDeployment__CDeploymentResult_DeploymentProgress NativeDeploymentOperation;
#else
typedef __FIAsyncOperationWithProgress_2_Windows__CManagement__CDeployment__CDeploymentResult_Windows__CManagement__CDeployment__CDeploymentProgress NativeDeploymentOperation;
#endif
static const GUID package_manager_iid = {0x9a7d4b65,0x5e8f,0x4fc7,{0xa2,0xe5,0x7f,0x69,0x25,0xcb,0x8b,0x53}};
static const GUID async_info_iid = {0x36,0,0,{0xc0,0,0,0,0,0,0,0x46}};
typedef struct { const wchar_t *family, *name; } OptionalPackage;
static const OptionalPackage optional_packages[] = {
    {L"Microsoft.Copilot_8wekyb3d8bbwe", L"Microsoft Copilot"},
    {L"Microsoft.549981C3F5F10_8wekyb3d8bbwe", L"Cortana"},
    {L"Clipchamp.Clipchamp_yxz26nhyzhsrt", L"Clipchamp"},
    {L"Microsoft.BingNews_8wekyb3d8bbwe", L"Microsoft News"},
    {L"Microsoft.BingWeather_8wekyb3d8bbwe", L"Microsoft Weather"},
    {L"Microsoft.BingFinance_8wekyb3d8bbwe", L"Microsoft Money"},
    {L"Microsoft.BingSports_8wekyb3d8bbwe", L"Microsoft Sports"},
    {L"Microsoft.GetHelp_8wekyb3d8bbwe", L"Get Help"},
    {L"Microsoft.Getstarted_8wekyb3d8bbwe", L"Tips / Get Started"},
    {L"Microsoft.MicrosoftSolitaireCollection_8wekyb3d8bbwe", L"Microsoft Solitaire Collection"},
    {L"Microsoft.MicrosoftOfficeHub_8wekyb3d8bbwe", L"Microsoft 365 / Office Hub"},
    {L"Microsoft.People_8wekyb3d8bbwe", L"Microsoft People"},
    {L"Microsoft.SkypeApp_kzf8qxf38zg5c", L"Skype"},
    {L"Microsoft.WindowsFeedbackHub_8wekyb3d8bbwe", L"Feedback Hub"},
    {L"Microsoft.WindowsMaps_8wekyb3d8bbwe", L"Windows Maps"},
    {L"Microsoft.XboxApp_8wekyb3d8bbwe", L"Xbox Console Companion"},
    {L"Microsoft.GamingApp_8wekyb3d8bbwe", L"Xbox app (game management)"},
    {L"Microsoft.ZuneMusic_8wekyb3d8bbwe", L"Media Player / Groove Music"},
    {L"Microsoft.ZuneVideo_8wekyb3d8bbwe", L"Movies & TV"},
    {L"Microsoft.MixedReality.Portal_8wekyb3d8bbwe", L"Mixed Reality Portal"},
    {L"Microsoft.Microsoft3DViewer_8wekyb3d8bbwe", L"3D Viewer"},
    {L"Microsoft.MSPaint_8wekyb3d8bbwe", L"Paint 3D (not Windows Paint)"},
    {L"Microsoft.MicrosoftStickyNotes_8wekyb3d8bbwe", L"Sticky Notes (back up your notes first)"},
    {L"Microsoft.YourPhone_8wekyb3d8bbwe", L"Phone Link"},
    {L"Microsoft.OutlookForWindows_8wekyb3d8bbwe", L"Outlook for Windows"},
    {L"MSTeams_8wekyb3d8bbwe", L"Microsoft Teams"},
    {L"MicrosoftTeams_8wekyb3d8bbwe", L"Microsoft Teams (personal)"}
};

static const OptionalPackage *allowed_family(const wchar_t *family) {
    for (unsigned i=0; i<ARRAYSIZE(optional_packages); ++i)
        if (!wcscmp(family,optional_packages[i].family)) return &optional_packages[i];
    return NULL;
}

const wchar_t *packages_display_name(const wchar_t *family) {
    const OptionalPackage *p = allowed_family(family);
    if (!p) return family;
    static const struct { const wchar_t *en, *vi; } names[] = {
        {L"Microsoft News", L"Microsoft News (tin tức)"},
        {L"Microsoft Weather", L"Microsoft Weather (thời tiết)"},
        {L"Microsoft Money", L"Microsoft Money (tài chính)"},
        {L"Microsoft Sports", L"Microsoft Sports (thể thao)"},
        {L"Get Help", L"Get Help (trợ giúp)"},
        {L"Tips / Get Started", L"Mẹo / Bắt đầu"},
        {L"Microsoft People", L"Microsoft People (danh bạ)"},
        {L"Feedback Hub", L"Trung tâm phản hồi"},
        {L"Windows Maps", L"Bản đồ Windows"},
        {L"Xbox app (game management)", L"Ứng dụng Xbox (quản lý trò chơi)"},
        {L"Movies & TV", L"Phim & TV"},
        {L"Mixed Reality Portal", L"Cổng thực tế hỗn hợp"},
        {L"3D Viewer", L"Trình xem 3D"},
        {L"Paint 3D (not Windows Paint)", L"Paint 3D (không phải Windows Paint)"},
        {L"Sticky Notes (back up your notes first)", L"Sticky Notes (hãy sao lưu ghi chú trước)"},
        {L"Phone Link", L"Liên kết điện thoại"},
        {L"Outlook for Windows", L"Outlook dành cho Windows"},
        {L"Microsoft Teams (personal)", L"Microsoft Teams (cá nhân)"}
    };
    for (size_t i = 0; i < ARRAYSIZE(names); ++i)
        if (!wcscmp(p->name, names[i].en)) return tr(names[i].vi, names[i].en);
    return p->name;
}

DWORD packages_enumerate(PackageItem *items, unsigned capacity, unsigned *count,
                         const wchar_t *only_family) {
    *count=0;
    if (only_family && !allowed_family(only_family)) return ERROR_INVALID_PARAMETER;
    for (unsigned i=0; i<ARRAYSIZE(optional_packages); ++i) {
        const OptionalPackage *p=&optional_packages[i];
        UINT32 n=0, length=0;
        if (only_family && wcscmp(only_family,p->family)) continue;
        LONG rc=GetPackagesByPackageFamily(p->family,&n,NULL,&length,NULL);
        if (rc==ERROR_SUCCESS && n==0) continue;
        if (rc!=ERROR_INSUFFICIENT_BUFFER) return (DWORD)rc;
        if (n>PACKAGE_ITEMS_MAX || length>262144) return ERROR_BUFFER_OVERFLOW;
        PWSTR *names=calloc(n,sizeof(*names));
        PWSTR buffer=calloc(length,sizeof(wchar_t));
        if (!names || !buffer) { free(names); free(buffer); return ERROR_NOT_ENOUGH_MEMORY; }
        rc=GetPackagesByPackageFamily(p->family,&n,names,&length,buffer);
        if (rc==ERROR_SUCCESS) for (UINT32 j=0; j<n; ++j) {
            if (*count>=capacity) { rc=ERROR_BUFFER_OVERFLOW; break; }
            PackageItem *item=&items[(*count)++];
            ZeroMemory(item,sizeof(*item));
            if (FAILED(StringCchCopyW(item->full_name,ARRAYSIZE(item->full_name),names[j])) ||
                FAILED(StringCchCopyW(item->family,ARRAYSIZE(item->family),p->family))) {
                rc=ERROR_BUFFER_OVERFLOW; break;
            }
            StringCchCopyW(item->display,ARRAYSIZE(item->display),packages_display_name(p->family));
        }
        free(names); free(buffer);
        if (rc!=ERROR_SUCCESS) return (DWORD)rc;
    }
    return ERROR_SUCCESS;
}

HRESULT packages_remove(const PackageItem *item) {
    if (app_is_smoke_test()) return E_ACCESSDENIED;
    if (!item) return E_INVALIDARG;
    /* Re-derive identity from the exact full name at the mutation boundary. */
    wchar_t actual_family[160];
    UINT32 length=ARRAYSIZE(actual_family);
    LONG err=PackageFamilyNameFromFullName(item->full_name,&length,actual_family);
    if (err || !allowed_family(actual_family) || wcscmp(actual_family,item->family))
        return E_INVALIDARG;
    HRESULT hr=RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(hr)) return hr;
    HSTRING class_name=NULL, package_name=NULL;
    IInspectable *instance=NULL;
    NativePackageManager *manager=NULL;
    NativeDeploymentOperation *operation=NULL;
    IAsyncInfo *info=NULL;
    NativeDeploymentResult *result=NULL;
    const wchar_t cls[]=L"Windows.Management.Deployment.PackageManager";
    hr=WindowsCreateString(cls,ARRAYSIZE(cls)-1,&class_name);
    if (SUCCEEDED(hr)) hr=RoActivateInstance(class_name,&instance);
    if (SUCCEEDED(hr)) hr=IInspectable_QueryInterface(instance,&package_manager_iid,(void **)&manager);
    if (SUCCEEDED(hr)) hr=WindowsCreateString(item->full_name,(UINT32)wcslen(item->full_name),&package_name);
    if (SUCCEEDED(hr)) hr=manager->lpVtbl->RemovePackageAsync(manager,package_name,&operation);
    if (SUCCEEDED(hr)) hr=operation->lpVtbl->QueryInterface(operation,&async_info_iid,(void **)&info);
    if (SUCCEEDED(hr)) {
        AsyncStatus state=Started;
        ULONGLONG started=GetTickCount64(), last_log=started;
        do {
            hr=IAsyncInfo_get_Status(info,&state);
            if (FAILED(hr) || state!=Started) break;
            ULONGLONG now=GetTickCount64();
            if (now-last_log>15000) { app_log(tr(L"Đang chờ Windows xử lý gói ứng dụng: %s", L"Waiting for Windows package deployment: %s"),item->display); last_log=now; }
            /* Wait until the OS acknowledges cancellation; never report a timeout as success. */
            if (now-started>300000) IAsyncInfo_Cancel(info);
            Sleep(100);
        } while (state==Started);
        if (SUCCEEDED(hr)) {
            if (state==Canceled) hr=HRESULT_FROM_WIN32(ERROR_CANCELLED);
            else if (state==Error) {
                HRESULT deployment_error=E_FAIL;
                HRESULT query=IAsyncInfo_get_ErrorCode(info,&deployment_error);
                hr=FAILED(query)?query:deployment_error;
                if (SUCCEEDED(hr)) hr=E_FAIL;
            } else hr=operation->lpVtbl->GetResults(operation,&result);
        }
        if (result) {
            HRESULT extended=S_OK;
            if (SUCCEEDED(result->lpVtbl->get_ExtendedErrorCode(result,&extended)) && FAILED(extended)) hr=extended;
            HSTRING message=NULL;
            if (SUCCEEDED(result->lpVtbl->get_ErrorText(result,&message)) && message) {
                const wchar_t *raw=WindowsGetStringRawBuffer(message,NULL);
                if (raw && *raw) app_log(tr(L"Xử lý gói ứng dụng: %s", L"Package deployment: %s"),raw);
                WindowsDeleteString(message);
            }
        }
    }
    if (result) result->lpVtbl->Release(result);
    if (info) { IAsyncInfo_Close(info); IAsyncInfo_Release(info); }
    if (operation) operation->lpVtbl->Release(operation);
    if (manager) manager->lpVtbl->Release(manager);
    if (instance) IInspectable_Release(instance);
    WindowsDeleteString(package_name); WindowsDeleteString(class_name);
    RoUninitialize();
    app_log(L"%s: %s (0x%08lX)",SUCCEEDED(hr)?tr(L"ĐÃ GỠ", L"REMOVED"):tr(L"LỖI", L"FAILED"),item->full_name,(unsigned long)hr);
    return hr;
}

BOOL packages_self_test(void) {
    if (allowed_family(L"Microsoft.WindowsStore_8wekyb3d8bbwe")) return FALSE;
    if (allowed_family(L"Microsoft.Windows.ShellExperienceHost_cw5n1h2txyewy")) return FALSE;
    for (unsigned i=0;i<ARRAYSIZE(optional_packages);++i) {
        if (!wcschr(optional_packages[i].family,L'_')) return FALSE;
        for (unsigned j=i+1;j<ARRAYSIZE(optional_packages);++j)
            if (!wcscmp(optional_packages[i].family,optional_packages[j].family)) return FALSE;
    }
    /* Exercise the real WinRT activation boundary without starting a deployment. */
    HRESULT hr=RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(hr)) return FALSE;
    HSTRING name=NULL;
    IInspectable *instance=NULL;
    NativePackageManager *manager=NULL;
    const wchar_t cls[]=L"Windows.Management.Deployment.PackageManager";
    hr=WindowsCreateString(cls,ARRAYSIZE(cls)-1,&name);
    if (SUCCEEDED(hr)) hr=RoActivateInstance(name,&instance);
    if (SUCCEEDED(hr)) hr=IInspectable_QueryInterface(instance,&package_manager_iid,(void **)&manager);
    if (manager) manager->lpVtbl->Release(manager);
    if (instance) IInspectable_Release(instance);
    WindowsDeleteString(name);
    RoUninitialize();
    return SUCCEEDED(hr);
}
