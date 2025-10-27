# 🛠️ Windows Utility Tool – Hướng dẫn sử dụng | User Guide (v5.4)

<p align="center">
  <img src="download.gif" alt="Demo" />
  
  <em>Giao diện chính | Main UI – Windows Utility Tool v5.4</em>
</p>

## Giới thiệu | Introduction
VI: **Windows Utility Tool** là công cụ miễn phí giúp thực hiện các tác vụ quản trị, tối ưu và sao lưu hệ thống nhanh chóng, an toàn. Giao diện trực quan, hỗ trợ đa ngôn ngữ (Tiếng Việt/English). Ứng dụng dạng **Portable** (không cần cài đặt) và hỗ trợ **chạy một phiên bản duy nhất**.

EN: **Windows Utility Tool** is a free utility for quick and safe system administration, optimization, and backup on Windows. It features a clean UI with bilingual support (Vietnamese/English). It’s **portable** (no installation needed) and enforces **single-instance** execution.

---

## Yêu cầu hệ thống | System Requirements
- VI: Windows 10/11; cần quyền **Administrator** (ứng dụng tự yêu cầu UAC).
- EN: Windows 10/11; requires **Administrator** privileges (UAC prompt on start).

---

## Tải nhanh qua PowerShell | Quick Download via PowerShell
VI/EN: Mở PowerShell và chạy | Open PowerShell and run:

```powershell
Invoke-WebRequest -Uri "https://tinyurl.com/Doctoten" -OutFile "$env:USERPROFILE\Downloads\WindowsUtilityTool.exe"
```

VI: Lệnh sẽ tải `WindowsUtilityTool.exe` vào thư mục **Downloads**. Chạy file từ đó.

EN: The command downloads `WindowsUtilityTool.exe` to your **Downloads** folder. Run it from there.

---

## Hướng dẫn sử dụng | How to Use
VI:
1. Giải nén `Windows-utility-tool.zip` (nếu dùng bản zip).
2. Mở `Windows-utility-tool/code/dist/` và chạy `windows_utility_tool.exe`.
3. Nếu SmartScreen cảnh báo: chọn “More info” -> “Run anyway”.
4. Luôn chạy với quyền Administrator.

EN:
1. Extract `Windows-utility-tool.zip` (if using the zip package).
2. Go to `Windows-utility-tool/code/dist/` and run `windows_utility_tool.exe`.
3. If SmartScreen warns: click “More info” -> “Run anyway”.
4. Always run as Administrator.

---

## Các chức năng chính | Key Features

### Các chức năng (Tiếng Việt)
| Chức năng | Mô tả |
| :--- | :--- |
| 📶 Sao lưu WiFi | Sao lưu tất cả WiFi (XML + mật khẩu) vào thư mục con `WiFi_Backup_YYYYMMDD_HHMMSS`. |
| 💾 Sao lưu Driver | Tạo bản sao toàn bộ driver (đang phát triển). |
| 🔧 Thiết lập Windows | Mở cửa sổ chạy script Chris Titus Tech: Stable/Dev (PowerShell mới). |
| 🗑️ Xóa Bloatware | Gỡ các ứng dụng không cần thiết cài sẵn. |
| 📋 Xem thông tin mạng | Liệt kê card mạng với IP/MAC/DNS; có nút Tải lại. |
| 🔃 Xóa Cache DNS | Thực hiện `ipconfig /flushdns`. |
| ⚙️ Reset TCP/IP | Thực hiện `netsh int ip reset`; yêu cầu khởi động lại. |
| 📲 Đổi DNS nhanh | Chuyển DNS sang Google/Cloudflare hoặc về DHCP. |
| 📥 Khôi phục WiFi | Khôi phục từ các file `.xml` đã sao lưu. |
| ⏱️ Hẹn giờ tắt máy | Đặt phút/giờ; lưu lịch tại `%LOCALAPPDATA%\WindowsUtilityTool\shutdown_schedule.json` và hiển thị thời gian còn lại khi mở app. |

### Key Features (English)
| Feature | Description |
| :--- | :--- |
| 📶 Backup WiFi | Backs up all saved WiFi (XML + passwords) into `WiFi_Backup_YYYYMMDD_HHMMSS` subfolder. |
| 💾 Backup Drivers | Full driver backup (in development). |
| 🔧 Windows Setup | Opens a window to run Chris Titus Tech scripts: Stable/Dev (new PowerShell). |
| 🗑️ Remove Bloatware | Remove preinstalled unnecessary apps. |
| 📋 View Network Info | List interfaces with IP/MAC/DNS; Refresh button. |
| 🔃 Flush DNS Cache | Runs `ipconfig /flushdns`. |
| ⚙️ Reset TCP/IP | Runs `netsh int ip reset`; requires reboot. |
| 📲 Quick DNS Change | Switch to Google/Cloudflare or back to DHCP. |
| 📥 Restore WiFi | Restore from saved `.xml` profiles. |
| ⏱️ Shutdown Timer | Schedule minutes/hours; persists to `%LOCALAPPDATA%\WindowsUtilityTool\shutdown_schedule.json` to display remaining time. |

---

## Hướng dẫn chi tiết: Sao lưu & Khôi phục WiFi | Detailed Guide: WiFi Backup & Restore

### A) Sao lưu | Backup
VI:
1. Nhấn "Sao lưu WiFi".
2. Chọn thư mục đích.
3. App tạo thư mục con `WiFi_Backup_YYYYMMDD_HHMMSS` và lưu toàn bộ file XML cùng `WiFi_Passwords.txt` vào đó.

EN:
1. Click "Backup WiFi".
2. Choose a destination folder.
3. The app creates `WiFi_Backup_YYYYMMDD_HHMMSS` subfolder with all XML files and `WiFi_Passwords.txt`.

### B) Khôi phục | Restore
VI: Mở "Thiết lập mạng" -> chọn file `.xml` đã sao lưu -> Open.

EN: Open "Network Setup" -> select saved `.xml` files -> Open.

---

## Xử lý sự cố | Troubleshooting
- VI/EN: Luôn chạy ứng dụng với quyền Administrator (UAC). If a feature fails: try Run as administrator.
- VI/EN: Nếu không tìm thấy Card WiFi trên máy ảo, cần gắn USB WiFi passthrough.
- VI/EN: Nếu app không mở, antivirus có thể chặn; thêm vào exception.

---
**Phát triển bởi | Developed by: Doctoten**
