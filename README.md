# ServerCheck 🛡️ (C++ Process WatchGuard)

A lightweight, system-level process monitor designed to keep game servers and web services running 24/7. This is a core component of my infrastructure at **nebirrs-lab.de**.

## 🚀 Features
- **Real-time Monitoring:** Tracks multiple processes simultaneously using the Windows Toolhelp32 API.
- **Auto-Recovery:** Automatically restarts crashed or closed processes via defined `.bat` or `.exe` paths.
- **Discord Integration:** Sends instant alerts to a Discord webhook when a service fails (with a 10-minute anti-spam cooldown).
- **Web Dashboard Integration:** Exports the current server status to a `status.json` file for web-based monitoring (Nginx/HTML).

## 🛠️ Technical Focus
- **Language:** C++
- **Platform:** Windows (Win32 API)
- **Concepts:** Multithreading, File I/O, Chrono (Time Management), Windows Process Snapshots.

## 📦 Setup & Usage
1. **Configure Servers:** Create a `config.txt` in the root folder. Each server needs two lines:
   ```text
   valheim.exe
   C:\Servers\Valheim\start.bat