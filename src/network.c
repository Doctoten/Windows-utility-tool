#include "app.h"
#include "theme.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string.h>

enum {
    ID_NET_LIST = 3001, ID_NET_REFRESH, ID_NET_FAMILY, ID_NET_PROVIDER,
    ID_NET_PRIMARY, ID_NET_SECONDARY, ID_NET_APPLY, ID_NET_AUTO,
    ID_NET_FLUSH, ID_NET_RESET, ID_NET_WIFI, ID_NET_STATUS,
    ID_NET_HEADING, ID_NET_HINT, ID_NET_GROUP, ID_NET_FAMILY_LABEL,
    ID_NET_PROVIDER_LABEL, ID_NET_PRIMARY_LABEL, ID_NET_SECONDARY_LABEL,
    ID_NET_NOTE
};

typedef struct {
    NET_LUID luid;
    ULONG ipv4_index, ipv6_index;
    BOOL ipv4_enabled, ipv6_enabled;
    IF_OPER_STATUS oper_status;
    wchar_t name[257], status[80], ip[1536], mac[96], dns[1536];
} NetworkAdapter;

typedef enum { NET_REFRESH, NET_DNS, NET_AUTO, NET_FLUSH, NET_RESET } NetworkJobKind;
typedef struct {
    NetworkJobKind kind;
    int family;
    ULONG index;
    NET_LUID luid;
    wchar_t adapter[257], primary[64], secondary[64];
    DWORD error, refresh_error, step_errors[3];
    unsigned completed, attempted;
    NetworkAdapter *rows;
    size_t row_count;
} NetworkJob;

typedef struct {
    HWND window, list;
    NetworkAdapter *rows;
    size_t row_count;
    NetworkJob *job;
    unsigned status_kind;
} NetworkWindow;

static HWND network_window;
static const wchar_t *const dns_presets[2][3][2] = {
    {{L"1.1.1.1", L"1.0.0.1"}, {L"8.8.8.8", L"8.8.4.4"}, {L"9.9.9.9", L"149.112.112.112"}},
    {{L"2606:4700:4700::1111", L"2606:4700:4700::1001"},
     {L"2001:4860:4860::8888", L"2001:4860:4860::8844"},
     {L"2620:fe::fe", L"2620:fe::9"}}
};

/* InetPton accepts address literals only: never host names or command fragments. */
BOOL network_valid_dns(const wchar_t *address, int family) {
    if (!address || !address[0] || wcslen(address) >= 64) return FALSE;
    if (family == AF_INET) {
        IN_ADDR value;
        if (InetPtonW(AF_INET, address, &value) != 1) return FALSE;
        ULONG host = ntohl(value.s_addr);
        return (host >> 24) != 0 && (host >> 24) < 224;
    }
    if (family == AF_INET6) {
        IN6_ADDR value;
        if (InetPtonW(AF_INET6, address, &value) != 1) return FALSE;
        return !IN6_IS_ADDR_UNSPECIFIED(&value) && !IN6_IS_ADDR_MULTICAST(&value)
               && !IN6_IS_ADDR_V4MAPPED(&value);
    }
    return FALSE;
}

static BOOL same_dns(const wchar_t *first, const wchar_t *second, int family) {
    BYTE a[16] = {0}, b[16] = {0};
    return InetPtonW(family, first, a) == 1 && InetPtonW(family, second, b) == 1
           && memcmp(a, b, family == AF_INET ? 4u : 16u) == 0;
}

BOOL network_self_test(void) {
    static const struct { const wchar_t *address; int family; BOOL valid; } cases[] = {
        {L"127.0.0.1", AF_INET, TRUE}, {L"192.168.1.1", AF_INET, TRUE},
        {L"::1", AF_INET6, TRUE}, {L"fd00::53", AF_INET6, TRUE},
        {NULL, AF_INET, FALSE}, {L"", AF_INET, FALSE},
        {L"0.0.0.0", AF_INET, FALSE}, {L"224.0.0.1", AF_INET, FALSE},
        {L"255.255.255.255", AF_INET, FALSE}, {L"1.1.1.256", AF_INET, FALSE},
        {L"1.1.1.1 & calc", AF_INET, FALSE}, {L"1.1.1.1\"", AF_INET, FALSE},
        {L"1.1.1.1\r\n", AF_INET, FALSE}, {L" 1.1.1.1", AF_INET, FALSE},
        {L"1.1.1.1:53", AF_INET, FALSE}, {L"dns.google", AF_INET, FALSE},
        {L"::", AF_INET6, FALSE}, {L"ff02::1", AF_INET6, FALSE},
        {L"::ffff:1.1.1.1", AF_INET6, FALSE}, {L"2001:::1", AF_INET6, FALSE},
        {L"[2606:4700:4700::1111]:53", AF_INET6, FALSE},
        {L"fe80::1%7", AF_INET6, FALSE}, {L"1.1.1.1", AF_INET6, FALSE},
        {L"2606:4700:4700::1111", AF_INET, FALSE}, {L"1.1.1.1", AF_UNSPEC, FALSE}
    };
    for (size_t i = 0; i < ARRAYSIZE(cases); ++i)
        if (network_valid_dns(cases[i].address, cases[i].family) != cases[i].valid) return FALSE;
    for (int family = 0; family < 2; ++family)
        for (int provider = 0; provider < 3; ++provider)
            for (int server = 0; server < 2; ++server)
                if (!network_valid_dns(dns_presets[family][provider][server], family ? AF_INET6 : AF_INET)) return FALSE;
    return same_dns(L"2001:4860:4860::8888", L"2001:4860:4860:0:0:0:0:8888", AF_INET6)
        && !same_dns(L"1.1.1.1", L"1.0.0.1", AF_INET);
}

static void append_address(wchar_t *output, size_t capacity, const SOCKET_ADDRESS *socket_address) {
    wchar_t address[96] = L"";
    const SOCKADDR *sa = socket_address->lpSockaddr;
    if (!sa) return;
    if (sa->sa_family == AF_INET && socket_address->iSockaddrLength >= (int)sizeof(SOCKADDR_IN)) {
        const SOCKADDR_IN *ipv4 = (const SOCKADDR_IN *)sa;
        InetNtopW(AF_INET, &ipv4->sin_addr, address, ARRAYSIZE(address));
    } else if (sa->sa_family == AF_INET6 && socket_address->iSockaddrLength >= (int)sizeof(SOCKADDR_IN6)) {
        const SOCKADDR_IN6 *ipv6 = (const SOCKADDR_IN6 *)sa;
        InetNtopW(AF_INET6, &ipv6->sin6_addr, address, ARRAYSIZE(address));
        if (ipv6->sin6_scope_id) {
            size_t length = wcslen(address);
            StringCchPrintfW(address + length, ARRAYSIZE(address) - length, L"%%%lu", ipv6->sin6_scope_id);
        }
    }
    if (!address[0]) return;
    if (wcslen(output) + wcslen(address) + 3 >= capacity) return;
    if (output[0]) StringCchCatW(output, capacity, L", ");
    StringCchCatW(output, capacity, address);
}

static const wchar_t *adapter_status(IF_OPER_STATUS status) {
    switch (status) {
    case IfOperStatusUp: return tr(L"Đã kết nối", L"Connected");
    case IfOperStatusDormant: return tr(L"Chờ kết nối", L"Dormant");
    case IfOperStatusNotPresent: return tr(L"Không khả dụng", L"Not present");
    case IfOperStatusLowerLayerDown: return tr(L"Mất liên kết", L"Link down");
    case IfOperStatusDown: return tr(L"Đã ngắt kết nối", L"Disconnected");
    default: return tr(L"Không xác định", L"Unknown");
    }
}

static DWORD collect_adapters(NetworkAdapter **rows, size_t *count) {
    ULONG size = 15 * 1024;
    IP_ADAPTER_ADDRESSES *buffer = NULL;
    DWORD result = ERROR_NOT_ENOUGH_MEMORY;
    *rows = NULL;
    *count = 0;
    for (int attempt = 0; attempt < 4; ++attempt) {
        if (size > 16 * 1024 * 1024) { result = ERROR_BUFFER_OVERFLOW; break; }
        buffer = (IP_ADAPTER_ADDRESSES *)malloc(size);
        if (!buffer) { result = ERROR_NOT_ENOUGH_MEMORY; break; }
        result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES |
                    GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, NULL, buffer, &size);
        if (result != ERROR_BUFFER_OVERFLOW) break;
        free(buffer);
        buffer = NULL;
    }
    if (result == ERROR_NO_DATA) { free(buffer); return ERROR_SUCCESS; }
    if (result != ERROR_SUCCESS) { free(buffer); return result; }
    size_t total = 0;
    for (IP_ADAPTER_ADDRESSES *p = buffer; p; p = p->Next)
        if (p->IfType != IF_TYPE_SOFTWARE_LOOPBACK) ++total;
    NetworkAdapter *items = total ? (NetworkAdapter *)calloc(total, sizeof(*items)) : NULL;
    if (total && !items) { free(buffer); return ERROR_NOT_ENOUGH_MEMORY; }
    size_t n = 0;
    for (IP_ADAPTER_ADDRESSES *p = buffer; p; p = p->Next) {
        if (p->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        NetworkAdapter *row = &items[n++];
        row->luid = p->Luid;
        row->ipv4_index = p->IfIndex;
        row->ipv6_index = p->Ipv6IfIndex;
        row->ipv4_enabled = p->Ipv4Enabled;
        row->ipv6_enabled = p->Ipv6Enabled;
        StringCchCopyW(row->name, ARRAYSIZE(row->name), p->FriendlyName ? p->FriendlyName : L"?");
        row->oper_status = p->OperStatus;
        StringCchCopyW(row->status, ARRAYSIZE(row->status), adapter_status(row->oper_status));
        for (IP_ADAPTER_UNICAST_ADDRESS *a = p->FirstUnicastAddress; a; a = a->Next)
            append_address(row->ip, ARRAYSIZE(row->ip), &a->Address);
        for (IP_ADAPTER_DNS_SERVER_ADDRESS *a = p->FirstDnsServerAddress; a; a = a->Next)
            append_address(row->dns, ARRAYSIZE(row->dns), &a->Address);
        for (ULONG i = 0; i < p->PhysicalAddressLength && i < MAX_ADAPTER_ADDRESS_LENGTH; ++i) {
            size_t length = wcslen(row->mac);
            StringCchPrintfW(row->mac + length, ARRAYSIZE(row->mac) - length,
                             i ? L"-%02X" : L"%02X", p->PhysicalAddress[i]);
        }
        if (!row->ip[0]) StringCchCopyW(row->ip, ARRAYSIZE(row->ip), L"—");
        if (!row->mac[0]) StringCchCopyW(row->mac, ARRAYSIZE(row->mac), L"—");
        if (!row->dns[0]) StringCchCopyW(row->dns, ARRAYSIZE(row->dns), L"—");
    }
    free(buffer);
    *rows = items;
    *count = total;
    return ERROR_SUCCESS;
}

static void network_work(void *context) {
    NetworkJob *job = (NetworkJob *)context;
    wchar_t arguments[512];
    if (job->kind == NET_DNS || job->kind == NET_AUTO) {
        NET_LUID current_luid;
        job->error = ConvertInterfaceIndexToLuid(job->index, &current_luid);
        if (!job->error && current_luid.Value != job->luid.Value) job->error = ERROR_DEVICE_NOT_CONNECTED;
        if (!job->error) {
            const wchar_t *protocol = job->family == AF_INET ? L"ipv4" : L"ipv6";
            if (job->kind == NET_AUTO)
                StringCchPrintfW(arguments, ARRAYSIZE(arguments),
                    L"interface %s set dnsservers name=%lu source=dhcp", protocol, job->index);
            else
                StringCchPrintfW(arguments, ARRAYSIZE(arguments),
                    L"interface %s set dnsservers name=%lu source=static address=%s validate=no",
                    protocol, job->index, job->primary);
            app_log(tr(L"DNS: %s (card mạng %lu, %s)", L"DNS: %s (interface %lu, %s)"), job->adapter, job->index, protocol);
            job->attempted++;
            job->error = app_run_tool(L"netsh.exe", arguments);
            job->step_errors[0] = job->error;
            if (!job->error) ++job->completed;
            if (!job->error && job->kind == NET_DNS && job->secondary[0]) {
                StringCchPrintfW(arguments, ARRAYSIZE(arguments),
                    L"interface %s add dnsservers name=%lu address=%s index=2 validate=no",
                    protocol, job->index, job->secondary);
                job->attempted++;
                job->error = app_run_tool(L"netsh.exe", arguments);
                job->step_errors[1] = job->error;
                if (!job->error) ++job->completed;
            }
        }
    } else if (job->kind == NET_FLUSH) {
        job->attempted = 1;
        job->error = app_run_tool(L"ipconfig.exe", L"/flushdns");
        if (!job->error) job->completed = 1;
    } else if (job->kind == NET_RESET) {
        const wchar_t *commands[] = {L"interface ipv4 reset", L"interface ipv6 reset", L"winsock reset"};
        for (size_t i = 0; i < ARRAYSIZE(commands); ++i) {
            job->attempted++;
            DWORD error = app_run_tool(L"netsh.exe", commands[i]);
            job->step_errors[i] = error;
            if (error && !job->error) job->error = error;
            if (!error) ++job->completed;
        }
    }
    if (job->kind != NET_REFRESH)
        app_log(tr(L"Mạng: mã kết quả %lu; %u/%u bước thành công.",
                   L"Network: result code %lu; %u/%u steps succeeded."),
                job->error, job->completed, job->attempted);
    job->refresh_error = collect_adapters(&job->rows, &job->row_count);
}

static void show_rows(NetworkWindow *state, NetworkJob *job) {
    ULONGLONG selected_luid = 0;
    int selected = ListView_GetNextItem(state->list, -1, LVNI_SELECTED);
    if (selected >= 0 && (size_t)selected < state->row_count) selected_luid = state->rows[selected].luid.Value;
    SendMessageW(state->list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(state->list);
    free(state->rows);
    state->rows = job->rows;
    state->row_count = job->row_count;
    job->rows = NULL;
    int restore = -1;
    for (size_t i = 0; i < state->row_count; ++i) {
        NetworkAdapter *row = &state->rows[i];
        LVITEMW item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = (int)i;
        item.pszText = row->name;
        int index = ListView_InsertItem(state->list, &item);
        if (index >= 0) {
            ListView_SetItemText(state->list, index, 1, row->status);
            ListView_SetItemText(state->list, index, 2, row->ip);
            ListView_SetItemText(state->list, index, 3, row->mac);
            ListView_SetItemText(state->list, index, 4, row->dns);
        }
        if (row->luid.Value == selected_luid) restore = index;
    }
    if (restore >= 0) ListView_SetItemState(state->list, restore, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    SendMessageW(state->list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(state->list, NULL, TRUE);
}

static void network_complete(NetworkWindow *state) {
    HWND window = state->window;
    NetworkJob *job = state->job;
    state->job = NULL;
    app_finish_job(window);
    if (!job) return; /* Shared Wi-Fi restoration owns and frees its context. */
    wchar_t message[1536] = L"";
    if (!job->refresh_error) show_rows(state, job);
    if (job->kind == NET_REFRESH) {
        if (job->refresh_error) app_error(window, tr(L"Không thể đọc thông tin card mạng", L"Could not read network adapters"), job->refresh_error);
    } else if (job->kind == NET_RESET) {
        StringCchPrintfW(message, ARRAYSIZE(message),
            tr(L"Đã hoàn tất %u/3 bước đặt lại mạng.\n\nMã kết quả: IPv4 = %lu; IPv6 = %lu; Winsock = %lu.\n"
               L"Mã 0 là thành công. Xem nhật ký để biết chi tiết. Các bước lỗi có thể đã thay đổi một phần cấu hình.\n\n"
               L"Khởi động lại máy tính để áp dụng các thay đổi đã thực hiện.",
               L"Completed %u/3 network reset steps.\n\nResult codes: IPv4 = %lu; IPv6 = %lu; Winsock = %lu.\n"
               L"Code 0 means success. See the log for details. Failed steps may have changed some settings.\n\n"
               L"Restart your computer to apply any changes made."),
            job->completed, job->step_errors[0], job->step_errors[1], job->step_errors[2]);
    } else if (job->error && job->kind == NET_DNS && job->completed) {
        StringCchPrintfW(message, ARRAYSIZE(message),
            tr(L"DNS chính %s đã được đặt cho %s, nhưng không thể thêm DNS phụ %s (mã %lu).\n\n"
               L"Cấu hình đã thay đổi một phần. Kiểm tra danh sách DNS và nhật ký trước khi thử lại.",
               L"Primary DNS %s was set for %s, but secondary DNS %s could not be added (code %lu).\n\n"
               L"The configuration changed partially. Check the DNS list and log before retrying."),
            job->primary, job->adapter, job->secondary, job->error);
    } else if (job->error) {
        StringCchPrintfW(message, ARRAYSIZE(message),
            tr(L"Thao tác mạng không hoàn tất (mã %lu). Xem nhật ký để biết chi tiết.\n"
               L"Kiểm tra cấu hình hiện tại: lệnh lỗi vẫn có thể thay đổi một phần thiết lập.",
               L"The network operation did not complete (code %lu). See the log for details.\n"
               L"Check the current configuration: a failed command may still have changed some settings."), job->error);
    } else if (job->kind == NET_FLUSH) {
        StringCchCopyW(message, ARRAYSIZE(message), tr(L"Đã xóa bộ nhớ đệm DNS.", L"DNS cache flushed."));
    } else {
        StringCchPrintfW(message, ARRAYSIZE(message),
            job->kind == NET_AUTO
                ? tr(L"Đã đặt DNS %s tự động (DHCP) cho %s.", L"Automatic (DHCP) %s DNS was set for %s.")
                : tr(L"Đã cập nhật DNS %s cho %s.", L"%s DNS was updated for %s."),
            job->family == AF_INET ? L"IPv4" : L"IPv6", job->adapter);
    }
    if (job->refresh_error && job->kind != NET_REFRESH) {
        wchar_t note[160];
        StringCchPrintfW(note, ARRAYSIZE(note),
            tr(L"\n\nKhông tải lại được danh sách (mã %lu). Thông tin đang hiển thị có thể đã cũ.",
               L"\n\nCould not refresh the list (code %lu). Displayed information may be outdated."), job->refresh_error);
        StringCchCatW(message, ARRAYSIZE(message), note);
    }
    state->status_kind = job->refresh_error ? 1 : job->error ? 2 : 0;
    SetDlgItemTextW(window, ID_NET_STATUS,
        job->refresh_error ? tr(L"Tải lại thất bại; thông tin có thể đã cũ.", L"Refresh failed; information may be outdated.") :
        job->error ? tr(L"Thao tác có lỗi. Xem nhật ký.", L"Operation had errors. See the log.") :
        tr(L"Sẵn sàng. Chọn card mạng trước khi đổi DNS.", L"Ready. Select an adapter before changing DNS."));
    if (message[0]) MessageBoxW(window, message, tr(L"Thiết lập mạng", L"Network setup"),
        MB_OK | (job->error || job->refresh_error ? MB_ICONWARNING : MB_ICONINFORMATION));
    free(job->rows);
    free(job);
}

static void start_network_job(NetworkWindow *state, NetworkJob *job) {
    HWND window = state->window;
    state->job = job;
    if (!app_start_job(window, network_work, job)) {
        if (IsWindow(window) && (NetworkWindow *)GetWindowLongPtrW(window, GWLP_USERDATA) == state)
            state->job = NULL;
        free(job);
        return;
    }
    SetDlgItemTextW(window, ID_NET_STATUS, tr(L"Đang xử lý…", L"Working…"));
}

static int selected_family(HWND window) {
    return SendDlgItemMessageW(window, ID_NET_FAMILY, CB_GETCURSEL, 0, 0) == 1 ? AF_INET6 : AF_INET;
}

static void populate_provider(HWND window) {
    LRESULT provider = SendDlgItemMessageW(window, ID_NET_PROVIDER, CB_GETCURSEL, 0, 0);
    if (provider >= 0 && provider < 3) {
        int family = selected_family(window) == AF_INET6;
        SetDlgItemTextW(window, ID_NET_PRIMARY, dns_presets[family][provider][0]);
        SetDlgItemTextW(window, ID_NET_SECONDARY, dns_presets[family][provider][1]);
    }
}

static void request_dns(NetworkWindow *state, BOOL automatic) {
    int selected = ListView_GetNextItem(state->list, -1, LVNI_SELECTED);
    if (selected < 0 || (size_t)selected >= state->row_count) {
        MessageBoxW(state->window, tr(L"Vui lòng chọn một card mạng trong danh sách.", L"Select a network adapter in the list."),
                    tr(L"Đổi DNS", L"Change DNS"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    NetworkAdapter *adapter = &state->rows[selected];
    int family = selected_family(state->window);
    if (!(family == AF_INET ? adapter->ipv4_enabled : adapter->ipv6_enabled)) {
        MessageBoxW(state->window,
            tr(L"Giao thức này chưa được bật trên card mạng đã chọn. Chọn giao thức khác hoặc bật nó trong cài đặt Windows.",
               L"This protocol is not enabled on the selected adapter. Select another protocol or enable it in Windows settings."),
            tr(L"Đổi DNS", L"Change DNS"), MB_OK | MB_ICONWARNING);
        return;
    }
    NetworkJob *job = (NetworkJob *)calloc(1, sizeof(*job));
    if (!job) { app_error(state->window, L"DNS", ERROR_NOT_ENOUGH_MEMORY); return; }
    job->kind = automatic ? NET_AUTO : NET_DNS;
    job->family = family;
    job->luid = adapter->luid;
    job->index = family == AF_INET ? adapter->ipv4_index : adapter->ipv6_index;
    StringCchCopyW(job->adapter, ARRAYSIZE(job->adapter), adapter->name);
    if (!automatic) {
        GetDlgItemTextW(state->window, ID_NET_PRIMARY, job->primary, ARRAYSIZE(job->primary));
        GetDlgItemTextW(state->window, ID_NET_SECONDARY, job->secondary, ARRAYSIZE(job->secondary));
        if (!network_valid_dns(job->primary, family) ||
            (job->secondary[0] && (!network_valid_dns(job->secondary, family) || same_dns(job->primary, job->secondary, family)))) {
            MessageBoxW(state->window,
                tr(L"Nhập địa chỉ DNS hợp lệ đúng giao thức IPv4/IPv6 đã chọn. DNS phụ có thể để trống nhưng không được trùng DNS chính.\n"
                   L"Không nhập tên miền, khoảng trắng, cổng hoặc địa chỉ multicast/không xác định.",
                   L"Enter valid DNS addresses for the selected IPv4/IPv6 protocol. Secondary DNS may be empty but must differ from primary DNS.\n"
                   L"Do not enter host names, spaces, ports, or multicast/unspecified addresses."),
                tr(L"Địa chỉ DNS không hợp lệ", L"Invalid DNS address"), MB_OK | MB_ICONWARNING);
            free(job);
            return;
        }
    }
    wchar_t details[4096];
    if (automatic)
        StringCchPrintfW(details, ARRAYSIZE(details),
            tr(L"Card mạng: %s (index %lu)\nGiao thức: %s\nDNS hiện tại: %s\n\n"
               L"Xóa DNS đặt thủ công của giao thức này và lấy DNS tự động (DHCP)?\nĐịa chỉ IP của card mạng không thay đổi.",
               L"Adapter: %s (index %lu)\nProtocol: %s\nCurrent DNS: %s\n\n"
               L"Remove this protocol's manual DNS settings and obtain DNS automatically (DHCP)?\nThe adapter's IP address will stay unchanged."),
            adapter->name, job->index, family == AF_INET ? L"IPv4" : L"IPv6", adapter->dns);
    else
        StringCchPrintfW(details, ARRAYSIZE(details),
            tr(L"Card mạng: %s (index %lu)\nGiao thức: %s\nDNS hiện tại: %s\n\n"
               L"DNS chính mới: %s\nDNS phụ mới: %s\n\nThay danh sách DNS thủ công của giao thức này bằng các địa chỉ trên?\n"
               L"Địa chỉ IP của card mạng không thay đổi. Khả năng truy cập DNS sẽ không được kiểm tra trước khi áp dụng.",
               L"Adapter: %s (index %lu)\nProtocol: %s\nCurrent DNS: %s\n\n"
               L"New primary DNS: %s\nNew secondary DNS: %s\n\nReplace this protocol's manual DNS list with these addresses?\n"
               L"The adapter's IP address will stay unchanged. DNS reachability will not be tested before applying."),
            adapter->name, job->index, family == AF_INET ? L"IPv4" : L"IPv6", adapter->dns,
            job->primary, job->secondary[0] ? job->secondary : tr(L"Không có", L"None"));
    HWND window = state->window;
    if (!app_confirm(window, details) || !IsWindow(window) ||
        (NetworkWindow *)GetWindowLongPtrW(window, GWLP_USERDATA) != state) { free(job); return; }
    start_network_job(state, job);
}

static void request_operation(NetworkWindow *state, NetworkJobKind kind) {
    HWND window = state->window;
    if (kind == NET_FLUSH && !app_confirm(window,
        tr(L"Xóa bộ nhớ đệm phân giải DNS của Windows?\n\nCấu hình máy chủ DNS được giữ nguyên; các tên miền sẽ được phân giải lại khi cần.",
           L"Clear the Windows DNS resolver cache?\n\nYour DNS server configuration is preserved; names will be resolved again when needed."))) return;
    if (kind == NET_RESET && !app_confirm(window,
        tr(L"Đặt lại IPv4, IPv6 và danh mục Winsock cho TẤT CẢ card mạng?\n\n"
           L"Thao tác có thể xóa IP tĩnh, gateway, DNS và thiết lập mạng/VPN tùy chỉnh, đồng thời ngắt kết nối.\n"
           L"Ghi lại thiết lập hiện tại trước khi tiếp tục. Bạn cần khởi động lại máy tính sau đó.",
           L"Reset IPv4, IPv6 and the Winsock catalog for ALL network adapters?\n\n"
           L"This can remove static IP addresses, gateways, DNS and custom network/VPN settings, and disconnect you.\n"
           L"Record your current settings before continuing. A computer restart is required afterward."))) return;
    if (!IsWindow(window) || (NetworkWindow *)GetWindowLongPtrW(window, GWLP_USERDATA) != state) return;
    NetworkJob *job = (NetworkJob *)calloc(1, sizeof(*job));
    if (!job) { app_error(state->window, tr(L"Mạng", L"Network"), ERROR_NOT_ENOUGH_MEMORY); return; }
    job->kind = kind;
    start_network_job(state, job);
}

static void move_control(HWND window, int id, int x, int y, int w, int h) {
    MoveWindow(GetDlgItem(window, id), x, y, w, h, TRUE);
}

static void network_layout(HWND window) {
    RECT rect;
    GetClientRect(window, &rect);
    int w = rect.right, h = rect.bottom;
    int list_height = h - 360;
    if (list_height < 130) list_height = 130;
    move_control(window, ID_NET_HEADING, 20, 16, w - 170, 25);
    move_control(window, ID_NET_HINT, 20, 46, w - 40, 22);
    move_control(window, ID_NET_REFRESH, w - 150, 15, 130, 30);
    move_control(window, ID_NET_LIST, 20, 76, w - 40, list_height);
    int y = 76 + list_height + 14;
    move_control(window, ID_NET_GROUP, 20, y, w - 40, 168);
    int half = (w - 72) / 2;
    move_control(window, ID_NET_FAMILY_LABEL, 34, y + 26, 94, 24);
    move_control(window, ID_NET_FAMILY, 132, y + 22, 145, 140);
    move_control(window, ID_NET_PROVIDER_LABEL, 306, y + 26, 128, 24);
    move_control(window, ID_NET_PROVIDER, 437, y + 22, w - 477, 180);
    move_control(window, ID_NET_PRIMARY_LABEL, 34, y + 62, 120, 22);
    move_control(window, ID_NET_PRIMARY, 34, y + 86, half, 29);
    move_control(window, ID_NET_SECONDARY_LABEL, 46 + half, y + 62, 270, 22);
    move_control(window, ID_NET_SECONDARY, 46 + half, y + 86, half, 29);
    move_control(window, ID_NET_APPLY, 34, y + 126, 188, 30);
    move_control(window, ID_NET_AUTO, 234, y + 126, 226, 30);
    move_control(window, ID_NET_NOTE, 20, y + 178, w - 40, 23);
    move_control(window, ID_NET_FLUSH, 20, y + 209, 188, 32);
    move_control(window, ID_NET_RESET, 220, y + 209, 188, 32);
    move_control(window, ID_NET_WIFI, 420, y + 209, 188, 32);
    move_control(window, ID_NET_STATUS, 20, h - 28, w - 40, 22);
}

static void create_network_controls(NetworkWindow *state) {
    HWND window = state->window;
    app_control(window, L"STATIC", tr(L"Card mạng & DNS", L"Adapters & DNS"), 0, 0, 0, 0, 0, ID_NET_HEADING);
    app_control(window, L"STATIC", tr(L"Chọn card mạng, sau đó chọn giao thức và nhà cung cấp DNS.",
        L"Select an adapter, then choose the protocol and DNS provider."), 0, 0, 0, 0, 0, ID_NET_HINT);
    app_control(window, L"BUTTON", tr(L"Tải lại", L"Refresh"), WS_TABSTOP, 0, 0, 0, 0, ID_NET_REFRESH);
    state->list = app_control(window, WC_LISTVIEWW, L"", WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                             0, 0, 0, 0, ID_NET_LIST);
    ListView_SetExtendedListViewStyle(state->list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
    const wchar_t *columns[] = {tr(L"Card mạng", L"Adapter"), tr(L"Trạng thái", L"Status"),
        tr(L"Địa chỉ IP", L"IP addresses"), L"MAC", tr(L"Máy chủ DNS", L"DNS servers")};
    const int widths[] = {180, 128, 275, 152, 310};
    for (int i = 0; i < 5; ++i) {
        LVCOLUMNW column = {0};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = (LPWSTR)columns[i];
        column.cx = widths[i];
        column.iSubItem = i;
        ListView_InsertColumn(state->list, i, &column);
    }
    app_control(window, L"BUTTON", tr(L"DNS cho card mạng đã chọn", L"DNS for the selected adapter"), BS_GROUPBOX, 0, 0, 0, 0, ID_NET_GROUP);
    app_control(window, L"STATIC", tr(L"Giao thức", L"Protocol"), 0, 0, 0, 0, 0, ID_NET_FAMILY_LABEL);
    HWND family = app_control(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, ID_NET_FAMILY);
    SendMessageW(family, CB_ADDSTRING, 0, (LPARAM)L"IPv4");
    SendMessageW(family, CB_ADDSTRING, 0, (LPARAM)L"IPv6");
    SendMessageW(family, CB_SETCURSEL, 0, 0);
    app_control(window, L"STATIC", tr(L"Nhà cung cấp", L"Provider"), 0, 0, 0, 0, 0, ID_NET_PROVIDER_LABEL);
    HWND provider = app_control(window, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, ID_NET_PROVIDER);
    const wchar_t *providers[] = {L"Cloudflare", L"Google", L"Quad9", tr(L"Tùy chỉnh", L"Custom")};
    for (size_t i = 0; i < ARRAYSIZE(providers); ++i) SendMessageW(provider, CB_ADDSTRING, 0, (LPARAM)providers[i]);
    SendMessageW(provider, CB_SETCURSEL, 0, 0);
    app_control(window, L"STATIC", tr(L"DNS chính", L"Primary DNS"), 0, 0, 0, 0, 0, ID_NET_PRIMARY_LABEL);
    app_control(window, L"STATIC", tr(L"DNS phụ (không bắt buộc)", L"Secondary DNS (optional)"), 0, 0, 0, 0, 0, ID_NET_SECONDARY_LABEL);
    HWND primary = app_control(window, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 0, 0, ID_NET_PRIMARY);
    HWND secondary = app_control(window, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 0, 0, ID_NET_SECONDARY);
    SendMessageW(primary, EM_SETLIMITTEXT, 63, 0);
    SendMessageW(secondary, EM_SETLIMITTEXT, 63, 0);
    app_control(window, L"BUTTON", tr(L"Áp dụng DNS…", L"Apply DNS…"), WS_TABSTOP, 0, 0, 0, 0, ID_NET_APPLY);
    app_control(window, L"BUTTON", tr(L"DNS tự động (DHCP)…", L"Automatic DNS (DHCP)…"), WS_TABSTOP, 0, 0, 0, 0, ID_NET_AUTO);
    app_control(window, L"STATIC", tr(L"Chỉ giao thức đã chọn thay đổi. Chọn IPv4 và IPv6 lần lượt nếu muốn đổi cả hai.",
        L"Only the selected protocol changes. Apply separately to IPv4 and IPv6 to change both."), 0, 0, 0, 0, 0, ID_NET_NOTE);
    app_control(window, L"BUTTON", tr(L"Xóa bộ nhớ đệm DNS", L"Flush DNS cache"), WS_TABSTOP, 0, 0, 0, 0, ID_NET_FLUSH);
    app_control(window, L"BUTTON", tr(L"Đặt lại TCP/IP…", L"Reset TCP/IP…"), WS_TABSTOP, 0, 0, 0, 0, ID_NET_RESET);
    app_control(window, L"BUTTON", tr(L"Khôi phục Wi-Fi…", L"Restore Wi-Fi…"), WS_TABSTOP, 0, 0, 0, 0, ID_NET_WIFI);
    app_control(window, L"STATIC", L"", 0, 0, 0, 0, 0, ID_NET_STATUS);
    populate_provider(window);
    network_layout(window);
}

static void translate_network(NetworkWindow *state) {
    HWND window = state->window;
    SetWindowTextW(window, tr(L"Thiết lập mạng — Win AIO Optimize", L"Network setup — Win AIO Optimize"));
    static const struct { int id; const wchar_t *vi, *en; } labels[] = {
        {ID_NET_HEADING, L"Card mạng & DNS", L"Adapters & DNS"},
        {ID_NET_HINT, L"Chọn card mạng, sau đó chọn giao thức và nhà cung cấp DNS.", L"Select an adapter, then choose the protocol and DNS provider."},
        {ID_NET_REFRESH, L"Tải lại", L"Refresh"},
        {ID_NET_GROUP, L"DNS cho card mạng đã chọn", L"DNS for the selected adapter"},
        {ID_NET_FAMILY_LABEL, L"Giao thức", L"Protocol"},
        {ID_NET_PROVIDER_LABEL, L"Nhà cung cấp", L"Provider"},
        {ID_NET_PRIMARY_LABEL, L"DNS chính", L"Primary DNS"},
        {ID_NET_SECONDARY_LABEL, L"DNS phụ (không bắt buộc)", L"Secondary DNS (optional)"},
        {ID_NET_APPLY, L"Áp dụng DNS…", L"Apply DNS…"},
        {ID_NET_AUTO, L"DNS tự động (DHCP)…", L"Automatic DNS (DHCP)…"},
        {ID_NET_NOTE, L"Chỉ giao thức đã chọn thay đổi. Chọn IPv4 và IPv6 lần lượt nếu muốn đổi cả hai.", L"Only the selected protocol changes. Apply separately to IPv4 and IPv6 to change both."},
        {ID_NET_FLUSH, L"Xóa bộ nhớ đệm DNS", L"Flush DNS cache"},
        {ID_NET_RESET, L"Đặt lại TCP/IP…", L"Reset TCP/IP…"},
        {ID_NET_WIFI, L"Khôi phục Wi-Fi…", L"Restore Wi-Fi…"}
    };
    for (size_t i = 0; i < ARRAYSIZE(labels); ++i)
        SetDlgItemTextW(window, labels[i].id, tr(labels[i].vi, labels[i].en));
    const wchar_t *columns[] = {tr(L"Card mạng", L"Adapter"), tr(L"Trạng thái", L"Status"),
        tr(L"Địa chỉ IP", L"IP addresses"), L"MAC", tr(L"Máy chủ DNS", L"DNS servers")};
    for (int i = 0; i < (int)ARRAYSIZE(columns); ++i) {
        LVCOLUMNW column = {0}; column.mask = LVCF_TEXT; column.pszText = (wchar_t *)columns[i];
        ListView_SetColumn(state->list, i, &column);
    }
    HWND provider = GetDlgItem(window, ID_NET_PROVIDER);
    LRESULT selected = SendMessageW(provider, CB_GETCURSEL, 0, 0);
    SendMessageW(provider, CB_DELETESTRING, 3, 0);
    SendMessageW(provider, CB_INSERTSTRING, 3, (LPARAM)tr(L"Tùy chỉnh", L"Custom"));
    SendMessageW(provider, CB_SETCURSEL, selected, 0);
    for (size_t i = 0; i < state->row_count; ++i) {
        StringCchCopyW(state->rows[i].status, ARRAYSIZE(state->rows[i].status), adapter_status(state->rows[i].oper_status));
        ListView_SetItemText(state->list, (int)i, 1, state->rows[i].status);
    }
    SetDlgItemTextW(window, ID_NET_STATUS,
        state->status_kind == 1 ? tr(L"Tải lại thất bại; thông tin có thể đã cũ.", L"Refresh failed; information may be outdated.") :
        state->status_kind == 2 ? tr(L"Thao tác có lỗi. Xem nhật ký.", L"Operation had errors. See the log.") :
        tr(L"Sẵn sàng. Chọn card mạng trước khi đổi DNS.", L"Ready. Select an adapter before changing DNS."));
}

static LRESULT CALLBACK network_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    NetworkWindow *state = (NetworkWindow *)GetWindowLongPtrW(window, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE:
        state = (NetworkWindow *)calloc(1, sizeof(*state));
        if (!state) return -1;
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)state);
        theme_attach(window);
        create_network_controls(state);
        if (!app_is_smoke_test()) PostMessageW(window, WM_COMMAND, ID_NET_REFRESH, 0);
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO *limits = (MINMAXINFO *)lparam;
        limits->ptMinTrackSize.x = 850;
        limits->ptMinTrackSize.y = 650;
        return 0;
    }
    case WM_SIZE:
        if (state && wparam != SIZE_MINIMIZED) network_layout(window);
        return 0;
    case WM_COMMAND:
        if (!state || app_busy(NULL)) return 0;
        switch (LOWORD(wparam)) {
        case ID_NET_REFRESH: request_operation(state, NET_REFRESH); break;
        case ID_NET_FLUSH: request_operation(state, NET_FLUSH); break;
        case ID_NET_RESET: request_operation(state, NET_RESET); break;
        case ID_NET_APPLY: request_dns(state, FALSE); break;
        case ID_NET_AUTO: request_dns(state, TRUE); break;
        case ID_NET_WIFI: restore_wifi(window); break;
        case ID_NET_PROVIDER:
        case ID_NET_FAMILY:
            if (HIWORD(wparam) == CBN_SELCHANGE) populate_provider(window);
            break;
        case ID_NET_PRIMARY:
        case ID_NET_SECONDARY:
            if (HIWORD(wparam) == EN_CHANGE && GetFocus() == GetDlgItem(window, LOWORD(wparam)))
                SendDlgItemMessageW(window, ID_NET_PROVIDER, CB_SETCURSEL, 3, 0);
            break;
        }
        return 0;
    case WM_APP_JOB_DONE:
        if (state) network_complete(state);
        return 0;
    case WM_APP_LANGUAGE:
        if (state) translate_network(state);
        return 0;
    case WM_CLOSE:
        if (!app_busy(window) && IsWindowEnabled(window)) DestroyWindow(window);
        return 0;
    case WM_NCDESTROY:
        if (state) { free(state->rows); free(state); }
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        if (network_window == window) network_window = NULL;
        break;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

void network_show(HWND owner) {
    if (network_window && IsWindow(network_window)) {
        ShowWindow(network_window, SW_RESTORE);
        SetForegroundWindow(network_window);
        return;
    }
    WNDCLASSEXW cls = {0};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = network_proc;
    cls.hInstance = g_instance;
    cls.hCursor = LoadCursorW(NULL, IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszClassName = L"WAIONetworkWindow";
    if (!RegisterClassExW(&cls) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        app_error(owner, tr(L"Cửa sổ mạng", L"Network window"), GetLastError());
        return;
    }
    network_window = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName,
        tr(L"Thiết lập mạng — Win AIO Optimize", L"Network setup — Win AIO Optimize"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1030, 720,
        owner, NULL, g_instance, NULL);
    if (!network_window) { app_error(owner, tr(L"Cửa sổ mạng", L"Network window"), GetLastError()); return; }
    ShowWindow(network_window, app_is_smoke_test() ? SW_HIDE : SW_SHOW);
    UpdateWindow(network_window);
}

BOOL network_ui_test(HWND owner) {
    if (!app_is_smoke_test()) return FALSE;
    network_show(owner);
    if (!IsWindow(network_window)) return FALSE;
    BOOL original = g_vi, ok = TRUE;
    HWND window = network_window;
    SetDlgItemTextW(window, ID_NET_PRIMARY, L"fd00::53");
    SetDlgItemTextW(window, ID_NET_SECONDARY, L"fd00::54");
    SendDlgItemMessageW(window, ID_NET_FAMILY, CB_SETCURSEL, 1, 0);
    SendDlgItemMessageW(window, ID_NET_PROVIDER, CB_SETCURSEL, 3, 0);
    NetworkWindow *state = (NetworkWindow *)GetWindowLongPtrW(window, GWLP_USERDATA);
    NetworkJob inventory = {0};
    inventory.rows = calloc(1, sizeof(*inventory.rows));
    if (!inventory.rows) { DestroyWindow(window); return FALSE; }
    inventory.row_count = 1;
    inventory.rows[0].oper_status = IfOperStatusUp;
    inventory.rows[0].luid.Value = 1;
    StringCchCopyW(inventory.rows[0].name, ARRAYSIZE(inventory.rows[0].name), L"Test adapter");
    show_rows(state, &inventory);
    ListView_SetItemState(state->list, 0, LVIS_SELECTED, LVIS_SELECTED);
    for (int lang = 0; lang < 2; ++lang) {
        wchar_t text[128]; g_vi = lang != 0;
        SendMessageW(window, WM_APP_LANGUAGE, 0, 0);
        GetDlgItemTextW(window, ID_NET_PRIMARY, text, ARRAYSIZE(text));
        if (wcscmp(text, L"fd00::53")) ok = FALSE;
        GetDlgItemTextW(window, ID_NET_SECONDARY, text, ARRAYSIZE(text));
        if (wcscmp(text, L"fd00::54")) ok = FALSE;
        GetDlgItemTextW(window, ID_NET_HEADING, text, ARRAYSIZE(text));
        if (wcscmp(text, tr(L"Card mạng & DNS", L"Adapters & DNS"))) ok = FALSE;
        ListView_GetItemText(state->list, 0, 1, text, ARRAYSIZE(text));
        if (wcscmp(text, tr(L"Đã kết nối", L"Connected"))) ok = FALSE;
        if (SendDlgItemMessageW(window, ID_NET_FAMILY, CB_GETCURSEL, 0, 0) != 1 ||
            SendDlgItemMessageW(window, ID_NET_PROVIDER, CB_GETCURSEL, 0, 0) != 3 ||
            ListView_GetNextItem(state->list, -1, LVNI_SELECTED) != 0) ok = FALSE;
    }
    g_vi = original; DestroyWindow(window);
    return ok;
}
