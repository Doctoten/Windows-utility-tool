# 🛠️ Tiện ích hỗ trợ cài Windows - Hướng dẫn sử dụng (v4.4)

<p align="center">
  <img src="download.gif" alt="Demo" />
  
  <em>Giao diện chính của Windows Utility Tool v4.4</em>
</p>

## Giới thiệu
**Windows Utility Tool** là một công cụ miễn phí, được thiết kế để giúp người dùng Windows thực hiện các tác vụ quản trị, tối ưu và sao lưu hệ thống một cách nhanh chóng và an toàn. Với giao diện trực quan và hoàn toàn bằng tiếng Việt, ứng dụng này là người bạn đồng hành đáng tin cậy sau mỗi lần cài đặt lại Windows hoặc khi cần "dọn dẹp" máy tính.

Ứng dụng được đóng gói dưới dạng **Portable**, nghĩa là bạn không cần cài đặt. Nó cũng được thiết kế để **không chạy nhiều phiên bản cùng lúc**, tránh gây nhầm lẫn cho người dùng.

---

## Yêu cầu hệ thống
- **Hệ điều hành**: Windows 10, Windows 11.
- **Quyền hạn**: **Administrator**. Ứng dụng tích hợp cơ chế tự động yêu cầu quyền Admin khi khởi động.

---

## Cài đặt nhanh qua PowerShell
Bạn có thể tải phiên bản mới nhất về máy tính một cách nhanh chóng bằng cách mở **PowerShell** và dán vào dòng lệnh sau:

```powershell
Invoke-WebRequest -Uri "https://tinyurl.com/Doctoten" -OutFile "$env:USERPROFILE\Downloads\WindowsUtilityTool.exe"
```

> **Lưu ý**: Lệnh trên sẽ tự động tải file `WindowsUtilityTool.exe` vào thư mục **Downloads** của bạn. Sau khi tải xong, bạn có thể chạy file từ đó.

---

## Hướng dẫn sử dụng
1.  Tải và giải nén tệp `Windows-utility-tool.zip`.
2.  Đi tới thư mục `Windows-utility-tool/code/dist/`.
3.  Chạy file `windows_utility_tool.exe`.
4.  Nếu Windows hiện cảnh báo SmartScreen, hãy chọn "More info" -> "Run anyway".

> **Lưu ý quan trọng**: Luôn chạy ứng dụng với quyền Administrator. Nếu bạn hủy hộp thoại UAC, ứng dụng sẽ không khởi động.

---

## Các chức năng chính

### 1. Sao lưu & Khôi phục
| Chức năng | Mô tả |
| :--- | :--- |
| 📶 **Sao lưu Wifi** | Tự động quét và sao lưu tất cả các mạng WiFi đã lưu (tên mạng và mật khẩu) vào một thư mục do bạn chọn. |
| 💾 **Sao lưu Driver** | Tạo một bản sao lưu toàn bộ driver của hệ thống, rất hữu ích khi cài lại Windows (đang phát triển). |

### 2. Thiết lập & Tối ưu hóa
| Chức năng | Mô tả |
| :--- | :--- |
| 🔧 **Thiết lập Windows** | Tinh chỉnh các cài đặt hệ thống phổ biến (đang phát triển). |
| 🗑️ **Xóa Bloatware** | Gỡ bỏ các ứng dụng không cần thiết được cài sẵn trên Windows (đang phát triển). |

### 3. Công cụ mạng
Nhấn nút **"Thiết lập mạng"** để mở cửa sổ công cụ mạng chuyên dụng.

<p align="center">
  <img src="https://i.imgur.com/your_network_window_image.png" alt="Cửa sổ Thiết lập mạng" width="700"/>
  <em>Cửa sổ quản lý và thiết lập mạng.</em>
</p>

| Chức năng | Mô tả |
| :--- | :--- |
| 📋 **Xem thông tin mạng** | Hiển thị danh sách các card mạng trên hệ thống, cùng với địa chỉ IP, địa chỉ MAC và **DNS Servers** hiện tại. Có nút **Tải lại** để cập nhật thông tin ngay lập tức. |
| 🔃 **Xóa Cache DNS** | Thực hiện lệnh `ipconfig /flushdns` để xóa bộ nhớ đệm DNS, giúp khắc phục các lỗi truy cập website. |
| ⚙️ **Reset TCP/IP** | Thực hiện lệnh `netsh int ip reset` để đặt lại toàn bộ chồng giao thức mạng, sửa các lỗi kết nối phức tạp. **Yêu cầu khởi động lại máy tính.** |
| 📲 **Đổi DNS nhanh** | Cho phép đổi DNS của card mạng được chọn sang Google DNS, Cloudflare DNS, hoặc trả về chế độ tự động (DHCP). |
| 📥 **Khôi phục WiFi** | Khôi phục lại các cấu hình mạng WiFi từ những file `.xml` đã được sao lưu trước đó. |

---

## Hướng dẫn chi tiết: Sao lưu & Khôi phục WiFi

### A. Cách Sao lưu
1.  Nhấn nút "Sao lưu WiFi".
2.  Chọn một thư mục để lưu trữ.
3.  Ứng dụng sẽ tự động tạo một thư mục con tên là `WiFi_Backup` bên trong đó.
4.  Kết quả trong thư mục `WiFi_Backup`:
    -   **Các file `.xml`**: Mỗi file tương ứng với một mạng WiFi. Đây là file dùng để **khôi phục**.
    -   **File `WiFi_Passwords.txt`**: File văn bản tổng hợp tên và mật khẩu của tất cả các mạng để bạn xem lại nhanh chóng.

### B. Cách Khôi phục
1.  Nhấn nút "Thiết lập mạng".
2.  Trong cửa sổ mới, nhấn nút "Khôi phục WiFi từ Backup".
3.  Tìm đến thư mục chứa các file `.xml` đã sao lưu và chọn tất cả các file bạn muốn khôi phục.
4.  Nhấn "Open", ứng dụng sẽ tự động thêm lại các mạng WiFi vào máy của bạn.

---

## Xử lý sự cố (Troubleshooting)
- **Lỗi "Access Denied" hoặc chức năng không hoạt động**: Bạn chưa chạy ứng dụng với quyền "Run as administrator".
- **Chức năng WiFi báo lỗi "Không tìm thấy Card WiFi"**:
    - **Nguyên nhân**: Ứng dụng không tìm thấy card mạng không dây trên hệ thống.
    - **Đối với máy ảo (VMWare, VirtualBox)**: Đây là hành vi dự kiến. Máy ảo mặc định sử dụng card mạng có dây ảo. Để sử dụng chức năng này, bạn cần có một **USB WiFi** và kết nối nó trực tiếp vào máy ảo.
- **Ứng dụng không khởi động**: Có thể bị phần mềm diệt virus nhận nhầm. Hãy thử thêm ứng dụng vào danh sách loại trừ (exception list).

---
**Phát triển bởi: Doctoten**
