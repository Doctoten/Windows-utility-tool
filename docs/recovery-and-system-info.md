# Recovery, appearance and system information

## Recovery before configuration changes

Optimizer apply/restore, optional-app removal, DNS changes, TCP/IP reset and WiFi profile import use the recovery confirmation. Read-only inventory, backup exports, DNS-cache flushing and shutdown scheduling do not create recovery points.

The confirmation presents **Create a registry backup (Recommended)** and **Create a system restore point (Recommended)**, initially checked. Optimization choices themselves remain unchecked until selected. Cancel remains the default action. Checking recovery options authorizes those operations only after **I agree** is pressed.

The selected recovery steps run on a worker thread while the dialog displays progress. Registry backup precedes the restore-point request. If either selected step fails, the app does not start the requested configuration change. The dialog reports the error and allows retry, cancellation, or an explicit decision to uncheck an unavailable protection. A failed attempt can leave completed or partial backup files; their path is logged.

Registry backup uses [RegSaveKeyExW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regsavekeyexw) to save **HKLM\SYSTEM, HKLM\SOFTWARE and the running account's HKCU** as binary `.hiv` files. They are stored in a unique `Registry-{GUID}` directory under `%LOCALAPPDATA%\WinAIOOptimize`, with access restricted to the owner, administrators and SYSTEM. `BACKUP-COMPLETE.txt` is written only when all three exports succeed. Backup privilege is restored after export.

This is a backup of the named registry hives, not a full system-state backup: it excludes other users, SAM/SECURITY, volatile keys and personal files. Binary hives cannot be double-clicked like `.reg` exports. Prefer **Restore last settings** for changes made by the optimizer. Whole-hive recovery requires an experienced administrator, normally in Windows Recovery Environment; the app deliberately does not replace live system hives.

Restore points use [SRSetRestorePointW](https://learn.microsoft.com/en-us/windows/win32/api/srrestoreptapi/nf-srrestoreptapi-srsetrestorepointw) through a dynamically loaded system `srclient.dll`, with paired begin/end calls to complete a checkpoint before applying the requested configuration. [COM callback security](https://learn.microsoft.com/en-us/windows/win32/sr/using-system-restore) is initialized for the local Windows services involved. System Protection must already be enabled. The app exposes a shortcut to Windows System Protection; it does not silently enable the feature or change restore-point frequency policy.

Windows may reuse a recent point under its creation-frequency policy (24 hours by default). A successful request is logged as a Windows-confirmed sequence number, not an unconditional claim that a brand-new point was created. Disabled System Protection, missing components, insufficient space and API failures block the selected changes until the user resolves the problem or changes their selection. Restore points do not replace personal-file backups or guarantee recovery of removed app data.

## Light and dark appearance

The default **Windows theme** option follows `AppsUseLightTheme`. **Light** and **Dark** allow an app-only override stored in the app's HKCU preference key. Theme changes repaint the dashboard, app dialogs, buttons, list headers, lists, edits and selectors without rebuilding controls or clearing their data. Windows setting changes are handled while the app is open. High Contrast uses the user's system colors instead of the custom palette.

The app requests the documented [DWM dark title bar and rounded corners](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute) on Windows 11. Windows-owned file pickers, message boxes and some native chrome may retain the system's appearance.

## System information

The dashboard shows computer name, CPU, Windows build and physical/available RAM. **System information** provides a categorized, refreshable inventory with a selectable full-value view and an explicit **Copy row** action. Refresh runs in a background worker; data represents the most recent refresh, not continuous monitoring.

Read-only WMI queries cover the computer/model/account, Windows version/build/install/boot times, CPU cores/threads/clock/virtualization, individual RAM modules, GPU/driver/display mode, physical disks, local volumes/free space, motherboard, BIOS, IP-enabled adapters/DNS/DHCP and battery. Native APIs add firmware mode and reported Secure Boot state. [DXGI adapter descriptions](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc1) provide dedicated/shared video-memory sizes without the 32-bit WMI `AdapterRAM` limitation.

Missing devices and properties are shown as unavailable. Provider failures are reported with an error code and partial results remain visible. Enumeration has bounded row counts and waits. Driver/firmware values can be incomplete; disk status is Windows-reported status, not a SMART health diagnosis. Dates and numeric provider enums are explicitly labeled with their format. The inventory does not collect passwords or product keys, and hardware identifiers are not written to the activity log or sent over the network.

## Tiếng Việt

Trước khi áp dụng/khôi phục tối ưu, gỡ ứng dụng, đổi DNS, đặt lại TCP/IP hoặc nhập hồ sơ WiFi, hộp xác nhận đề xuất **Tạo bản sao Registry (Khuyên dùng)** và **Tạo điểm khôi phục hệ thống (Khuyên dùng)**. Hai lựa chọn bảo vệ được đánh dấu sẵn; các mục tối ưu vẫn mặc định bỏ chọn. Chỉ chạy sau khi nhấn **Tôi đồng ý**. Nếu bước bảo vệ đã chọn thất bại, ứng dụng dừng trước thay đổi và cho phép thử lại, hủy hoặc tự bỏ chọn mục không thể tạo.

Registry được lưu dưới dạng `.hiv` cho SYSTEM, SOFTWARE và HKCU của tài khoản chạy app, trong thư mục `Registry-{GUID}` riêng tại `%LOCALAPPDATA%\WinAIOOptimize`. File `BACKUP-COMPLETE.txt` đánh dấu ba phần đã được xuất thành công. Đây không phải bản sao toàn bộ trạng thái máy hoặc dữ liệu cá nhân. Khôi phục nguyên hive cần người có chuyên môn, thường qua Windows Recovery Environment; với các tối ưu của app, ưu tiên **Khôi phục cài đặt**.

System Protection phải được bật từ trước. Nút **Bảo vệ hệ thống…** mở thiết lập Windows. App không tự bật bảo vệ hoặc thay đổi giới hạn tạo điểm khôi phục. Windows có thể dùng lại điểm gần đây theo chính sách tần suất; nhật ký ghi số thứ tự được Windows xác nhận. Điểm khôi phục không thay thế sao lưu file cá nhân hoặc bảo đảm lấy lại dữ liệu ứng dụng đã gỡ.

Giao diện mặc định **Theo Windows**, hỗ trợ chọn riêng **Sáng** hoặc **Tối**, giữ nguyên dữ liệu nhập khi đổi. Chế độ tương phản cao sử dụng màu hệ thống. Hộp thoại do Windows quản lý có thể giữ giao diện của hệ điều hành.

Trang **Thông tin máy** có bộ lọc nhóm, làm mới và sao chép dòng: máy/model, Windows, CPU, từng thanh RAM, GPU/VRAM/driver, ổ đĩa và phân vùng, mainboard, BIOS, mạng/DNS, pin, UEFI và Secure Boot. Dữ liệu là trạng thái tại lần làm mới. Mục không có hoặc không đọc được sẽ được ghi rõ; thông tin do driver cung cấp có thể không đầy đủ. Không thu thập mật khẩu/key sản phẩm và không tự ghi định danh phần cứng vào nhật ký hoặc gửi ra mạng.
