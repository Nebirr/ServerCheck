# ServerCheck 🛡️ (Universal C++ WatchGuard)

**ServerCheck** is a lightweight, modular system-level process monitor designed to ensure 24/7 uptime for game servers, web services, and background processes. This tool is a core component of the automated infrastructure at **nebirrs-lab.de**.

---

## 🚀 Key Features

- **Universal Monitoring:** Tracks any Windows process using the `Win32 Toolhelp32 API`.
- **Friendly Names:** Support for human-readable display names (e.g., "Valheim Main Server" instead of `valheim_server.exe`).
- **Modular Log Scraping:** Dynamically extracts specific data from log files (such as Join-Codes, Ports, or Session IDs) via user-defined search terms.
- **Smart Auto-Recovery:** Instantly detects crashes, restarts services via `.bat` or `.exe`, and actively monitors the startup phase.
- **Discord Integration:** Real-time alerts via Webhooks, featuring a built-in 10-minute anti-spam cooldown per server.
- **Web Dashboard Ready:** Exports the live status of all monitored servers to a `status.json` file for integration with Nginx or web frontends.

---

## ⚙️ Setup & Requirements

### 1. Configure Discord Webhook
Create a file named `webhook.txt` in the program's root directory and paste your Discord Webhook URL into it:
```text
[https://discord.com/api/webhooks/your_id/your_token](https://discord.com/api/webhooks/your_id/your_token)

### 2. Configure Servers
Create a config.txt in the root directory. Each monitored server requires a block of exactly 5 lines:

1. Display Name (e.g., Valheim Community Server)

2. Process Name (e.g., valheim_server.exe)

3. Restart Path (Full path to the .bat or .exe file)

4. Log Path (Path to the log file or NONE)

5. Search Term (The string to search for in the log, e.g., join code  or NONE)

---

## 📝 Example ## config.txt

Valheim Modded
valheim_server.exe
C:\Servers\Valheim\start.bat
C:\Servers\Valheim\BepInEx\LogOutput.log
join code 
Minecraft Survival
minecraft_server.exe
D:\Minecraft\run.bat
D:\Minecraft\logs\latest.log
Port: 
Web-Service Node
node.exe
C:\Scripts\start_web.bat
NONE
NONE

---

## 🛠️ Technical Focus
Language: C++ (ISO C++17 Standard)

Platform: Windows (Win32 API)

Core Concepts: - Process Snapshots: Efficient querying of running processes with minimal system impact.

Modular Log Parsing: Dynamic searching within file streams during runtime.

Chrono Timekeeping: Precise control over alert intervals and boot duration measurement.

UTF-8/UTF-16 Conversion: Clean handling of Windows special characters for web service compatibility.

---

## 📄 License
This project is licensed under the MIT License. See the LICENSE file for more details.

---
