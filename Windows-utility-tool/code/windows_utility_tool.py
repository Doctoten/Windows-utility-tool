#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Tiện ích hỗ trợ cài Windows - Windows Installation Support Utility
Phiên bản: 5.9 (Tối ưu giao diện và Hoàn thiện)
Phát triển bởi: Doctoten
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess, os, sys, threading, ctypes, psutil, time, re, json
from datetime import datetime
from pathlib import Path
from ctypes import wintypes
from tkinter import TclError
import ttkbootstrap as ttkb
from ttkbootstrap import Style
import xml.etree.ElementTree as ET

# ==================================================================================================
# HỆ THỐNG QUẢN LÝ NGÔN NGỮ
# ==================================================================================================
LANGUAGES = {
    'vi': {
        "app_title": "🛠️ Tiện ích hỗ trợ cài Win dạo", "app_version": "v5.4 (Build by Doctoten)",
        "win_setup_btn": "🔧 Thiết lập Windows", "net_setup_btn": "🛜 Thiết lập mạng", "bloat_remove_btn": "🗑️ Xóa Bloatware",
        "wifi_backup_btn": "📶 Sao lưu Wifi", "driver_setup_btn": "🧩 Thiết lập Driver",
        "net_win_title": "Thiết lập mạng", "net_info_frame": "Thông tin Card mạng", "net_col_name": "Tên", "net_col_type": "Trạng thái",
        "net_col_ip": "Địa chỉ IP", "net_col_mac": "Địa chỉ MAC", "net_col_dns": "DNS Servers", "net_refresh_btn": "🔃 Tải lại",
        "net_actions_frame": "Chức năng", "net_flush_dns_btn": "Xóa Cache DNS", "net_reset_tcp_btn": "Reset TCP/IP",
        "net_restore_wifi_btn": "Khôi phục WiFi", "net_dns_frame": "Đổi DNS nhanh (cho card mạng đã chọn)", "net_clear_dns_btn": "Xóa DNS",
        "net_google_dns": "Google DNS", "net_cloudflare_dns": "Cloudflare DNS",
        "bloat_win_title": "🗑️ Xóa Bloatware", "bloat_header_subtitle": "Chọn và gỡ bỏ các ứng dụng không cần thiết",
        "bloat_list_label": "Danh sách ứng dụng có thể gỡ bỏ:", "bloat_col_select": "Chọn", "bloat_col_app_name": "Tên ứng dụng",
        "bloat_col_package_name": "Tên gói", "bloat_col_version": "Phiên bản", "bloat_col_location": "Đường dẫn",
        "bloat_load_btn": "🔄 Tải danh sách", "bloat_select_all_btn": "☑️ Chọn tất cả", "bloat_deselect_all_btn": "❌ Bỏ chọn tất cả",
        "bloat_remove_selected_btn": "🗑️ Xóa đã chọn",
        "shutdown_timer_frame": "Hẹn giờ Tắt máy", "shutdown_time_label": "Tắt máy sau:", "shutdown_unit_minutes": "Phút",
        "shutdown_unit_hours": "Giờ", "shutdown_set_btn": "Bắt đầu", "shutdown_cancel_btn": "Hủy lệnh",
        "status_ready": "Sẵn sàng...", "status_dev": "Chức năng đang được phát triển.", "status_loading_net": "Đang làm mới thông tin mạng...",
        "status_flushing_dns": "Đang xóa cache DNS...", "status_resettiing_tcp": "Đang reset TCP/IP...", "status_changing_dns": "Đang đổi DNS cho '{iface}'...",
        "status_restoring_wifi": "Đang khôi phục {i}/{total}...", "status_backing_up_wifi": "Bắt đầu sao lưu WiFi...", "status_loading_wifi_profiles": "Đang lấy danh sách WiFi...",
        "status_loading_bloatware": "Đang tải danh sách ứng dụng...", "status_removing_bloatware": "Đang xóa {i}/{total}: {name}",
        "win_setup_win_title": "Thiết lập Windows",
        "win_setup_header_subtitle": "Chạy kịch bản tối ưu hóa Windows của Chris Titus Tech",
        "win_setup_stable_btn": "Chạy bản Ổn định (Khuyên dùng)",
        "win_setup_dev_btn": "Chạy bản Phát triển",
        "win_setup_info": "Lưu ý: Thao tác này sẽ mở một cửa sổ PowerShell mới để chạy kịch bản. Vui lòng làm theo hướng dẫn trong đó.",
        "status_running_win_script": "Đang khởi chạy kịch bản thiết lập Windows...",
        "title_info": "Thông báo", "title_warning": "Cảnh báo", "title_error": "Lỗi", "title_confirm": "Xác nhận",
        "btn_confirm": "Xác nhận", "btn_cancel": "Hủy",
        "msg_net_no_iface_selected": "Vui lòng chọn một card mạng từ danh sách.", "msg_bloat_no_app_selected": "Vui lòng chọn ít nhất một ứng dụng để xóa.",
        "msg_confirm_bloat_remove": "Bạn có chắc chắn muốn xóa {count} ứng dụng sau?\n\n{app_list}\nHành động này không thể hoàn tác!",
        "msg_wifi_no_card": "Không tìm thấy card mạng WiFi trên hệ thống.", "msg_wifi_no_profile_to_backup": "Không có profile WiFi nào để sao lưu.",
        "msg_wifi_backup_complete": "Đã xử lý xong. Thành công: {s}/{t}.\nDữ liệu được lưu tại:\n{path}",
        "msg_wifi_restore_complete": "Đã khôi phục thành công {s}/{t} profile WiFi.", "title_wifi_restore": "Chọn file backup WiFi (.xml)",
        "msg_dns_flush_success": "Đã xóa cache DNS thành công.", "msg_dns_flush_fail": "Xóa cache DNS thất bại.",
        "msg_bloat_load_fail": "Lỗi khi tải danh sách: {e}", "msg_net_fetch_fail": "Không thể lấy thông tin mạng: {e}",
        "title_wifi_backup_dir": "Chọn thư mục để lưu sao lưu WiFi",
        "msg_wlan_not_found": "Không tìm thấy dịch vụ WLAN.", "status_starting_wlan": "Đang khởi động dịch vụ WLAN...",
        "msg_wlan_cannot_start": "Không thể khởi động dịch vụ WLAN.",
        "title_driver_backup_dir": "Chọn thư mục để lưu Driver",
        "status_backing_up_drivers": "Đang sao lưu Driver...",
        "msg_driver_backup_complete": "Sao lưu driver hoàn tất.\nDữ liệu lưu tại:\n{path}",
        "msg_driver_backup_fail": "Sao lưu driver thất bại.\nChi tiết:\n{e}",
        "status_canceling_drivers": "Đang hủy thao tác driver...",
        "msg_driver_cancelled": "Đã hủy thao tác driver.",
        "driver_win_title": "Thiết lập Driver",
        "driver_header_subtitle": "Sao lưu và Cài đặt (Import) Driver",
        "driver_backup_inner_btn": "💾 Sao lưu Driver",
        "driver_import_inner_btn": "⬆️ Cài đặt/Import Driver",
        "title_driver_import_dir": "Chọn thư mục chứa file .INF",
        "status_importing_drivers": "Đang cài đặt Driver...",
        "msg_driver_import_complete": "Cài đặt driver hoàn tất.",
        "msg_driver_import_fail": "Cài đặt driver thất bại.\nChi tiết:\n{e}",
        "msg_bloat_remove_complete": "Xóa xong. Thành công: {s}/{t}.\n\nLỗi:\n{errors}",
        "msg_critical_error_log": "Đã xảy ra lỗi nghiêm trọng. Xem chi tiết trong file log:\n{path}",
        "app_already_running_msg": "Một phiên bản khác của ứng dụng đã đang chạy.",
        "shutdown_status_set": "Máy sẽ tắt sau {value} {unit}", "shutdown_status_none": "Không có lịch tắt máy.",
        "shutdown_status_pending": "Đang có lịch tắt máy.",
        "shutdown_invalid_format": "Vui lòng nhập một số hợp lệ.", "msg_reset_tcp_success": "Reset TCP/IP thành công. Vui lòng khởi động lại máy tính để hoàn tất.",
        "msg_reset_tcp_fail": "Reset TCP/IP thất bại.", "msg_dns_change_success": "Đã đổi DNS cho '{iface}' thành công.", "msg_dns_change_fail": "Không thể đổi DNS cho '{iface}'.",
    },
    'en': {
        "app_title": "🛠️ Windows Setup Utility", "app_version": "v5.4 (Build by Doctoten)",
        "win_setup_btn": "🔧 Windows Setup", "net_setup_btn": "🛜 Network Setup", "bloat_remove_btn": "🗑️ Remove Bloatware",
        "wifi_backup_btn": "📶 Backup Wifi", "driver_setup_btn": "🧩 Driver Setup",
        "net_win_title": "Network Setup", "net_info_frame": "Network Interface Information", "net_col_name": "Name", "net_col_type": "Status",
        "net_col_ip": "IP Address", "net_col_mac": "MAC Address", "net_col_dns": "DNS Servers", "net_refresh_btn": "🔃 Refresh",
        "net_actions_frame": "Actions", "net_flush_dns_btn": "Flush DNS Cache", "net_reset_tcp_btn": "Reset TCP/IP",
        "net_restore_wifi_btn": "Restore WiFi", "net_dns_frame": "Quick DNS Change (for selected interface)", "net_clear_dns_btn": "Clear DNS",
        "net_google_dns": "Google DNS", "net_cloudflare_dns": "Cloudflare DNS",
        "bloat_win_title": "🗑️ Remove Bloatware", "bloat_header_subtitle": "Select and remove unnecessary applications",
        "bloat_list_label": "List of removable applications:", "bloat_col_select": "Select", "bloat_col_app_name": "App Name",
        "bloat_col_package_name": "Package Name", "bloat_col_version": "Version", "bloat_col_location": "Install Location",
        "bloat_load_btn": "🔄 Load List", "bloat_select_all_btn": "☑️ Select All", "bloat_deselect_all_btn": "❌ Deselect All",
        "bloat_remove_selected_btn": "🗑️ Remove Selected",
        "shutdown_timer_frame": "Shutdown Timer", "shutdown_time_label": "Shutdown after:", "shutdown_unit_minutes": "Minutes",
        "shutdown_unit_hours": "Hours", "shutdown_set_btn": "Confirm", "shutdown_cancel_btn": "Cancel",
        "status_ready": "Ready...", "status_dev": "This feature is under development.", "status_loading_net": "Refreshing network information...",
        "status_flushing_dns": "Flushing DNS cache...", "status_resettiing_tcp": "Resetting TCP/IP...", "status_changing_dns": "Changing DNS for '{iface}'...",
        "status_restoring_wifi": "Restoring {i}/{total}...", "status_backing_up_wifi": "Starting WiFi backup...", "status_loading_wifi_profiles": "Loading WiFi profiles...",
        "status_loading_bloatware": "Loading application list...", "status_removing_bloatware": "Removing {i}/{total}: {name}",
        "win_setup_win_title": "Windows Setup",
        "win_setup_header_subtitle": "Run Chris Titus Tech's Windows Utility script",
        "win_setup_stable_btn": "Run Stable Branch (Recommended)",
        "win_setup_dev_btn": "Run Dev Branch",
        "win_setup_info": "Note: This will open a new PowerShell window to run the script. Please follow the instructions there.",
        "status_running_win_script": "Launching Windows setup script...",
        "title_info": "Information", "title_warning": "Warning", "title_error": "Error", "title_confirm": "Confirmation",
        "btn_confirm": "Confirm", "btn_cancel": "Cancel",
        "msg_net_no_iface_selected": "Please select a network interface from the list.", "msg_bloat_no_app_selected": "Please select at least one application to remove.",
        "msg_confirm_bloat_remove": "Are you sure you want to remove the following {count} applications?\n\n{app_list}\nThis action cannot be undone!",
        "msg_wifi_no_card": "No WiFi network interface found on this system.", "msg_wifi_no_profile_to_backup": "No WiFi profiles found to back up.",
        "msg_wifi_backup_complete": "Processing complete. Success: {s}/{t}.\nData saved at:\n{path}",
        "msg_wifi_restore_complete": "Successfully restored {s}/{t} WiFi profiles.", "title_wifi_restore": "Select WiFi backup files (.xml)",
        "msg_dns_flush_success": "Successfully flushed the DNS cache.", "msg_dns_flush_fail": "Failed to flush DNS cache.",
        "msg_bloat_load_fail": "Failed to load list: {e}", "msg_net_fetch_fail": "Cannot fetch network info: {e}",
        "title_wifi_backup_dir": "Select folder to save WiFi backup",
        "msg_wlan_not_found": "WLAN service not found.", "status_starting_wlan": "Starting WLAN service...",
        "msg_wlan_cannot_start": "Could not start WLAN service.",
        "title_driver_backup_dir": "Select folder to save Drivers",
        "status_backing_up_drivers": "Backing up drivers...",
        "msg_driver_backup_complete": "Driver backup completed.\nData saved at:\n{path}",
        "msg_driver_backup_fail": "Driver backup failed.\nDetails:\n{e}",
        "status_canceling_drivers": "Canceling driver operation...",
        "msg_driver_cancelled": "Driver operation canceled.",
        "driver_win_title": "Driver Setup",
        "driver_header_subtitle": "Backup and Import/Install Drivers",
        "driver_backup_inner_btn": "💾 Backup Drivers",
        "driver_import_inner_btn": "⬆️ Import/Install Drivers",
        "title_driver_import_dir": "Select folder with .INF files",
        "status_importing_drivers": "Installing drivers...",
        "msg_driver_import_complete": "Driver installation completed.",
        "msg_driver_import_fail": "Driver installation failed.\nDetails:\n{e}",
        "msg_bloat_remove_complete": "Removal complete. Success: {s}/{t}.\n\nFailures:\n{errors}",
        "msg_critical_error_log": "A critical error occurred. See log file for details:\n{path}",
        "app_already_running_msg": "Another instance of the application is already running.",
        "shutdown_status_set": "PC will shut down in {value} {unit}", "shutdown_status_none": "No shutdown scheduled.",
        "shutdown_status_pending": "A shutdown is scheduled.",
        "shutdown_invalid_format": "Please enter a valid number.", "msg_reset_tcp_success": "TCP/IP reset successfully. Please restart your computer to complete.",
        "msg_reset_tcp_fail": "Failed to reset TCP/IP.", "msg_dns_change_success": "DNS for '{iface}' changed successfully.", "msg_dns_change_fail": "Failed to change DNS for '{iface}'.",
    }
}

class SingleInstance:
    def __init__(self, name):
        self.mutex_name = name
        self.mutex_handle = None
        self.CreateMutex = ctypes.windll.kernel32.CreateMutexW
        self.CreateMutex.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.LPCWSTR]
        self.CreateMutex.restype = wintypes.HANDLE
        self.GetLastError = ctypes.windll.kernel32.GetLastError
        self.ReleaseMutex = ctypes.windll.kernel32.ReleaseMutex
        self.CloseHandle = ctypes.windll.kernel32.CloseHandle
        self.ERROR_ALREADY_EXISTS = 183

    def __enter__(self):
        self.mutex_handle = self.CreateMutex(None, False, self.mutex_name)
        if self.GetLastError() == self.ERROR_ALREADY_EXISTS:
            self.CloseHandle(self.mutex_handle)
            self.mutex_handle = None
            raise RuntimeError("App is already running.")
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.mutex_handle:
            self.ReleaseMutex(self.mutex_handle)
            self.CloseHandle(self.mutex_handle)

def is_admin():
    try: return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception: return False

def resource_path(relative_path):
    try: base_path = sys._MEIPASS
    except Exception: base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

def set_current_process_app_id(app_id):
    try:
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(app_id)
    except Exception:
        pass

class WindowsUtilityTool:
    def __init__(self, root):
        self.root = root
        try:
            # Đặt font mặc định cho ttk qua Style để tránh gán trực tiếp -font vào widget
            self.style = Style()
            self.style.configure('TButton', font=('Segoe UI', 10))
            self.style.configure('TLabel', font=('Segoe UI', 10))
        except Exception:
            pass
        self.current_lang = 'vi'
        self.net_win = None
        self.bloat_win = None
        self.win_setup_win = None
        self.setup_window()
        self.create_widgets()
        self.update_ui_language()
        # Kiểm tra lịch tắt máy hiện có
        self.root.after(200, self.check_existing_shutdown_schedule)

    def _schedule_store_path(self):
        base = os.getenv('LOCALAPPDATA') or str(Path.home())
        folder = Path(base) / "WindowsUtilityTool"
        folder.mkdir(exist_ok=True)
        return folder / "shutdown_schedule.json"
        
    def setup_window(self):
        self.root.geometry("520x470") # Increased height for new section
        self.root.resizable(False, False)
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() // 2) - (520 // 2)
        y = (self.root.winfo_screenheight() // 2) - (470 // 2)
        self.root.geometry(f"520x470+{x}+{y}")
        self._set_window_icon(self.root)
        self.root.configure(bg='#f0f0f0')
        
    def create_widgets(self):
        # Header
        self.header_frame = tk.Frame(self.root, bg='#2c3e50', height=70); self.header_frame.pack(fill='x'); self.header_frame.pack_propagate(False)
        self.title_label = tk.Label(self.header_frame, font=('Segoe UI', 15, 'bold'), fg='white', bg='#2c3e50'); self.title_label.pack(pady=(10, 2))
        self.version_label = tk.Label(self.header_frame, font=('Segoe UI', 9), fg='#ecf0f1', bg='#2c3e50'); self.version_label.pack()

        # Content
        content_frame = tk.Frame(self.root, bg='#f0f0f0'); content_frame.pack(fill='both', expand=True, padx=20, pady=15)
        btn_font = ('Segoe UI', 10)
        
        grid_frame = tk.Frame(content_frame, bg='#f0f0f0')
        grid_frame.pack(fill='x', pady=4)
        # 3 cột đồng đều
        grid_frame.grid_columnconfigure(0, weight=1, uniform='funcs')
        grid_frame.grid_columnconfigure(1, weight=1, uniform='funcs')
        grid_frame.grid_columnconfigure(2, weight=1, uniform='funcs')
        # 2 hàng
        grid_frame.grid_rowconfigure(0, weight=1)
        grid_frame.grid_rowconfigure(1, weight=1)

        pad = {'padx': 6, 'pady': 6}
        self.win_setup_btn = ttkb.Button(grid_frame, command=self.windows_setup, bootstyle='primary'); self.win_setup_btn.grid(row=0, column=0, sticky='nsew', **pad)
        self.net_setup_btn = ttkb.Button(grid_frame, command=self.open_network_window, bootstyle='secondary'); self.net_setup_btn.grid(row=0, column=1, sticky='nsew', **pad)
        self.bloat_remove_btn = ttkb.Button(grid_frame, command=self.open_bloatware_window, bootstyle='danger'); self.bloat_remove_btn.grid(row=0, column=2, sticky='nsew', **pad)
        self.wifi_backup_btn = ttkb.Button(grid_frame, command=self.backup_wifi, bootstyle='warning'); self.wifi_backup_btn.grid(row=1, column=0, sticky='nsew', **pad)
        self.driver_backup_btn = ttkb.Button(grid_frame, command=self.open_driver_window, bootstyle='success'); self.driver_backup_btn.grid(row=1, column=1, sticky='nsew', **pad)
        
        # Status Bar and Language Switcher
        status_frame = tk.Frame(self.root, bg='#34495e', height=30); status_frame.pack(fill='x', side='bottom'); status_frame.pack_propagate(False)
        self.status_var = tk.StringVar()
        self.status_label = tk.Label(status_frame, textvariable=self.status_var, font=('Arial', 9), fg='white', bg='#34495e'); self.status_label.pack(side='left', padx=10, pady=5, fill='x', expand=True)
        # Main progress bar for long-running tasks (e.g., WiFi backup)
        self.main_progress = ttk.Progressbar(status_frame, length=160, mode='determinate')
        self.main_progress_var = tk.StringVar(value="")
        self.main_progress_label = tk.Label(status_frame, textvariable=self.main_progress_var, font=('Arial', 9), fg='white', bg='#34495e')
        self._main_progress_visible = False  # show only when running
        
        self.lang_menu_btn = tk.Menubutton(status_frame, text='Language', bg='#34495e', fg='white', activebackground='#34495e', activeforeground='white', relief='ridge', bd=1, highlightthickness=1, highlightbackground='#2c3e50', highlightcolor='#ecf0f1')
        self.lang_menu = tk.Menu(self.lang_menu_btn, tearoff=0, bg='#34495e', fg='white', activebackground='#2c3e50', activeforeground='white')
        self.lang_menu.add_command(label='Tiếng Việt', command=lambda: self.switch_language('Tiếng Việt'))
        self.lang_menu.add_command(label='English', command=lambda: self.switch_language('English'))
        self.lang_menu_btn.config(menu=self.lang_menu)
        self.lang_menu_btn.pack(side='right', padx=10)

        # Shutdown Timer Frame
        shutdown_frame = ttk.LabelFrame(content_frame, padding=(10, 5)); shutdown_frame.pack(fill='x', pady=(10, 0), padx=10)
        self.shutdown_frame = shutdown_frame

        # Top row: status text (full width)
        sf_top = tk.Frame(shutdown_frame); sf_top.pack(fill='x')
        self.shutdown_status_label = tk.Label(sf_top, font=('Arial', 9, 'italic'), anchor='w', justify='left', wraplength=420); self.shutdown_status_label.pack(fill='x', padx=2, pady=(0, 6))

        # Middle row: input controls
        sf_mid = tk.Frame(shutdown_frame); sf_mid.pack(fill='x')
        self.shutdown_time_label = tk.Label(sf_mid, font=('Arial', 10)); self.shutdown_time_label.pack(side='left', padx=(0, 5))
        self.shutdown_entry = ttk.Entry(sf_mid, width=8, font=('Arial', 10)); self.shutdown_entry.pack(side='left', padx=5)
        self.shutdown_unit_var = tk.StringVar()
        self.shutdown_unit_menu = tk.OptionMenu(sf_mid, self.shutdown_unit_var, ""); self.shutdown_unit_menu.pack(side='left', padx=5)

        # Bottom row: action buttons
        sf_bottom = tk.Frame(shutdown_frame); sf_bottom.pack(fill='x', pady=(6, 0))
        sf_bottom.columnconfigure(0, weight=1); sf_bottom.columnconfigure(1, weight=1)

        self.shutdown_set_btn = ttkb.Button(sf_bottom, command=self.set_shutdown, bootstyle='primary')
        self.shutdown_set_btn.grid(row=0, column=0, sticky='ew', padx=(0,2))
        
        self.shutdown_cancel_btn = ttkb.Button(sf_bottom, command=self.cancel_shutdown, bootstyle='danger')
        self.shutdown_cancel_btn.grid(row=0, column=1, sticky='ew', padx=(2,0))

    def update_ui_language(self):
        lang = LANGUAGES[self.current_lang]
        self.root.title(lang['app_title'])
        self.title_label.config(text=lang['app_title'])
        self.version_label.config(text=lang['app_version'])
        self.win_setup_btn.config(text=lang['win_setup_btn'])
        self.net_setup_btn.config(text=lang['net_setup_btn'])
        self.bloat_remove_btn.config(text=lang['bloat_remove_btn'])
        self.wifi_backup_btn.config(text=lang['wifi_backup_btn'])
        self.driver_backup_btn.config(text=lang['driver_setup_btn'])
        self.status_var.set(lang['status_ready'])
        if self.net_win and self.net_win.winfo_exists(): self._update_network_window_lang()
        if self.bloat_win and self.bloat_win.winfo_exists(): self._update_bloatware_window_lang()
        if self.win_setup_win and self.win_setup_win.winfo_exists(): self._update_win_setup_window_lang()
        
        self.shutdown_frame.config(text=lang['shutdown_timer_frame'])
        self.shutdown_time_label.config(text=lang['shutdown_time_label'])
        self.shutdown_set_btn.config(text=lang['shutdown_set_btn'])
        self.shutdown_cancel_btn.config(text=lang['shutdown_cancel_btn'])
        self.shutdown_status_label.config(text=lang['shutdown_status_none'])
        
        menu = self.shutdown_unit_menu['menu']
        menu.delete(0, 'end')
        menu.add_command(label=lang['shutdown_unit_minutes'], command=tk._setit(self.shutdown_unit_var, lang['shutdown_unit_minutes']))
        menu.add_command(label=lang['shutdown_unit_hours'], command=tk._setit(self.shutdown_unit_var, lang['shutdown_unit_hours']))
        self.shutdown_unit_var.set(lang['shutdown_unit_minutes'])

    def switch_language(self, lang_name):
        self.current_lang = 'vi' if lang_name == 'Tiếng Việt' else 'en'
        self.update_ui_language()

    def _show_message(self, title_key, message_key, msg_type='info', parent=None, **kwargs):
        lang = LANGUAGES[self.current_lang]
        title = lang.get(title_key, title_key)
        message = lang.get(message_key, "???").format(**kwargs) if kwargs else lang.get(message_key, message_key)
        parent_win = parent if parent and parent.winfo_exists() else self.root
        
        if msg_type == 'info': messagebox.showinfo(title, message, parent=parent_win)
        elif msg_type == 'warning': messagebox.showwarning(title, message, parent=parent_win)
        elif msg_type == 'error': messagebox.showerror(title, message, parent=parent_win)
        elif msg_type == 'confirm': return messagebox.askyesno(title, message, parent=parent_win)

    def run_in_thread(self, target_func, *args):
        threading.Thread(target=target_func, args=args, daemon=True).start()

    def schedule_on_main_thread(self, func, *args, **kwargs):
        """Schedules a function to be called in the main GUI thread."""
        self.root.after(0, lambda: func(*args, **kwargs))

    def run_command(self, command, **kwargs):
        try:
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            result = subprocess.run(command, shell=True, capture_output=True, text=True, check=False, encoding='utf-8', errors='ignore', startupinfo=startupinfo, **kwargs)
            return result.stdout + result.stderr
        except Exception as e: return f"Execution error: {e}"

    def update_status(self, key, **kwargs):
        lang = LANGUAGES[self.current_lang]
        self.status_var.set(lang.get(key, key).format(**kwargs))
        self.root.update()
        
    def _set_window_icon(self, win):
        try:
            win.iconbitmap(resource_path("icon.ico"))
        except Exception:
            try:
                icon_png = resource_path("icon.png")
                if os.path.exists(icon_png):
                    win.iconphoto(True, tk.PhotoImage(file=icon_png))
            except Exception:
                pass

    def _center_window(self, win, width=None, height=None):
        win.update_idletasks()
        if width is None or height is None:
            width = win.winfo_width()
            height = win.winfo_height()
            if width <= 1 or height <= 1:
                width = win.winfo_reqwidth()
                height = win.winfo_reqheight()
        x = (win.winfo_screenwidth() // 2) - (width // 2)
        y = (win.winfo_screenheight() // 2) - (height // 2)
        win.geometry(f"{width}x{height}+{x}+{y}")

    # Progress helpers (main status bar)
    def _show_main_progress(self, maximum=100):
        try:
            self.main_progress['maximum'] = maximum
            self.main_progress['value'] = 0
            self.main_progress_var.set("0%")
            if not getattr(self, '_main_progress_visible', False):
                # Pack label first so it stays at the far right
                self.main_progress_label.pack(side='right', padx=(6, 8))
                self.main_progress.pack(side='right', padx=6)
                self._main_progress_visible = True
            self.root.update_idletasks()
        except Exception:
            pass

    def _update_main_progress(self, value):
        try:
            self.main_progress['value'] = value
            try:
                maximum = float(self.main_progress['maximum']) or 1.0
            except Exception:
                maximum = 1.0
            pct = int((float(value) / maximum) * 100)
            self.main_progress_var.set(f"{pct}%")
            self.root.update_idletasks()
        except Exception:
            pass

    def _hide_main_progress(self):
        try:
            if getattr(self, '_main_progress_visible', False):
                self.main_progress.pack_forget()
                self.main_progress_label.pack_forget()
                self._main_progress_visible = False
            self.main_progress_var.set("")
            self.root.update_idletasks()
        except Exception:
            pass

    def windows_setup(self):
        self.open_win_setup_window()

    def open_win_setup_window(self):
        if self.win_setup_win and self.win_setup_win.winfo_exists():
            self.win_setup_win.focus()
            return
        
        self.win_setup_win = tk.Toplevel(self.root)
        self.win_setup_win.geometry("500x300")
        self.win_setup_win.resizable(False, False)
        self.win_setup_win.transient(self.root)
        self.win_setup_win.grab_set()

        self._set_window_icon(self.win_setup_win)

        h_frame = tk.Frame(self.win_setup_win, bg='#3498db', height=60)
        h_frame.pack(fill='x')
        h_frame.pack_propagate(False)
        self.win_setup_title_lbl = tk.Label(h_frame, font=('Arial', 16, 'bold'), fg='white', bg='#3498db')
        self.win_setup_title_lbl.pack(pady=(5,0))
        self.win_setup_subtitle_lbl = tk.Label(h_frame, font=('Arial', 10), fg='#ecf0f1', bg='#3498db')
        self.win_setup_subtitle_lbl.pack()

        content_frame = tk.Frame(self.win_setup_win, bg='#f0f0f0')
        content_frame.pack(fill='both', expand=True, padx=20, pady=20)
        
        self.win_setup_stable_btn = ttkb.Button(content_frame, command=lambda: self._run_win_script("https://christitus.com/win"), bootstyle='primary')
        self.win_setup_stable_btn.pack(fill='x', pady=5)

        self.win_setup_dev_btn = ttkb.Button(content_frame, command=lambda: self._run_win_script("https://christitus.com/windev"), bootstyle='secondary')
        self.win_setup_dev_btn.pack(fill='x', pady=5)
        
        self.win_setup_info_lbl = tk.Label(content_frame, font=('Arial', 9, 'italic'), bg='#f0f0f0', wraplength=450, justify='center')
        self.win_setup_info_lbl.pack(pady=(15, 0))

        self._update_win_setup_window_lang()
        self._center_window(self.win_setup_win, 500, 300)

    def _update_win_setup_window_lang(self):
        lang = LANGUAGES[self.current_lang]
        self.win_setup_win.title(lang['win_setup_win_title'])
        self.win_setup_title_lbl.config(text=lang['win_setup_win_title'])
        self.win_setup_subtitle_lbl.config(text=lang['win_setup_header_subtitle'])
        self.win_setup_stable_btn.config(text=lang['win_setup_stable_btn'])
        self.win_setup_dev_btn.config(text=lang['win_setup_dev_btn'])
        self.win_setup_info_lbl.config(text=lang['win_setup_info'])

    def _run_win_script(self, url):
        self.update_status('status_running_win_script')
        command = f"powershell -NoExit -Command \"irm '{url}' | iex\""
        try:
            subprocess.Popen(command, shell=True)
        except Exception as e:
            title = LANGUAGES[self.current_lang]['title_error']
            messagebox.showerror(title, f"Failed to launch script: {e}", parent=self.win_setup_win)
        
        self.update_status('status_ready')

    def open_driver_window(self):
        if hasattr(self, 'driver_win') and self.driver_win and self.driver_win.winfo_exists():
            self.driver_win.focus(); return
        self.driver_win = tk.Toplevel(self.root)
        self.driver_win.geometry("500x300")
        self.driver_win.resizable(False, False)
        self.driver_win.transient(self.root)
        self.driver_win.grab_set()
        self._set_window_icon(self.driver_win)

        h_frame = tk.Frame(self.driver_win, bg='#27ae60', height=60)
        h_frame.pack(fill='x'); h_frame.pack_propagate(False)
        self.driver_title_lbl = tk.Label(h_frame, font=('Arial', 16, 'bold'), fg='white', bg='#27ae60'); self.driver_title_lbl.pack(pady=(5,0))
        self.driver_subtitle_lbl = tk.Label(h_frame, font=('Arial', 10), fg='#ecf0f1', bg='#27ae60'); self.driver_subtitle_lbl.pack()

        content_frame = tk.Frame(self.driver_win, bg='#f0f0f0')
        content_frame.pack(fill='both', expand=True, padx=20, pady=20)

        self.driver_backup_inner_btn = ttkb.Button(content_frame, bootstyle='success', command=self.backup_drivers)
        self.driver_backup_inner_btn.pack(fill='x', pady=6)

        self.driver_import_inner_btn = ttkb.Button(content_frame, bootstyle='primary', command=self.import_drivers)
        self.driver_import_inner_btn.pack(fill='x', pady=6)

        # Cancel button (visible only during driver operations)
        self.driver_cancel_btn = ttkb.Button(content_frame, bootstyle='danger', command=self.cancel_driver_operation)
        self.driver_cancel_btn.pack_forget()

        self._update_driver_window_lang()
        self._center_window(self.driver_win, 500, 300)

    def _update_driver_window_lang(self):
        lang = LANGUAGES[self.current_lang]
        if hasattr(self, 'driver_win') and self.driver_win and self.driver_win.winfo_exists():
            self.driver_win.title(lang['driver_win_title'])
            self.driver_title_lbl.config(text=lang['driver_win_title'])
            self.driver_subtitle_lbl.config(text=lang['driver_header_subtitle'])
            self.driver_backup_inner_btn.config(text=lang['driver_backup_inner_btn'])
            self.driver_import_inner_btn.config(text=lang['driver_import_inner_btn'])
            if hasattr(self, 'driver_cancel_btn'):
                self.driver_cancel_btn.config(text=lang['btn_cancel'])

    # ---- Driver operation state helpers ----
    def _set_driver_running_state(self, running):
        try:
            if running:
                if hasattr(self, 'driver_cancel_btn') and self.driver_win and self.driver_win.winfo_exists():
                    self.driver_cancel_btn.pack(fill='x', pady=6)
                if hasattr(self, 'driver_backup_inner_btn'): self.driver_backup_inner_btn.config(state='disabled')
                if hasattr(self, 'driver_import_inner_btn'): self.driver_import_inner_btn.config(state='disabled')
            else:
                if hasattr(self, 'driver_cancel_btn') and self.driver_win and self.driver_win.winfo_exists():
                    self.driver_cancel_btn.pack_forget()
                if hasattr(self, 'driver_backup_inner_btn'): self.driver_backup_inner_btn.config(state='normal')
                if hasattr(self, 'driver_import_inner_btn'): self.driver_import_inner_btn.config(state='normal')
            self.root.update_idletasks()
        except Exception:
            pass

    def cancel_driver_operation(self):
        # User-initiated cancel: mark flag and try to terminate process
        self._driver_cancel_requested = True
        self.update_status('status_canceling_drivers')
        self._terminate_driver_proc()

    def _terminate_driver_proc(self):
        proc = getattr(self, '_driver_proc', None)
        try:
            if proc and proc.poll() is None:
                try:
                    proc.terminate()
                except Exception:
                    pass
                try:
                    subprocess.run(f'taskkill /PID {proc.pid} /T /F', shell=True, capture_output=True)
                except Exception:
                    pass
        except Exception:
            pass

    def import_drivers(self):
        src_dir = filedialog.askdirectory(title=LANGUAGES[self.current_lang]['title_driver_import_dir'], parent=self.driver_win if hasattr(self, 'driver_win') else self.root)
        if not src_dir:
                    return
        self.update_status('status_importing_drivers')
        self._driver_cancel_requested = False
        self._show_main_progress(100)
        self._set_driver_running_state(True)
        self.run_in_thread(self._import_drivers_task, src_dir)
        # Tự đóng cửa sổ Driver Setup sau khi người dùng chọn xong
        try:
            if hasattr(self, 'driver_win') and self.driver_win and self.driver_win.winfo_exists():
                self.driver_win.destroy()
        except Exception:
            pass

    def _import_drivers_task(self, src_dir):
        # Tiến trình đã hiển thị từ thread chính
        try:
            cmd = f'pnputil /add-driver "{Path(src_dir) / "*.inf"}" /subdirs /install'
            startupinfo = subprocess.STARTUPINFO(); startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            creation = getattr(subprocess, 'CREATE_NEW_PROCESS_GROUP', 0)
            proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, startupinfo=startupinfo, creationflags=creation)
            self._driver_proc = proc
            fake_val = 0
            while True:
                ret = proc.poll()
                if getattr(self, '_driver_cancel_requested', False):
                    self._terminate_driver_proc()
                    break
                if ret is not None:
                    break
                if fake_val < 90:
                    fake_val = min(90, fake_val + 1)
                    self.schedule_on_main_thread(self._update_main_progress, fake_val)
                time.sleep(0.2)

            if getattr(self, '_driver_cancel_requested', False):
                self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_driver_cancelled', parent=self.root)
            elif proc.returncode == 0:
                self._update_main_progress(100)
                self._show_message('title_info', 'msg_driver_import_complete', parent=self.root)
            else:
                out = proc.stdout.read() if proc.stdout else ''
                self._update_main_progress(100)
                self._show_message('title_error', 'msg_driver_import_fail', msg_type='error', e=out, parent=self.root)
        except Exception as e:
            self._update_main_progress(100)
            self._show_message('title_error', 'msg_driver_import_fail', msg_type='error', e=str(e), parent=self.root)
        finally:
            self.update_status('status_ready')
            self._hide_main_progress()
            self._driver_proc = None
            self._driver_cancel_requested = False
            self.schedule_on_main_thread(self._set_driver_running_state, False)

    def backup_drivers(self):
        # Chọn thư mục đích
        target_dir = filedialog.askdirectory(title=LANGUAGES[self.current_lang]['title_driver_backup_dir'], parent=self.root)
        if not target_dir:
                    return
        # Cập nhật trạng thái và chạy nền
        self.update_status('status_backing_up_drivers')
        self._driver_cancel_requested = False
        self._show_main_progress(100)
        self._set_driver_running_state(True)
        self.run_in_thread(self._backup_drivers_task, target_dir)
        # Tự đóng cửa sổ Driver Setup sau khi người dùng chọn xong
        try:
            if hasattr(self, 'driver_win') and self.driver_win and self.driver_win.winfo_exists():
                self.driver_win.destroy()
        except Exception:
            pass

    def _backup_drivers_task(self, target_dir):
        # Tiến trình đã hiển thị từ thread chính
        try:
            base_dir = Path(target_dir)
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            backup_path = base_dir / f"Driver_Backup_{timestamp}"
            backup_path.mkdir(parents=True, exist_ok=True)

            # Chạy DISM export driver
            cmd = f'dism /online /export-driver /destination:"{backup_path}"'
            startupinfo = subprocess.STARTUPINFO(); startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            creation = getattr(subprocess, 'CREATE_NEW_PROCESS_GROUP', 0)
            proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, startupinfo=startupinfo, creationflags=creation)
            self._driver_proc = proc

            # Chờ tiến trình hoàn thành, cập nhật tiến trình giả lập
            fake_val = 0
            while True:
                ret = proc.poll()
                if getattr(self, '_driver_cancel_requested', False):
                    self._terminate_driver_proc()
                    break
                if ret is not None:
                    break
                if fake_val < 90:
                    fake_val = min(90, fake_val + 1)
                    self.schedule_on_main_thread(self._update_main_progress, fake_val)
                time.sleep(0.2)

            if getattr(self, '_driver_cancel_requested', False):
                self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_driver_cancelled', parent=self.root)
            elif proc.returncode == 0:
                self._update_main_progress(100)
                self._show_message('title_info', 'msg_driver_backup_complete', path=backup_path, parent=self.root)
                try:
                    os.startfile(backup_path)
                except Exception:
                    pass
            else:
                output = proc.stdout.read() if proc.stdout else ''
                self._update_main_progress(100)
                self._show_message('title_error', 'msg_driver_backup_fail', msg_type='error', e=output, parent=self.root)
        except Exception as e:
            self._update_main_progress(100)
            self._show_message('title_error', 'msg_driver_backup_fail', msg_type='error', e=str(e), parent=self.root)
        finally:
            self.update_status('status_ready')
            self._hide_main_progress()
            self._driver_proc = None
            self._driver_cancel_requested = False
            self.schedule_on_main_thread(self._set_driver_running_state, False)

    def open_bloatware_window(self):
        if self.bloat_win and self.bloat_win.winfo_exists(): self.bloat_win.focus(); return
        self.bloat_win = tk.Toplevel(self.root)
        self.bloat_win.geometry("900x600")
        self.bloat_win.resizable(True, True)
        self.bloat_win.transient(self.root)
        self.bloat_win.grab_set()
        self._set_window_icon(self.bloat_win)

        h_frame = tk.Frame(self.bloat_win, bg='#e74c3c', height=60); h_frame.pack(fill='x'); h_frame.pack_propagate(False)
        self.bloat_title_lbl = tk.Label(h_frame, font=('Arial', 16, 'bold'), fg='white', bg='#e74c3c'); self.bloat_title_lbl.pack(pady=(15, 5))
        self.bloat_subtitle_lbl = tk.Label(h_frame, font=('Arial', 10), fg='#ecf0f1', bg='#e74c3c'); self.bloat_subtitle_lbl.pack()

        # Content frame
        content_frame = tk.Frame(self.bloat_win, bg='#f0f0f0')
        content_frame.pack(fill='both', expand=True, padx=20, pady=20)

        # Buttons frame (ĐƯA LÊN TRƯỚC VÀ PACK VỀ PHÍA DƯỚI)
        button_frame = tk.Frame(content_frame, bg='#f0f0f0')
        button_frame.pack(side='bottom', fill='x', pady=(10, 0))
        
        # Danh sách bloatware (Bây giờ sẽ chiếm không gian còn lại)
        list_frame = tk.Frame(content_frame, bg='#f0f0f0')
        list_frame.pack(fill='both', expand=True)

        self.bloat_list_lbl = tk.Label(list_frame, font=('Arial', 12, 'bold'), bg='#f0f0f0')
        self.bloat_list_lbl.pack(anchor='w', pady=(0, 10))
        
        cols = ('Select', 'App Name', 'Package Name', 'Version', 'Location')
        self.bloat_tree = ttk.Treeview(list_frame, columns=cols, show='headings', height=15)
        style = ttk.Style(self.bloat_win)
        style.configure("Custom.Treeview", font=('Segoe UI', 10), rowheight=28)
        style.configure("Custom.Treeview.Heading", font=('Segoe UI', 10, 'bold'))
        self.bloat_tree.configure(style="Custom.Treeview")
        
        self.bloat_tree.column('Select', width=60, anchor='center')
        self.bloat_tree.column('App Name', width=250, anchor='w')
        self.bloat_tree.column('Package Name', width=200, anchor='w')
        self.bloat_tree.column('Version', width=100, anchor='center')
        self.bloat_tree.column('Location', width=200, anchor='w')
        
        scroll = ttk.Scrollbar(list_frame, orient='vertical', command=self.bloat_tree.yview)
        self.bloat_tree.configure(yscrollcommand=scroll.set)
        
        self.bloat_tree.pack(side='left', fill='both', expand=True)
        scroll.pack(side='right', fill='y')
        
        btn_pady = 8
        self.bloat_load_btn = ttkb.Button(button_frame, command=self.load_bloatware_list, bootstyle='primary')
        self.bloat_load_btn.pack(side='left', padx=(0, 10), fill='x', expand=True)
        
        self.bloat_select_all_btn = ttkb.Button(button_frame, command=self.select_all_bloatware, bootstyle='success')
        self.bloat_select_all_btn.pack(side='left', padx=(0, 10), fill='x', expand=True)

        self.bloat_deselect_all_btn = ttkb.Button(button_frame, command=self.deselect_all_bloatware, bootstyle='secondary')
        self.bloat_deselect_all_btn.pack(side='left', padx=(0, 10), fill='x', expand=True)

        self.bloat_remove_btn = ttkb.Button(button_frame, command=self.remove_selected_bloatware, bootstyle='danger')
        self.bloat_remove_btn.pack(side='right', padx=(10, 0), fill='x', expand=True)
        
        s_frame = tk.Frame(self.bloat_win, bg='#34495e', height=30)
        s_frame.pack(fill='x', side='bottom')
        s_frame.pack_propagate(False)
        self.bloat_status_var = tk.StringVar()
        tk.Label(s_frame, textvariable=self.bloat_status_var, font=('Arial', 9), fg='white', bg='#34495e').pack(side='left', padx=10, pady=5, fill='x', expand=True)
        # Progress bar for bloatware deletion (show only when running)
        self.bloat_progress = ttk.Progressbar(s_frame, length=220, mode='determinate')
        self.bloat_progress_var = tk.StringVar(value="")
        self.bloat_progress_label = tk.Label(s_frame, textvariable=self.bloat_progress_var, font=('Arial', 9), fg='white', bg='#34495e')
        self._bloat_progress_visible = False
        
        self.bloat_tree.bind('<Button-1>', self.toggle_bloatware_selection)
        
        self._update_bloatware_window_lang()
        self.bloat_win.after(100, lambda: self.run_in_thread(self.load_bloatware_list))
        self._center_window(self.bloat_win, 900, 600)

    def _update_bloatware_window_lang(self):
        lang = LANGUAGES[self.current_lang]
        self.bloat_win.title(lang['bloat_win_title'])
        self.bloat_title_lbl.config(text=lang['bloat_win_title'])
        self.bloat_subtitle_lbl.config(text=lang['bloat_header_subtitle'])
        self.bloat_list_lbl.config(text=lang['bloat_list_label'])
        self.bloat_tree.heading('Select', text=lang['bloat_col_select']); self.bloat_tree.heading('App Name', text=lang['bloat_col_app_name'])
        self.bloat_tree.heading('Package Name', text=lang['bloat_col_package_name']); self.bloat_tree.heading('Version', text=lang['bloat_col_version']); self.bloat_tree.heading('Location', text=lang['bloat_col_location'])
        self.bloat_load_btn.config(text=lang['bloat_load_btn']); self.bloat_select_all_btn.config(text=lang['bloat_select_all_btn'])
        self.bloat_deselect_all_btn.config(text=lang['bloat_deselect_all_btn']); self.bloat_remove_btn.config(text=lang['bloat_remove_selected_btn'])
        self.bloat_status_var.set(lang['status_ready'])

    def load_bloatware_list(self):
        self.schedule_on_main_thread(self.update_status, 'status_loading_bloatware')
        apps_to_add = []
        try:
            cmd = 'Get-AppxPackage | Where-Object {$_.NonRemovable -eq $false} | Select-Object Name,PackageFullName,Version,InstallLocation | Sort-Object Name | ConvertTo-Json'
            result = self.run_command(f'powershell -ExecutionPolicy Bypass -Command "{cmd}"')
            if not result or "Error" in result:
                raise Exception(result)
            apps = json.loads(result)
            if isinstance(apps, dict):
                apps = [apps]
            for app in apps:
                apps_to_add.append(('☐', app.get('Name', 'N/A'), app.get('PackageFullName', 'N/A'), app.get('Version', 'N/A'), app.get('InstallLocation', 'N/A')))
            def update_gui():
                for i in self.bloat_tree.get_children():
                    self.bloat_tree.delete(i)
                for app_data in apps_to_add:
                    self.bloat_tree.insert('', 'end', values=app_data)
                self.update_status('status_ready')
            self.schedule_on_main_thread(update_gui)
        except Exception as e:
            self.schedule_on_main_thread(self._show_message, 'title_error', 'msg_bloat_load_fail', msg_type='error', parent=self.bloat_win, e=str(e))
            self.schedule_on_main_thread(self.update_status, 'status_ready')

    def toggle_bloatware_selection(self, event):
        item_id = self.bloat_tree.identify_row(event.y)
        if not item_id: return
        if self.bloat_tree.identify_column(event.x) == '#1':
            current_values = self.bloat_tree.item(item_id, 'values'); new_symbol = '☑' if current_values[0] == '☐' else '☐'
            new_values = (new_symbol,) + current_values[1:]; self.bloat_tree.item(item_id, values=new_values)
    def select_all_bloatware(self, select=True):
        for item in self.bloat_tree.get_children():
            vals = self.bloat_tree.item(item, 'values'); new_vals = ('☑' if select else '☐',) + vals[1:]
            self.bloat_tree.item(item, values=new_vals)
    def deselect_all_bloatware(self): self.select_all_bloatware(select=False)
    def remove_selected_bloatware(self):
        items = [(self.bloat_tree.item(i, 'values')[2], self.bloat_tree.item(i, 'values')[1], i) for i in self.bloat_tree.get_children() if self.bloat_tree.item(i, 'values')[0] == '☑']
        if not items: self._show_message('title_warning', 'msg_bloat_no_app_selected', msg_type='warning', parent=self.bloat_win); return
        if self._confirm_bloatware_removal(items):
            self.run_in_thread(self._remove_bloatware_task, items)

    def _confirm_bloatware_removal(self, items):
        lang = LANGUAGES[self.current_lang]
        dlg = tk.Toplevel(self.bloat_win)
        dlg.title(lang['title_confirm'])
        dlg.geometry("420x320")
        dlg.resizable(False, False)
        dlg.transient(self.bloat_win)
        dlg.grab_set()
        self._set_window_icon(dlg)

        container = tk.Frame(dlg, bg='#f0f0f0')
        container.pack(fill='both', expand=True, padx=12, pady=12)

        # Build intro and warning from existing message template
        tmpl = lang.get('msg_confirm_bloat_remove', '')
        lines = tmpl.split('\n') if tmpl else []
        intro = lines[0] if lines else ''
        intro = intro.format(count=len(items), app_list="")
        warning = lines[-1] if len(lines) >= 2 else ''

        tk.Label(container, text=intro, font=('Arial', 10, 'bold'), bg='#f0f0f0', anchor='w', justify='left', wraplength=380).pack(fill='x')

        list_frame = tk.Frame(container, bg='#f0f0f0')
        list_frame.pack(fill='both', expand=True, pady=(8, 8))
        lb = tk.Listbox(list_frame, height=8)
        sb = ttk.Scrollbar(list_frame, orient='vertical', command=lb.yview)
        lb.configure(yscrollcommand=sb.set)
        lb.pack(side='left', fill='both', expand=True)
        sb.pack(side='right', fill='y')
        for _, name, _ in items:
            lb.insert('end', f"• {name}")

        if warning:
            tk.Label(container, text=warning, fg='#e74c3c', bg='#f0f0f0', anchor='w', justify='left', wraplength=380).pack(fill='x')

        btn_frame = tk.Frame(container, bg='#f0f0f0')
        btn_frame.pack(fill='x', pady=(8, 0))
        result = {'val': False}
        def on_ok():
            result['val'] = True
            dlg.destroy()
        def on_cancel():
            dlg.destroy()
            
        ok_btn = ttkb.Button(btn_frame, text=lang['btn_confirm'], command=on_ok, bootstyle='danger')
        ok_btn.pack(side='right', padx=5)

        cancel_btn = ttkb.Button(btn_frame, text=lang['btn_cancel'], command=on_cancel, bootstyle='secondary')
        cancel_btn.pack(side='right', padx=5)
        
        dlg.bind('<Return>', lambda e: on_ok())
        dlg.bind('<Escape>', lambda e: on_cancel())
        self._center_window(dlg, 420, 320)
        dlg.wait_window()
        return result['val']
    def _remove_bloatware_task(self, items):
        s_count = 0; failed = []
        # Show progress bar
        self.schedule_on_main_thread(self._show_bloat_progress, len(items))
        for i, (pkg, name, item_id) in enumerate(items, 1):
            self.schedule_on_main_thread(self.update_status, 'status_removing_bloatware', i=i, total=len(items), name=name)
            try:
                cmd = f'Remove-AppxPackage -Package "{pkg}" -AllUsers'; result = self.run_command(f'powershell -Command "{cmd}"')
                if "Error" in result: raise Exception(result)
                self.schedule_on_main_thread(self.bloat_tree.delete, item_id); s_count += 1
            except Exception as e: failed.append(f'• {name}: {e}')
            self.schedule_on_main_thread(self._update_bloat_progress, i)
        
        errors_str = '\n'.join(failed) if failed else "None"
        self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_bloat_remove_complete', parent=self.bloat_win, s=s_count, t=len(items), errors=errors_str)
        self.schedule_on_main_thread(self.update_status, 'status_ready')
        self.schedule_on_main_thread(self._hide_bloat_progress)

    # Progress helpers (bloatware window)
    def _show_bloat_progress(self, maximum=100):
        try:
            self.bloat_progress['maximum'] = maximum
            self.bloat_progress['value'] = 0
            self.bloat_progress_var.set("0%")
            if not getattr(self, '_bloat_progress_visible', False):
                # Pack label first to keep it at the far right
                self.bloat_progress_label.pack(side='right', padx=(10, 12))
                self.bloat_progress.pack(side='right', padx=10)
                self._bloat_progress_visible = True
            if self.bloat_win and self.bloat_win.winfo_exists():
                self.bloat_win.update_idletasks()
        except Exception:
            pass

    def _update_bloat_progress(self, value):
        try:
            self.bloat_progress['value'] = value
            try:
                maximum = float(self.bloat_progress['maximum']) or 1.0
            except Exception:
                maximum = 1.0
            pct = int((float(value) / maximum) * 100)
            self.bloat_progress_var.set(f"{pct}%")
            if self.bloat_win and self.bloat_win.winfo_exists():
                self.bloat_win.update_idletasks()
        except Exception:
            pass

    def _hide_bloat_progress(self):
        try:
            if getattr(self, '_bloat_progress_visible', False):
                self.bloat_progress.pack_forget()
                self.bloat_progress_label.pack_forget()
                self._bloat_progress_visible = False
            self.bloat_progress_var.set("")
            if self.bloat_win and self.bloat_win.winfo_exists():
                self.bloat_win.update_idletasks()
        except Exception:
            pass

    def open_network_window(self):
        if self.net_win and self.net_win.winfo_exists(): self.net_win.focus(); return
        self.net_win = tk.Toplevel(self.root); self.net_win.geometry("980x500")
        self.net_win.transient(self.root); self.net_win.grab_set()
        self._set_window_icon(self.net_win)
        
        info_frame = ttk.LabelFrame(self.net_win, padding=(10, 10)); info_frame.pack(fill='x', padx=10, pady=10)
        cols = ('Name', 'Type', 'IP', 'MAC', 'DNS'); self.net_tree = ttk.Treeview(info_frame, columns=cols, show='headings', height=8)
        self.net_tree.column('Name', width=150, anchor='w'); self.net_tree.column('Type', width=80, anchor='w'); self.net_tree.column('IP', width=120, anchor='w'); self.net_tree.column('MAC', width=150, anchor='w'); self.net_tree.column('DNS', width=250, anchor='w')
        self.net_tree.pack(fill='x', expand=True)
        
        self.net_refresh_btn = ttkb.Button(info_frame, command=lambda: self.run_in_thread(self.refresh_network_info), bootstyle='primary')
        self.net_refresh_btn.pack(pady=5, padx=5, fill='x')
        
        actions_frame = ttk.LabelFrame(self.net_win, padding=(10, 10)); actions_frame.pack(fill='x', padx=10, pady=10)
        
        r1 = tk.Frame(actions_frame); r1.pack(fill='x', pady=5)
        r1.columnconfigure(0, weight=1); r1.columnconfigure(1, weight=1); r1.columnconfigure(2, weight=1)
        self.net_flush_dns_btn = ttkb.Button(r1, command=self.flush_dns, bootstyle='secondary')
        self.net_flush_dns_btn.grid(row=0, column=0, sticky='ew', padx=(0, 3))
        
        self.net_reset_tcp_btn = ttkb.Button(r1, command=self.reset_tcp_ip, bootstyle='secondary')
        self.net_reset_tcp_btn.grid(row=0, column=1, sticky='ew', padx=3)

        self.net_restore_wifi_btn = ttkb.Button(r1, command=self.restore_wifi, bootstyle='secondary')
        self.net_restore_wifi_btn.grid(row=0, column=2, sticky='ew', padx=(3, 0))
        
        dns_frame = ttk.LabelFrame(actions_frame, padding=(10, 5)); dns_frame.pack(fill='x', pady=10)
        dns_frame.columnconfigure(0, weight=1); dns_frame.columnconfigure(1, weight=1); dns_frame.columnconfigure(2, weight=1)

        self.net_google_dns_btn = ttkb.Button(dns_frame, text="Google DNS", command=lambda: self.change_dns("8.8.8.8", "8.8.4.4"), bootstyle='primary')
        self.net_google_dns_btn.grid(row=0, column=0, sticky='ew', padx=(0,3))

        self.net_cf_dns_btn = ttkb.Button(dns_frame, text="Cloudflare DNS", command=lambda: self.change_dns("1.1.1.1", "1.0.0.1"), bootstyle='warning')
        self.net_cf_dns_btn.grid(row=0, column=1, sticky='ew', padx=3)

        self.net_clear_dns_btn = ttkb.Button(dns_frame, command=lambda: self.change_dns(), bootstyle='danger')
        self.net_clear_dns_btn.grid(row=0, column=2, sticky='ew', padx=(3,0))
        
        self._update_network_window_lang(); self.run_in_thread(self.refresh_network_info)
        self._center_window(self.net_win, 980, 500)

    def _update_network_window_lang(self):
        lang = LANGUAGES[self.current_lang]
        self.net_win.title(lang['net_win_title'])
        self.net_win.winfo_children()[0].config(text=lang['net_info_frame'])
        self.net_win.winfo_children()[1].config(text=lang['net_actions_frame'])
        self.net_win.winfo_children()[1].winfo_children()[1].config(text=lang['net_dns_frame'])

        self.net_tree.heading('Name', text=lang['net_col_name']); self.net_tree.heading('Type', text=lang['net_col_type']); self.net_tree.heading('IP', text=lang['net_col_ip'])
        self.net_tree.heading('MAC', text=lang['net_col_mac']); self.net_tree.heading('DNS', text=lang['net_col_dns'])
        self.net_refresh_btn.config(text=lang['net_refresh_btn'])
        self.net_flush_dns_btn.config(text=lang['net_flush_dns_btn'])
        self.net_reset_tcp_btn.config(text=lang['net_reset_tcp_btn'])
        self.net_restore_wifi_btn.config(text=lang['net_restore_wifi_btn'])
        self.net_google_dns_btn.config(text=lang['net_google_dns'])
        self.net_cf_dns_btn.config(text=lang['net_cloudflare_dns'])
        self.net_clear_dns_btn.config(text=lang['net_clear_dns_btn'])

    def _get_dns_servers(self, iface_name):
        try:
            result = self.run_command(f'netsh interface ipv4 show dnsservers name="{iface_name}"')
            if "none" in result.lower(): return "Automatic (DHCP)"
            servers = re.findall(r'\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b', result)
            return ", ".join(servers) if servers else "None"
        except Exception: return "Cannot fetch"
    def refresh_network_info(self):
        self.schedule_on_main_thread(self.update_status, 'status_loading_net')
        try:
            ifaces_data = []
            addrs = psutil.net_if_addrs(); stats = psutil.net_if_stats()
            for intf, addr_list in addrs.items():
                ip = mac = ""; is_up = "Up" if stats.get(intf) and stats[intf].isup else "Down"
                for addr in addr_list:
                    if addr.family == psutil.AF_LINK: mac = addr.address
                    if addr.family == 2: ip = addr.address
                dns = self._get_dns_servers(intf)
                ifaces_data.append((intf, is_up, ip, mac, dns))
            def update_gui():
                try:
                    focused_item = self.net_tree.focus()
                    for i in self.net_tree.get_children(): self.net_tree.delete(i)
                    for data in ifaces_data:
                        self.net_tree.insert("", "end", values=data)
                    if focused_item and self.net_tree.exists(focused_item):
                        self.net_tree.focus(focused_item)
                        self.net_tree.selection_set(focused_item)
                finally:
                    self.update_status('status_ready')
            self.schedule_on_main_thread(update_gui)
        except Exception as e:
            self.schedule_on_main_thread(self._show_message, 'title_error', 'msg_net_fetch_fail', msg_type='error', parent=self.net_win, e=str(e))
            self.schedule_on_main_thread(self.update_status, 'status_ready')
                
    def _has_wireless_interface(self):
        try:
            result = self.run_command("netsh wlan show interfaces")
            return not ("no wireless interface" in result or "không có giao diện" in result)
        except Exception: return False
    def _ensure_wlansvc_running(self):
        try:
            svc = psutil.win_service_get('wlansvc')
            if svc.status() == 'running': return True
        except psutil.NoSuchProcess:
            self._show_message('title_error', 'msg_wlan_not_found', msg_type='error'); return False
        self.update_status('status_starting_wlan'); self.run_command("net start wlansvc"); time.sleep(2)
        try:
            svc = psutil.win_service_get('wlansvc')
            if svc.status() == 'running': return True
        except psutil.NoSuchProcess:
            pass
        self._show_message('title_error', 'msg_wlan_cannot_start', msg_type='error'); return False
    def restore_wifi(self):
        if not self._has_wireless_interface(): self._show_message('title_warning', 'msg_wifi_no_card', msg_type='warning'); return
        files = filedialog.askopenfilenames(title=LANGUAGES[self.current_lang]['title_wifi_restore'], filetypes=[("XML files", "*.xml"), ("All files", "*.*")], parent=self.net_win)
        if files: self.run_in_thread(self._restore_wifi_task, files)
    def _restore_wifi_task(self, files):
        if not self._ensure_wlansvc_running(): self.update_status('status_ready'); return
        s_count = 0
        for i, f in enumerate(files, 1):
            self.schedule_on_main_thread(self.update_status, 'status_restoring_wifi', i=i, total=len(files))
            result = self.run_command(f'netsh wlan add profile filename="{f}" user=all')
            if "is added on interface" in result.lower() or "được thêm trên giao diện" in result.lower(): s_count += 1
        self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_wifi_restore_complete', s=s_count, t=len(files), parent=self.net_win)
        self.update_status('status_ready')
    def flush_dns(self):
        self.update_status('status_flushing_dns'); self.run_in_thread(self._flush_dns_task)
    def _flush_dns_task(self):
        result = self.run_command("ipconfig /flushdns")
        if "successfully flushed" in result.lower() or "đã xóa thành công" in result.lower():
            self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_dns_flush_success', parent=self.net_win)
        else:
            self.schedule_on_main_thread(self._show_message, 'title_error', 'msg_dns_flush_fail', msg_type='error', parent=self.net_win)
        self.update_status('status_ready')
    def reset_tcp_ip(self):
        self.update_status('status_resettiing_tcp'); self.run_in_thread(self._reset_tcp_ip_task)
    def _reset_tcp_ip_task(self):
        result = self.run_command("netsh int ip reset")
        if "resetting" in result.lower() and "ok" in result.lower():
            self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_reset_tcp_success', parent=self.net_win)
        else:
            self.schedule_on_main_thread(self._show_message, 'title_error', 'msg_reset_tcp_fail', msg_type='error', parent=self.net_win)
        self.update_status('status_ready')
    def change_dns(self, dns1=None, dns2=None):
        item = self.net_tree.focus()
        if not item: self._show_message('title_warning', 'msg_net_no_iface_selected', msg_type='warning', parent=self.net_win); return
        iface = self.net_tree.item(item)['values'][0]
        self.update_status('status_changing_dns', iface=iface); self.run_in_thread(self._change_dns_task, iface, dns1, dns2)
    def _change_dns_task(self, iface, dns1, dns2):
        if dns1 is None: cmd = f'netsh interface ipv4 set dnsserver name="{iface}" source=dhcp'
        else: cmd = f'netsh interface ipv4 set dnsserver name="{iface}" static {dns1} primary'
        res1 = self.run_command(cmd)
        if dns1 and dns2: self.run_command(f'netsh interface ipv4 add dnsserver name="{iface}" {dns2} index=2')
        
        success = not res1.strip() or "The requested operation requires elevation" not in res1
        
        if success:
            self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_dns_change_success', parent=self.net_win, iface=iface)
        else:
            self.schedule_on_main_thread(self._show_message, 'title_error', 'msg_dns_change_fail', msg_type='error', parent=self.net_win, iface=iface)
        
        self.run_in_thread(self.refresh_network_info)
        self.update_status('status_ready')

    def backup_wifi(self):
        if not self._has_wireless_interface():
            self._show_message('title_warning', 'msg_wifi_no_card', msg_type='warning')
            return
        backup_dir = filedialog.askdirectory(title=LANGUAGES[self.current_lang]['title_wifi_backup_dir'])
        if not backup_dir:
            return
        self.update_status('status_backing_up_wifi')
        self.run_in_thread(self._backup_wifi_task, backup_dir)

    def _backup_wifi_task(self, backup_dir):
        if not self._ensure_wlansvc_running():
            self.schedule_on_main_thread(self.update_status, 'status_ready')
            return
        logs = []
        try:
            self.schedule_on_main_thread(self.update_status, 'status_loading_wifi_profiles')
            profiles_res = self.run_command("netsh wlan show profiles")
            profiles = []
            for line in profiles_res.split('\n'):
                if 'All User Profile' in line or 'Hồ sơ Tất cả Người dùng' in line:
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        profiles.append(parts[1].strip())
            if not profiles:
                self.schedule_on_main_thread(self._show_message, 'title_warning', 'msg_wifi_no_profile_to_backup', msg_type='warning')
                return
            self.schedule_on_main_thread(self._show_main_progress, len(profiles))
            base_dir = Path(backup_dir)
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            backup_path = base_dir / f"WiFi_Backup_{timestamp}"
            backup_path.mkdir(parents=True, exist_ok=True)
            wifi_data = []
            s_count = 0
            for i, p in enumerate(profiles, 1):
                self.schedule_on_main_thread(self.update_status, 'status_backing_up_wifi', i=i, total=len(profiles), profile=p)
                pwd = "N/A"
                try:
                    export_output = self.run_command(f'netsh wlan export profile name="{p}" folder="{backup_path}" key=clear')
                    match = re.search(r'file "(.*?)"', export_output)
                    if match:
                        xml_file_path = Path(match.group(1))
                        if xml_file_path.exists():
                            try:
                                tree = ET.parse(xml_file_path)
                                root = tree.getroot()
                                ns = {'wlan': 'http://www.microsoft.com/networking/WLAN/profile/v1'}
                                key_material = root.find('.//wlan:MSM/wlan:security/wlan:sharedKey/wlan:keyMaterial', ns)
                                if key_material is not None and key_material.text:
                                    pwd = key_material.text
                                else:
                                    pwd = "N/A (No Key Material)"
                            except ET.ParseError:
                                pwd = "N/A (XML Parse Error)"
                        else:
                            pwd = "N/A (XML file not found)"
                    else:
                        res_show = self.run_command(f'netsh wlan show profile name="{p}" key=clear')
                        for res_l in res_show.split('\n'):
                            if "Key Content" in res_l or "Nội dung Khóa" in res_l:
                                pwd = res_l.split(':', 1)[1].strip()
                                break
                    wifi_data.append(f"SSID: {p} : {pwd}")
                    s_count += 1
                except Exception as e:
                    logs.append(f"Error with profile '{p}': {e}")
                self.schedule_on_main_thread(self._update_main_progress, i)
            if wifi_data:
                with open(backup_path / "WiFi_Passwords.txt", "w", encoding="utf-8") as f:
                    f.write("\n".join(wifi_data))
            self.schedule_on_main_thread(self._show_message, 'title_info', 'msg_wifi_backup_complete', s=s_count, t=len(profiles), path=backup_path, parent=self.root)
            os.startfile(backup_path)
        except Exception as e:
            log_file = Path.home() / "windows_utility_tool_wifi_log.txt"
            logs.append(f"Critical error: {e}")
            with open(log_file, "w", encoding="utf-8") as log:
                log.write("\n".join(logs))
            def show_error():
                self._show_message('title_error', 'msg_critical_error_log', msg_type='error', path=log_file)
            self.schedule_on_main_thread(show_error)
        finally:
            self.schedule_on_main_thread(self.update_status, 'status_ready')
            self.schedule_on_main_thread(self._hide_main_progress)

    def set_shutdown(self):
        """Hẹn giờ tắt máy theo khoảng thời gian và ép buộc đóng ứng dụng."""
        try:
            value = int(self.shutdown_entry.get())
            unit_str = self.shutdown_unit_var.get()
            lang = LANGUAGES[self.current_lang]
            
            if value <= 0:
                raise ValueError("Value must be positive")

            delay_seconds = 0
            if unit_str == lang['shutdown_unit_minutes']:
                delay_seconds = value * 60
            elif unit_str == lang['shutdown_unit_hours']:
                delay_seconds = value * 3600
            
            # Sử dụng /f để ép buộc đóng ứng dụng
            self.run_command(f"shutdown /s /f /t {delay_seconds}")

            # Lưu lịch hẹn của app để kiểm tra về sau mà không cần hủy lệnh hệ thống
            try:
                store = self._schedule_store_path()
                data = {"scheduled_at": time.time(), "delay_seconds": delay_seconds}
                with open(store, "w", encoding="utf-8") as f:
                    json.dump(data, f)
            except Exception:
                pass

            self.shutdown_status_label.config(text=lang['shutdown_status_set'].format(value=value, unit=unit_str.lower()))
            self._show_message('title_info', 'shutdown_status_set', value=value, unit=unit_str.lower())

        except (ValueError, TclError):
            self._show_message('title_error', 'shutdown_invalid_format', msg_type='error')

    def cancel_shutdown(self):
        """Hủy lệnh hẹn giờ tắt máy."""
        self.run_command("shutdown /a")
        lang = LANGUAGES[self.current_lang]
        self.shutdown_status_label.config(text=lang['shutdown_status_none'])
        self._show_message('title_info', 'shutdown_status_none')
        # Xóa lịch hẹn đã lưu của app (nếu có)
        try:
            store = self._schedule_store_path()
            if store.exists():
                store.unlink()
        except Exception:
            pass

    def check_existing_shutdown_schedule(self):
        """Kiểm tra lịch tắt máy do chính app đặt (không hủy lệnh hệ thống).
        Nếu file lưu lịch tồn tại và ETA còn ở tương lai, hiển thị trạng thái đang hẹn.
        """
        lang = LANGUAGES[self.current_lang]
        try:
            store = self._schedule_store_path()
            if not store.exists():
                self.shutdown_status_label.config(text=lang['shutdown_status_none'])
                return
            with open(store, "r", encoding="utf-8") as f:
                data = json.load(f)
            delay = int(data.get("delay_seconds", 0))
            scheduled_at = float(data.get("scheduled_at", 0))
            eta = scheduled_at + delay
            now = time.time()
            if delay <= 0 or now >= eta:
                # Hết hạn -> xóa file
                try:
                    store.unlink()
                except Exception:
                    pass
                self.shutdown_status_label.config(text=lang['shutdown_status_none'])
                return
            remaining = int(eta - now)
            # Ưu tiên hiển thị theo giờ nếu >= 3600s, ngược lại theo phút
            if remaining >= 3600:
                hours = max(1, remaining // 3600)
                self.shutdown_status_label.config(text=lang['shutdown_status_set'].format(value=hours, unit=lang['shutdown_unit_hours'].lower()))
            else:
                minutes = max(1, (remaining + 59) // 60)
                self.shutdown_status_label.config(text=lang['shutdown_status_set'].format(value=minutes, unit=lang['shutdown_unit_minutes'].lower()))
        except Exception:
            self.shutdown_status_label.config(text=lang['shutdown_status_none'])

def main():
    if os.name != 'nt':
        messagebox.showerror("Unsupported OS", "This tool only runs on Windows.")
        return
    try:
        with SingleInstance("WindowsUtilityTool_Doctoten_App_Mutex"):
            if not is_admin():
                try:
                    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " ".join(sys.argv), None, 1)
                except Exception as e:
                    messagebox.showerror("Admin Rights Required", f"Could not request administrator privileges.\nError: {e}")
                return
            set_current_process_app_id("Doctoten.WindowsUtilityTool")
            root = ttkb.Window(themename='flatly')
            app = WindowsUtilityTool(root)
            root.mainloop()
    except RuntimeError:
        _show_message_before_init('vi', 'title_warning', 'app_already_running_msg')
    except Exception as e:
        messagebox.showerror("Critical Application Error", f"A critical error occurred:\n{e}")

def _show_message_before_init(lang_key, title_key, message_key):
    lang = LANGUAGES.get(lang_key, LANGUAGES.get('en', {}))
    title = lang.get(title_key, "Warning")
    message = lang.get(message_key, "An unknown error occurred.")
    messagebox.showwarning(title, message)

if __name__ == "__main__":
    main()