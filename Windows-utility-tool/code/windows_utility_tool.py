#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Tiện ích hỗ trợ cài Windows - Windows Installation Support Utility
Phiên bản: 3.1 (Cải tiến ghi log)
Phát triển bởi: Doctoten
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess
import os
import sys
import shutil
from pathlib import Path
import threading
import ctypes

def is_admin():
    """Kiểm tra xem ứng dụng có đang chạy với quyền Administrator không."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

class WindowsUtilityTool:
    def __init__(self, root):
        self.root = root
        self.setup_window()
        self.create_widgets()
        
    def setup_window(self):
        """Thiết lập cửa sổ chính."""
        self.root.title("🛠️ Tiện ích hỗ trợ cài Win dạo v3.1")
        self.root.geometry("520x450")
        self.root.resizable(False, False)
        
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() // 2) - (520 // 2)
        y = (self.root.winfo_screenheight() // 2) - (450 // 2)
        self.root.geometry(f"520x450+{x}+{y}")
        
        try:
            if getattr(sys, 'frozen', False):
                icon_path = os.path.join(sys._MEIPASS, 'icon.ico')
            else:
                icon_path = 'icon.ico'
            self.root.iconbitmap(icon_path)
        except:
            pass
            
        self.root.configure(bg='#f0f0f0')
        
    def create_widgets(self):
        """Tạo các widget cho giao diện."""
        header_frame = tk.Frame(self.root, bg='#2c3e50', height=80)
        header_frame.pack(fill='x', padx=0, pady=0)
        header_frame.pack_propagate(False)
        
        tk.Label(
            header_frame, text="🛠️ Tiện ích hỗ trợ cài Win dạo",
            font=('Arial', 16, 'bold'), fg='white', bg='#2c3e50'
        ).pack(pady=(15, 5))
        
        tk.Label(
            header_frame, text="Phiên bản hoàn chỉnh v3.1",
            font=('Arial', 10), fg='#ecf0f1', bg='#2c3e50'
        ).pack()
        
        content_frame = tk.Frame(self.root, bg='#f0f0f0')
        content_frame.pack(fill='both', expand=True, padx=20, pady=20)
        
        button_style = {
            'font': ('Arial', 11, 'bold'), 'width': 22, 'height': 2,
            'relief': 'raised', 'bd': 2, 'cursor': 'hand2'
        }
        
        row1_frame = tk.Frame(content_frame, bg='#f0f0f0')
        row1_frame.pack(fill='x', pady=10)
        
        tk.Button(
            row1_frame, text="🔧 Thiết lập Windows", bg='#3498db', fg='white',
            command=self.windows_setup, **button_style
        ).pack(side='left', padx=10)
        
        tk.Button(
            row1_frame, text="🌐 Thiết lập mạng", bg='#95a5a6', fg='white',
            command=self.network_setup, **button_style
        ).pack(side='right', padx=10)
        
        row2_frame = tk.Frame(content_frame, bg='#f0f0f0')
        row2_frame.pack(fill='x', pady=10)
        
        tk.Button(
            row2_frame, text="🗑️ Xóa Bloatware", bg='#e74c3c', fg='white',
            command=self.remove_bloatware, **button_style
        ).pack(side='left', padx=10)
        
        tk.Button(
            row2_frame, text="📶 Sao lưu Wifi", bg='#f39c12', fg='white',
            command=self.backup_wifi, **button_style
        ).pack(side='right', padx=10)
        
        row3_frame = tk.Frame(content_frame, bg='#f0f0f0')
        row3_frame.pack(fill='x', pady=10)

        tk.Button(
            row3_frame, text="💾 Sao lưu Driver", bg='#27ae60', fg='white',
            command=self.backup_drivers, **button_style
        ).pack(side='left', padx=10)
        
        self.status_var = tk.StringVar(value="Sẵn sàng...")
        status_frame = tk.Frame(self.root, bg='#34495e', height=30)
        status_frame.pack(fill='x', side='bottom')
        status_frame.pack_propagate(False)
        
        tk.Label(
            status_frame, textvariable=self.status_var, font=('Arial', 9),
            fg='white', bg='#34495e'
        ).pack(side='left', padx=10, pady=5)

    def run_in_thread(self, target_func, *args):
        thread = threading.Thread(target=target_func, args=args, daemon=True)
        thread.start()

    def run_command(self, command, shell=True):
        try:
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            result = subprocess.run(
                command, shell=shell, capture_output=True, text=True,
                check=False, encoding='utf-8', errors='ignore', startupinfo=startupinfo
            )
            return result.stdout + result.stderr
        except Exception as e:
            return f"Lỗi thực thi lệnh: {str(e)}"

    def update_status(self, message):
        self.status_var.set(message)
        self.root.update()
        
    def windows_setup(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")
    def network_setup(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")
    def remove_bloatware(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")
    def backup_drivers(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")

    def backup_wifi(self):
        """Chức năng sao lưu WiFi với cơ chế chỉ ghi log khi có lỗi."""
        self.update_status("Bắt đầu sao lưu WiFi...")
        self.run_in_thread(self._backup_wifi_task)

    def _backup_wifi_task(self):
        log_messages = []
        
        try:
            log_messages.append("=== Bắt đầu sao lưu WiFi v3.1 ===\n")
            
            self.update_status("Đang lấy danh sách WiFi...")
            profiles_result = self.run_command("netsh wlan show profiles")
            log_messages.append(f"Kết quả lệnh 'netsh wlan show profiles':\n{profiles_result}\n")
            
            profiles = []
            for line in profiles_result.split('\n'):
                if "All User Profile" in line:
                    try:
                        profile_name = line.split(':', 1)[1].strip()
                        if profile_name:
                            profiles.append(profile_name)
                    except IndexError:
                        continue
            
            log_messages.append(f"Đã phân tích. Tìm thấy {len(profiles)} profile.\n")
            if not profiles:
                messagebox.showwarning("Không tìm thấy", "Không có profile WiFi nào để sao lưu.")
                self.update_status("Sẵn sàng.")
                return

            backup_dir = filedialog.askdirectory(title="Chọn thư mục để lưu backup WiFi")
            if not backup_dir:
                self.update_status("Đã hủy sao lưu.")
                return
            
            backup_path = Path(backup_dir) / "WiFi_Backup"
            backup_path.mkdir(exist_ok=True)
            log_messages.append(f"Thư mục sao lưu: {backup_path}\n")
            
            wifi_data = []
            success_count = 0
            for i, profile in enumerate(profiles, 1):
                self.update_status(f"Đang xử lý {i}/{len(profiles)}: {profile}")
                try:
                    cmd_export = f'netsh wlan export profile name="{profile}" folder="{backup_path}" key=clear'
                    self.run_command(cmd_export)
                    
                    cmd_show = f'netsh wlan show profile name="{profile}" key=clear'
                    result_show = self.run_command(cmd_show)
                    
                    password = "(Không có mật khẩu hoặc lỗi)"
                    for res_line in result_show.split('\n'):
                        if "Key Content" in res_line:
                            password = res_line.split(':', 1)[1].strip()
                            break
                    
                    wifi_data.append(f"usename: {profile}   : {password}")
                    success_count += 1
                    log_messages.append(f"Thành công: {profile}\n")
                except Exception as e:
                    log_messages.append(f"LỖI với profile '{profile}': {str(e)}\n")
                    wifi_data.append(f"usename: {profile}   : (Lỗi: {str(e)})")
                    continue
            
            if wifi_data:
                txt_file_path = backup_path / "WiFi_Passwords.txt"
                with open(txt_file_path, "w", encoding="utf-8") as f:
                    f.write("\n".join(wifi_data))
                log_messages.append("Ghi file .txt thành công.\n")

            messagebox.showinfo("Hoàn thành", 
                              f"Đã xử lý xong. Thành công: {success_count}/{len(profiles)}.\n"
                              f"Dữ liệu được lưu tại:\n{backup_path}")
            
            os.startfile(backup_path)

        except Exception as e:
            # Chỉ ghi log file khi có lỗi xảy ra
            log_file = Path.home() / "windows_utility_tool_wifi_log.txt"
            error_message = f"Lỗi nghiêm trọng: {str(e)}"
            log_messages.append(f"\nLỖI NGHIÊM TRỌNG XẢY RA: {error_message}")
            
            try:
                with open(log_file, "w", encoding="utf-8") as log:
                    log.write("\n".join(log_messages))
                messagebox.showerror("Lỗi", f"{error_message}\n\nChi tiết lỗi đã được ghi tại:\n{log_file}")
            except Exception as log_e:
                 messagebox.showerror("Lỗi nghiêm trọng", f"Lỗi chính: {error_message}\n\nKhông thể ghi file log: {log_e}")

        finally:
            self.update_status("Sẵn sàng.")

def main():
    if os.name != 'nt':
        messagebox.showerror("Không tương thích", "Công cụ này chỉ chạy trên Windows.")
        return
        
    if not is_admin():
        try:
            ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " ".join(sys.argv), None, 1)
        except Exception as e:
            messagebox.showerror("Yêu cầu quyền Admin", f"Không thể tự động yêu cầu quyền Admin.\nLỗi: {e}")
        return

    try:
        root = tk.Tk()
        app = WindowsUtilityTool(root)
        root.mainloop()
    except Exception as e:
        messagebox.showerror("Lỗi khởi động", f"Không thể khởi động ứng dụng: {str(e)}")

if __name__ == "__main__":
    main()
