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

### 1) Sao lưu & Khôi phục | Backup & Restore
| Chức năng | Mô tả |
| :--- | :--- |
| 📶 **Sao lưu WiFi** | VI: Sao lưu tất cả WiFi đã lưu (XML + mật khẩu) vào thư mục con `WiFi_Backup_YYYYMMDD_HHMMSS`. EN: Backup all saved WiFi profiles (XML + passwords) into `WiFi_Backup_YYYYMMDD_HHMMSS` under the chosen folder. |
| 💾 **Sao lưu Driver** | Tạo một bản sao lưu toàn bộ driver của hệ thống, rất hữu ích khi cài lại Windows (đang phát triển). |

### 2) Thiết lập & Tối ưu hóa | Setup & Optimize
| Chức năng | Mô tả |
| :--- | :--- |
| 🔧 **Thiết lập Windows** | VI: Mở cửa sổ chạy script Chris Titus Tech: Stable/Dev (PowerShell mới). EN: Opens a window to run Chris Titus Tech scripts: Stable/Dev (new PowerShell). |
| 🗑️ **Xóa Bloatware** | Gỡ bỏ các ứng dụng không cần thiết được cài sẵn trên Windows (đang phát triển). |

### 3) Công cụ mạng | Network Tools
Nhấn nút **"Thiết lập mạng"** để mở cửa sổ công cụ mạng chuyên dụng.



| Chức năng | Mô tả |
| :--- | :--- |
| 📋 **Xem thông tin mạng** | VI: Liệt kê card mạng cùng IP/MAC/DNS. EN: List interfaces with IP/MAC/DNS. |
| 🔃 **Xóa Cache DNS** | Thực hiện lệnh `ipconfig /flushdns` để xóa bộ nhớ đệm DNS, giúp khắc phục các lỗi truy cập website. |
| ⚙️ **Reset TCP/IP** | Thực hiện lệnh `netsh int ip reset` để đặt lại toàn bộ chồng giao thức mạng, sửa các lỗi kết nối phức tạp. **Yêu cầu khởi động lại máy tính.** |
| 📲 **Đổi DNS nhanh** | VI: Chuyển DNS sang Google/Cloudflare hoặc DHCP. EN: Switch to Google/Cloudflare or DHCP. |
| 📥 **Khôi phục WiFi** | VI: Khôi phục từ file `.xml` đã sao lưu. EN: Restore from saved `.xml` profiles. |

### 4) Hẹn giờ tắt máy | Shutdown Timer
- VI: Đặt thời gian tắt máy theo phút/giờ; lưu lịch vào `%LOCALAPPDATA%\WindowsUtilityTool\shutdown_schedule.json` để hiển thị còn lại khi mở app.
- EN: Schedule shutdown in minutes/hours; persists to `%LOCALAPPDATA%\WindowsUtilityTool\shutdown_schedule.json` to display remaining time on next start.

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
