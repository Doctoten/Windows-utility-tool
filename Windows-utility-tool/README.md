# 🛠️ Tiện ích hỗ trợ cài Windows - Hướng dẫn sử dụng (v3.1)

<p align="center">
  <img src="download.gif" alt="Demo" />
</p>

## Giới thiệu
**Windows Utility Tool** là một công cụ miễn phí, được thiết kế để giúp người dùng Windows thực hiện các tác vụ quản trị, tối ưu và sao lưu hệ thống một cách nhanh chóng và an toàn. Với giao diện trực quan và hoàn toàn bằng tiếng Việt, ứng dụng này là người bạn đồng hành đáng tin cậy sau mỗi lần cài đặt lại Windows hoặc khi cần "dọn dẹp" máy tính.

Vì đây là bản đầu tiên nên sẽ có lỗi và sẽ update nhiều tính năng trong tương lai và sẽ update nhiều ngôn ngữ khác


Ứng dụng được đóng gói dưới dạng **Portable**, nghĩa là bạn không cần cài đặt bất cứ thứ gì.

---

## Yêu cầu hệ thống
- **Hệ điều hành**: Windows 10, Windows 11.
- **Quyền hạn**: **Administrator**. Ứng dụng được tích hợp sẵn cơ chế tự động yêu cầu quyền Admin khi khởi động để đảm bảo mọi chức năng hoạt động chính xác.

---

## Hướng dẫn sử dụng
### Khởi động ứng dụng
1.  Tải và giải nén tệp tin `Windows-utility-tool.zip` (hoặc thư mục dự án) vào một nơi bất kỳ.
2.  Đi theo đường dẫn: `Windows-utility-tool/code/dist/`.
3.  Bên trong thư mục `dist`, bạn sẽ tìm thấy file `windows_utility_tool.exe`.
4.  **Click chuột phải** vào file `windows_utility_tool.exe` và chọn **"Run as administrator"**.

> **Lưu ý quan trọng**: Luôn chạy ứng dụng với quyền Administrator để tránh các lỗi không mong muốn, đặc biệt là với các chức năng can thiệp sâu vào hệ thống.

---

## Các chức năng chính
| Chức năng | Mô tả |
| :--- | :--- |
| 🔧 **Thiết lập Windows** | Tinh chỉnh các cài đặt hệ thống phổ biến (đang phát triển). |
| 🌐 **Thiết lập mạng** | Sửa các lỗi kết nối mạng thông thường (đang phát triển). |
| 🗑️ **Xóa Bloatware** | Gỡ bỏ các ứng dụng không cần thiết được cài sẵn trên Windows (đang phát triển). |
| 💾 **Sao lưu Driver** | Tạo một bản sao lưu toàn bộ driver của hệ thống, rất hữu ích khi cài lại Windows. |
| 📶 **Sao lưu WiFi** | Tự động quét và sao lưu tất cả các mạng WiFi đã từng kết nối, bao gồm tên mạng và mật khẩu. |

---

## Hướng dẫn chi tiết: Sao lưu & Khôi phục WiFi
### A. Cách Sao lưu
1.  Nhấn nút "Sao lưu WiFi".
2.  Chọn một thư mục để lưu trữ các tệp tin sao lưu.
3.  Ứng dụng sẽ tự động tạo một thư mục con tên là `WiFi_Backup` bên trong đó.
4.  Kết quả trong thư mục `WiFi_Backup`:
    -   **Các file `.xml`**: Mỗi file tương ứng với một mạng WiFi. Đây là file dùng để **khôi phục (restore)** cấu hình mạng.
    -   **File `WiFi_Passwords.txt`**: Một file văn bản tổng hợp tên và mật khẩu của tất cả các mạng, giúp bạn xem lại một cách nhanh chóng.

### B. Cách Khôi phục
Khi bạn cài lại Windows hoặc muốn chuyển danh sách WiFi sang máy tính khác:
1.  Mở **Command Prompt** với quyền **Administrator**.
2.  Gõ lệnh sau và nhấn Enter:
    ```bash
    netsh wlan add profile filename="Đường_dẫn_đầy_đủ_đến_file.xml"
    ```
    *Ví dụ:* `netsh wlan add profile filename="D:\WiFi_Backup\Wi-Fi-MyHome.xml"`
3.  Lặp lại lệnh trên cho tất cả các file `.xml` bạn muốn khôi phục.

---

## Xử lý sự cố (Troubleshooting)
- **Lỗi "Access Denied" hoặc chức năng không hoạt động**: Bạn chưa chạy ứng dụng với quyền "Run as administrator".
- **Ứng dụng không khởi động**: Có thể bị phần mềm diệt virus nhận nhầm. Hãy thử thêm ứng dụng vào danh sách loại trừ (exception list) của phần mềm diệt virus.
- **Không tìm thấy profile WiFi**: Đảm bảo máy tính của bạn đã từng kết nối WiFi và bạn đang chạy với quyền Admin.

---
**Phát triển bởi: Doctoten**