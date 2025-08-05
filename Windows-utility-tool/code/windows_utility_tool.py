#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Tiện ích hỗ trợ cài Windows - Windows Installation Support Utility
Phiên bản: 4.4 (Hiển thị thông tin DNS)
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
import psutil
import time
import re

# --- Thêm thư viện cho Mutex ---
from ctypes import wintypes

class SingleInstance:
    """
    Sử dụng một 'named mutex' để đảm bảo chỉ có một phiên bản của ứng dụng đang chạy.
    """
    def __init__(self, name):
        self.mutex_name = name
        self.mutex_handle = None
        
        # Định nghĩa các hàm WinAPI cần thiết
        self.CreateMutex = ctypes.windll.kernel32.CreateMutexW
        self.CreateMutex.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.LPCWSTR]
        self.CreateMutex.restype = wintypes.HANDLE

        self.GetLastError = ctypes.windll.kernel32.GetLastError
        self.GetLastError.restype = wintypes.DWORD

        self.ReleaseMutex = ctypes.windll.kernel32.ReleaseMutex
        self.ReleaseMutex.argtypes = [wintypes.HANDLE]
        self.ReleaseMutex.restype = wintypes.BOOL

        self.CloseHandle = ctypes.windll.kernel32.CloseHandle
        self.CloseHandle.argtypes = [wintypes.HANDLE]
        self.CloseHandle.restype = wintypes.BOOL
        
        # Các hằng số cần thiết
        self.ERROR_ALREADY_EXISTS = 183

    def __enter__(self):
        self.mutex_handle = self.CreateMutex(None, False, self.mutex_name)
        if self.GetLastError() == self.ERROR_ALREADY_EXISTS:
            self.CloseHandle(self.mutex_handle)
            self.mutex_handle = None
            raise RuntimeError("Một phiên bản khác của ứng dụng đã đang chạy.")
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.mutex_handle:
            self.ReleaseMutex(self.mutex_handle)
            self.CloseHandle(self.mutex_handle)

def is_admin():
    """Kiểm tra xem ứng dụng có đang chạy với quyền Administrator không."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def resource_path(relative_path):
    """ Lấy đường dẫn tuyệt đối đến tài nguyên, hoạt động cho cả chế độ dev và PyInstaller """
    try:
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

class WindowsUtilityTool:
    def __init__(self, root):
        self.root = root
        self.setup_window()
        self.create_widgets()
        
    def setup_window(self):
        """Thiết lập cửa sổ chính."""
        self.root.title("🛠️ Tiện ích hỗ trợ cài Win dạo v4.4")
        self.root.geometry("520x450")
        self.root.resizable(False, False)
        
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() // 2) - (520 // 2)
        y = (self.root.winfo_screenheight() // 2) - (450 // 2)
        self.root.geometry(f"520x450+{x}+{y}")
        
        try:
            icon_path = resource_path("icon.ico")
            self.root.iconbitmap(icon_path)
        except Exception as e:
            print(f"Không thể tải icon: {e}")
            pass
            
        self.root.configure(bg='#f0f0f0')
        
    def create_widgets(self):
        """Tạo các widget cho giao diện."""
        header_frame = tk.Frame(self.root, bg='#2c3e50', height=80)
        header_frame.pack(fill='x', padx=0, pady=0)
        header_frame.pack_propagate(False)
        tk.Label(header_frame, text="🛠️ Tiện ích hỗ trợ cài Win dạo", font=('Arial', 16, 'bold'), fg='white', bg='#2c3e50').pack(pady=(15, 5))
        tk.Label(header_frame, text="Phiên bản hoàn chỉnh v4.4", font=('Arial', 10), fg='#ecf0f1', bg='#2c3e50').pack()
        content_frame = tk.Frame(self.root, bg='#f0f0f0')
        content_frame.pack(fill='both', expand=True, padx=20, pady=20)
        button_style = {'font': ('Arial', 11, 'bold'), 'width': 22, 'height': 2, 'relief': 'raised', 'bd': 2, 'cursor': 'hand2'}
        row1_frame = tk.Frame(content_frame, bg='#f0f0f0')
        row1_frame.pack(fill='x', pady=10)
        tk.Button(row1_frame, text="🔧 Thiết lập Windows", bg='#3498db', fg='white', command=self.windows_setup, **button_style).pack(side='left', padx=10)
        tk.Button(row1_frame, text="🛜 Thiết lập mạng", bg='#95a5a6', fg='white', command=self.open_network_window, **button_style).pack(side='right', padx=10)
        row2_frame = tk.Frame(content_frame, bg='#f0f0f0')
        row2_frame.pack(fill='x', pady=10)
        tk.Button(row2_frame, text="🗑️ Xóa Bloatware", bg='#e74c3c', fg='white', command=self.remove_bloatware, **button_style).pack(side='left', padx=10)
        tk.Button(row2_frame, text="📶 Sao lưu Wifi", bg='#f39c12', fg='white', command=self.backup_wifi, **button_style).pack(side='right', padx=10)
        row3_frame = tk.Frame(content_frame, bg='#f0f0f0')
        row3_frame.pack(fill='x', pady=10)
        tk.Button(row3_frame, text="💾 Sao lưu Driver", bg='#27ae60', fg='white', command=self.backup_drivers, **button_style).pack(side='left', padx=10)
        self.status_var = tk.StringVar(value="Sẵn sàng...")
        status_frame = tk.Frame(self.root, bg='#34495e', height=30)
        status_frame.pack(fill='x', side='bottom')
        status_frame.pack_propagate(False)
        tk.Label(status_frame, textvariable=self.status_var, font=('Arial', 9), fg='white', bg='#34495e').pack(side='left', padx=10, pady=5)

    def run_in_thread(self, target_func, *args):
        thread = threading.Thread(target=target_func, args=args, daemon=True)
        thread.start()

    def run_command(self, command, shell=True, check=False):
        try:
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            result = subprocess.run(
                command, shell=shell, capture_output=True, text=True,
                check=check, encoding='utf-8', errors='ignore', startupinfo=startupinfo
            )
            return result.stdout + result.stderr
        except subprocess.CalledProcessError as e:
            return f"Lệnh thất bại với mã lỗi {e.returncode}:\n{e.stdout}\n{e.stderr}"
        except Exception as e:
            return f"Lỗi thực thi lệnh: {str(e)}"

    def update_status(self, message):
        self.status_var.set(message)
        self.root.update()
        
    def windows_setup(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")
    def remove_bloatware(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")
    def backup_drivers(self): messagebox.showinfo("Thông báo", "Chức năng đang được phát triển.")

    def open_network_window(self):
        """Mở cửa sổ quản lý và thiết lập mạng."""
        self.net_win = tk.Toplevel(self.root)
        self.net_win.title("Thiết lập mạng")
        self.net_win.geometry("980x500") # Mở rộng cửa sổ

        self.net_win.update_idletasks()
        width = self.net_win.winfo_width()
        height = self.net_win.winfo_height()
        x = (self.net_win.winfo_screenwidth() // 2) - (width // 2)
        y = (self.net_win.winfo_screenheight() // 2) - (height // 2)
        self.net_win.geometry(f'{width}x{height}+{x}+{y}')
        
        try:
            icon_path = resource_path("icon.ico")
            self.net_win.iconbitmap(icon_path)
        except Exception as e:
            print(f"Không thể tải icon cho cửa sổ mạng: {e}")
            pass

        self.net_win.transient(self.root)
        self.net_win.grab_set()

        info_frame = ttk.LabelFrame(self.net_win, text="Thông tin Card mạng", padding=(10, 10))
        info_frame.pack(fill='x', padx=10, pady=10)

        cols = ("Tên", "Loại", "Địa chỉ IP", "Địa chỉ MAC", "DNS Servers")
        self.net_tree = ttk.Treeview(info_frame, columns=cols, show='headings', height=8)
        
        for col in cols:
            self.net_tree.heading(col, text=col)
            width = 180 if col == "DNS Servers" else 150
            self.net_tree.column(col, width=width, anchor='w')
        
        self.net_tree.pack(fill='x', expand=True)
        
        refresh_button = tk.Button(info_frame, text="🔃 Tải lại danh sách", command=lambda: self.run_in_thread(self.refresh_network_info))
        refresh_button.pack(pady=5)
        
        self.run_in_thread(self.refresh_network_info)

        action_frame = ttk.LabelFrame(self.net_win, text="Chức năng", padding=(10, 10))
        action_frame.pack(fill='x', padx=10, pady=10)

        row1 = tk.Frame(action_frame)
        row1.pack(fill='x', pady=5)
        tk.Button(row1, text="Xóa Cache DNS", command=self.flush_dns).pack(side='left', padx=5)
        tk.Button(row1, text="Reset TCP/IP", command=self.reset_tcp_ip).pack(side='left', padx=5)
        tk.Button(row1, text="Khôi phục WiFi từ Backup", command=self.restore_wifi).pack(side='left', padx=5)

        dns_frame = ttk.LabelFrame(action_frame, text="Đổi DNS nhanh (cho card mạng đã chọn)", padding=(10, 5))
        dns_frame.pack(fill='x', pady=10)
        tk.Button(dns_frame, text="Google DNS", command=lambda: self.change_dns("8.8.8.8", "8.8.4.4")).pack(side='left', padx=5)
        tk.Button(dns_frame, text="Cloudflare DNS", command=lambda: self.change_dns("1.1.1.1", "1.0.0.1")).pack(side='left', padx=5)
        tk.Button(dns_frame, text="Xóa DNS (Tự động)", command=lambda: self.change_dns()).pack(side='left', padx=5)

    def _get_dns_servers(self, interface_name):
        """Lấy danh sách DNS server cho một card mạng cụ thể."""
        try:
            result = self.run_command(f'netsh interface ipv4 show dnsservers name="{interface_name}"')
            if "none" in result.lower():
                 return "Tự động (DHCP)"
            
            # Sử dụng regex để tìm tất cả các địa chỉ IP
            dns_servers = re.findall(r'\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b', result)
            return ", ".join(dns_servers) if dns_servers else "Không có"
        except Exception:
            return "Không thể lấy"

    def refresh_network_info(self):
        self.update_status("Đang làm mới thông tin mạng...")
        try:
            # Xóa các mục cũ
            for item in self.net_tree.get_children():
                self.net_tree.delete(item)
            
            # Lấy thông tin cơ bản
            addrs = psutil.net_if_addrs()
            stats = psutil.net_if_stats()
            
            # Duyệt qua từng card mạng
            for intf, addr_list in addrs.items():
                ip = mac = ""
                intf_type = "Up" if stats.get(intf) and stats[intf].isup else "Down"
                for addr in addr_list:
                    if addr.family == psutil.AF_LINK: mac = addr.address
                    if addr.family == 2: ip = addr.address
                
                # Lấy thông tin DNS
                dns = self._get_dns_servers(intf)
                
                # Thêm vào bảng
                self.net_tree.insert("", "end", values=(intf, intf_type, ip, mac, dns))

        except Exception as e:
            messagebox.showerror("Lỗi", f"Không thể lấy thông tin mạng: {e}", parent=self.net_win)
        finally:
            self.update_status("Sẵn sàng.")

    def _has_wireless_interface(self):
        """Kiểm tra xem hệ thống có card mạng không dây (WiFi) hay không."""
        try:
            result = self.run_command("netsh wlan show interfaces")
            if "There is no wireless interface on the system" in result or "Không có giao diện không dây nào trên hệ thống" in result:
                return False
            return True
        except Exception:
            return False

    def _ensure_wlansvc_running(self):
        """Kiểm tra và khởi động dịch vụ WLAN AutoConfig nếu cần."""
        try:
            service = psutil.win_service_get('wlansvc')
            if service.status() == 'running':
                return True
        except psutil.NoSuchProcess:
            messagebox.showerror("Lỗi", "Không tìm thấy dịch vụ WLAN AutoConfig (wlansvc).", parent=self.net_win if hasattr(self, 'net_win') else self.root)
            return False
        except Exception as e:
            messagebox.showerror("Lỗi", f"Lỗi không xác định khi kiểm tra dịch vụ WLAN: {e}", parent=self.net_win if hasattr(self, 'net_win') else self.root)
            return False

        self.update_status("Dịch vụ WLAN chưa chạy, đang khởi động...")
        result = self.run_command("net start wlansvc", check=True)
        time.sleep(2) 
        
        try:
            service.refresh()
            if service.status() == 'running':
                self.update_status("Dịch vụ WLAN đã được khởi động.")
                return True
            else:
                messagebox.showerror("Lỗi", f"Không thể khởi động dịch vụ WLAN AutoConfig.\nKết quả:\n{result}", parent=self.net_win if hasattr(self, 'net_win') else self.root)
                return False
        except Exception as e:
            messagebox.showerror("Lỗi", f"Lỗi sau khi cố khởi động dịch vụ WLAN: {e}", parent=self.net_win if hasattr(self, 'net_win') else self.root)
            return False

    def restore_wifi(self):
        if not self._has_wireless_interface():
            messagebox.showwarning("Không tìm thấy Card WiFi", "Không tìm thấy card mạng WiFi trên hệ thống.\n\nChức năng này không thể hoạt động.\n\n(Lưu ý: Nếu bạn đang dùng máy ảo, bạn cần kết nối một USB WiFi vào máy ảo để sử dụng tính năng này).")
            return

        files = filedialog.askopenfilenames(
            title="Chọn các file backup WiFi (.xml)",
            filetypes=[("XML files", "*.xml"), ("All files", "*.*")],
            parent=self.net_win
        )
        if not files:
            self.update_status("Đã hủy khôi phục WiFi.")
            return
        self.run_in_thread(self._restore_wifi_task, files)

    def _restore_wifi_task(self, files):
        """Tác vụ khôi phục WiFi, sử dụng 'user=all' để tăng tương thích."""
        if not self._ensure_wlansvc_running():
            self.update_status("Sẵn sàng.")
            return

        success_count = 0
        total_files = len(files)
        
        for i, file_path in enumerate(files, 1):
            self.update_status(f"Đang khôi phục {i}/{total_files}...")
            command = f'netsh wlan add profile filename="{file_path}" user=all'
            result = self.run_command(command)
            
            if "is added on interface" in result.lower() or "được thêm trên giao diện" in result.lower():
                success_count += 1
        
        messagebox.showinfo(
            "Hoàn thành",
            f"Đã khôi phục thành công {success_count}/{total_files} profile WiFi.",
            parent=self.net_win
        )
        self.update_status("Sẵn sàng.")

    def flush_dns(self):
        self.update_status("Đang xóa cache DNS...")
        self.run_in_thread(self._flush_dns_task)

    def _flush_dns_task(self):
        result = self.run_command("ipconfig /flushdns")
        if "successfully flushed the dns resolver cache" in result.lower() or "đã xóa thành công bộ đệm của trình phân giải dns" in result.lower():
            messagebox.showinfo("Hoàn thành", f"Đã xóa cache DNS thành công.\n\nKết quả:\n{result.strip()}", parent=self.net_win)
        else:
            messagebox.showerror("Lỗi", f"Xóa cache DNS thất bại.\n\nKết quả:\n{result.strip()}", parent=self.net_win)
        self.update_status("Sẵn sàng.")

    def reset_tcp_ip(self):
        self.update_status("Đang reset TCP/IP...")
        self.run_in_thread(self._reset_tcp_ip_task)

    def _reset_tcp_ip_task(self):
        result = self.run_command("netsh int ip reset")
        if "resetting" in result.lower() and "ok" in result.lower():
             messagebox.showinfo("Hoàn thành", f"Reset TCP/IP thành công. Vui lòng khởi động lại máy tính để hoàn tất.\n\nKết quả:\n{result.strip()}", parent=self.net_win)
        else:
            messagebox.showerror("Lỗi", f"Reset TCP/IP thất bại.\n\nKết quả:\n{result.strip()}", parent=self.net_win)
        self.update_status("Sẵn sàng.")

    def change_dns(self, dns1=None, dns2=None):
        selected_item = self.net_tree.focus()
        if not selected_item:
            messagebox.showwarning("Chưa chọn", "Vui lòng chọn một card mạng từ danh sách.", parent=self.net_win)
            return
        
        interface_name = self.net_tree.item(selected_item)['values'][0]
        self.update_status(f"Đang đổi DNS cho '{interface_name}'...")
        self.run_in_thread(self._change_dns_task, interface_name, dns1, dns2)

    def _change_dns_task(self, interface_name, dns1, dns2):
        if dns1 is None: # Xóa DNS
            command = f'netsh interface ipv4 set dnsserver name="{interface_name}" source=dhcp'
            result = self.run_command(command)
            if "error" not in result.lower() and "lỗi" not in result.lower() and result.strip() == "":
                 messagebox.showinfo("Hoàn thành", f"Đã xóa DNS cho '{interface_name}'. Card mạng sẽ nhận DNS tự động.", parent=self.net_win)
            else:
                 messagebox.showerror("Lỗi", f"Không thể xóa DNS.\n\nKết quả:\n{result.strip()}", parent=self.net_win)
        else: # Set DNS
            command1 = f'netsh interface ipv4 set dnsserver name="{interface_name}" static {dns1} primary'
            command2 = f'netsh interface ipv4 add dnsserver name="{interface_name}" {dns2} index=2'
            result1 = self.run_command(command1)
            result2 = self.run_command(command2)
            
            if (result1.strip() == "" and result2.strip() == ""):
                messagebox.showinfo("Hoàn thành", f"Đã đổi DNS cho '{interface_name}' thành công.", parent=self.net_win)
            else:
                full_result = f"Lệnh 1:\n{result1.strip()}\n\nLệnh 2:\n{result2.strip()}"
                messagebox.showerror("Lỗi", f"Không thể đổi DNS.\n\nKết quả:\n{full_result}", parent=self.net_win)

        self.update_status("Sẵn sàng.")

    def backup_wifi(self):
        if not self._has_wireless_interface():
            messagebox.showwarning("Không tìm thấy Card WiFi", "Không tìm thấy card mạng WiFi trên hệ thống.\n\nChức năng này không thể hoạt động.\n\n(Lưu ý: Nếu bạn đang dùng máy ảo, bạn cần kết nối một USB WiFi vào máy ảo để sử dụng tính năng này).")
            return
        self.update_status("Bắt đầu sao lưu WiFi...")
        self.run_in_thread(self._backup_wifi_task)

    def _backup_wifi_task(self):
        if not self._ensure_wlansvc_running():
            self.update_status("Sẵn sàng.")
            return
            
        log_messages = []
        try:
            log_messages.append("=== Bắt đầu sao lưu WiFi v4.1 ===\n")
            self.update_status("Đang lấy danh sách WiFi...")
            profiles_result = self.run_command("netsh wlan show profiles")
            log_messages.append(f"Kết quả lệnh 'netsh wlan show profiles':\n{profiles_result}\n")
            profiles = []
            for line in profiles_result.split('\n'):
                if "All User Profile" in line or "Hồ sơ Tất cả Người dùng" in line:
                    try:
                        profile_name = line.split(':', 1)[1].strip()
                        if profile_name: profiles.append(profile_name)
                    except IndexError: continue
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
                    self.run_command(f'netsh wlan export profile name="{profile}" folder="{backup_path}" key=clear')
                    result_show = self.run_command(f'netsh wlan show profile name="{profile}" key=clear')
                    password = "(Không có mật khẩu hoặc lỗi)"
                    for res_line in result_show.split('\n'):
                        if "Key Content" in res_line or "Nội dung Khóa" in res_line:
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
                with open(backup_path / "WiFi_Passwords.txt", "w", encoding="utf-8") as f: f.write("\n".join(wifi_data))
                log_messages.append("Ghi file .txt thành công.\n")
            messagebox.showinfo("Hoàn thành", f"Đã xử lý xong. Thành công: {success_count}/{len(profiles)}.\nDữ liệu được lưu tại:\n{backup_path}")
            os.startfile(backup_path)
        except Exception as e:
            log_file = Path.home() / "windows_utility_tool_wifi_log.txt"
            error_message = f"Lỗi nghiêm trọng: {str(e)}"
            log_messages.append(f"\nLỖI NGHIÊM TRỌNG XẢY RA: {error_message}")
            try:
                with open(log_file, "w", encoding="utf-8") as log: log.write("\n".join(log_messages))
                messagebox.showerror("Lỗi", f"{error_message}\n\nChi tiết lỗi đã được ghi tại:\n{log_file}")
            except Exception as log_e: messagebox.showerror("Lỗi nghiêm trọng", f"Lỗi chính: {error_message}\n\nKhông thể ghi file log: {log_e}")
        finally:
            self.update_status("Sẵn sàng.")

def main():
    if os.name != 'nt':
        messagebox.showerror("Không tương thích", "Công cụ này chỉ chạy trên Windows.")
        return
        
    # --- Thêm logic kiểm tra single-instance ---
    try:
        with SingleInstance("WindowsUtilityTool_Doctoten_App_Mutex"):
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

    except RuntimeError as e:
        messagebox.showwarning("Thông báo", str(e))
        return

if __name__ == "__main__":
    main()
