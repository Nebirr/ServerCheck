#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <sstream>
#include <chrono> 
#include <ctime>

struct ServerInfo {
    std::wstring exeName;
    std::wstring batPfad;
    std::wstring logPfad; 
    std::chrono::steady_clock::time_point letzterAlarm;
};

std::wstring holeJoinCode(std::wstring logPfad) {
    if (logPfad == L"NONE" || logPfad.empty()) return L"";

    std::ifstream file(logPfad);
    std::string line;
    std::string letzterCode = "";

    if (file.is_open()) {
        while (std::getline(file, line)) {
            size_t pos = line.find("join code ");
            if (pos != std::string::npos) {
                letzterCode = line.substr(pos + 10);
                letzterCode.erase(letzterCode.find_last_not_of(" \n\r\t") + 1);
            }
        }
        file.close();
    }
    return std::wstring(letzterCode.begin(), letzterCode.end());
}

bool istProzessAktiv(std::wstring prozessName) {
    bool gefunden = false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (prozessName == entry.szExeFile) {
                gefunden = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return gefunden;
}

void updateWebseite(const std::vector<ServerInfo>& liste) {
    std::ofstream jsonFile("C:\\Server\\nginx\\html\\status.json");
    if (jsonFile.is_open()) {
        jsonFile << "{";
        for (size_t i = 0; i < liste.size(); ++i) {
            bool aktiv = istProzessAktiv(liste[i].exeName);
            std::string name(liste[i].exeName.begin(), liste[i].exeName.end());
            jsonFile << "\"" << name << "\": \"" << (aktiv ? "Active" : "Offline") << "\"";
            if (i < liste.size() - 1) jsonFile << ", ";
        }
        jsonFile << "}";
        jsonFile.close();
    }
}

std::string wstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void sendeDiscordNachricht(std::wstring serverName, const std::string& webhookURL, bool istAbsturz, std::wstring zusatzInfo = L"") {
    std::string cleanWebhook = webhookURL;
    cleanWebhook.erase(cleanWebhook.find_last_not_of(" \n\r\t") + 1);
    if (cleanWebhook.empty() || cleanWebhook == "FEHLT") return;

    std::wstring wLogoURL = L"https://nebirrs-lab.de/Lab_Logo_apple.png";
    std::wstring wFarbe = istAbsturz ? L"15548997" : L"3066993";
    std::wstring wTitel = istAbsturz ? (L"Alarm: " + serverName) : (L"Start: " + serverName);

    std::wstring statusText = istAbsturz ? L"OFFLINE" : L"ONLINE";

    std::wstring wPayload = L"{"
        L"\\\"username\\\":\\\"Nebirrs Lab WatchGuard\\\","
        L"\\\"avatar_url\\\":\\\"" + wLogoURL + L"\\\","
        L"\\\"embeds\\\":[{"
        L"\\\"title\\\":\\\"" + wTitel + L"\\\","
        L"\\\"color\\\":" + wFarbe + L","
        L"\\\"fields\\\":["
        L"{\\\"name\\\":\\\"Status\\\",\\\"value\\\":\\\"" + statusText + L"\\\",\\\"inline\\\":true}";

    if (!zusatzInfo.empty()) {
        wPayload += L",{\\\"name\\\":\\\"Info\\\",\\\"value\\\":\\\"" + zusatzInfo + L"\\\",\\\"inline\\\":true}";
    }

    wPayload += L"],"
        L"\\\"thumbnail\\\":{\\\"url\\\":\\\"" + wLogoURL + L"\\\"}"
        L"}]}";

    std::string utf8Payload = wstringToUtf8(wPayload);

    std::string finalerBefehl = "curl -H \"Content-Type: application/json\" -X POST -d \"" + utf8Payload + "\" \"" + cleanWebhook + "\" > NUL 2>&1";

    system(finalerBefehl.c_str());
}

int main() {
    std::vector<ServerInfo> serverListe;
    std::wstring configName = L"config.txt";
    std::string webhookURL = "FEHLT";

    std::ifstream webhookFile("webhook.txt");
    if (webhookFile.is_open()) {
        std::getline(webhookFile, webhookURL);
        webhookFile.close();
    }

    std::wifstream configFile(configName);
    if (configFile.is_open()) {
        std::wstring tempExe, tempPfad, tempLog;

        while (std::getline(configFile, tempExe) &&
            std::getline(configFile, tempPfad) &&
            std::getline(configFile, tempLog)) {

            if (!tempExe.empty()) {
                serverListe.push_back({
                    tempExe,
                    tempPfad,
                    tempLog,
                    std::chrono::steady_clock::now() - std::chrono::minutes(11)
                    });
            }
        }
        configFile.close();
    }

    std::wcout << L"\n--- Initialisierungs-Check ---" << std::endl;

    for (auto& server : serverListe) {
        if (istProzessAktiv(server.exeName)) {
            std::wstring info = L"WatchGuard aktiv.";

            std::wstring code = holeJoinCode(server.logPfad);
            if (!code.empty()) {
                info = L"Online. Join-Code: " + code;
            }

            sendeDiscordNachricht(server.exeName, webhookURL, false, info);
        }
    }

    std::wcout << L"\nBeobachtung startet jetzt im Intervall..." << std::endl;
    Sleep(2000);

    while (true) {
        updateWebseite(serverListe);
        for (auto& server : serverListe) {
            if (!istProzessAktiv(server.exeName)) {
                sendeDiscordNachricht(server.exeName, webhookURL, true);

                auto startZeit = std::chrono::steady_clock::now();

                std::string restartCmd = "start \"\" \"" + wstringToUtf8(server.batPfad) + "\"";
                system(restartCmd.c_str());

                std::wcout << L"\n[RESTART] " << server.exeName << L" wird hochgefahren..." << std::endl;

                std::wstring code = L"";
                if (server.logPfad != L"NONE") {
                    int maxVersuche = 20; 
                    int aktuellerVersuch = 0;

                    while (code.empty() && aktuellerVersuch < maxVersuche) {
                        Sleep(20000); 
                        code = holeJoinCode(server.logPfad);

                        aktuellerVersuch++;
                        std::wcout << L"Warte auf Join-Code... (Versuch " << aktuellerVersuch << L")" << std::endl;
                    }
                }
                else {
                    Sleep(30000); 
                }

                auto endZeit = std::chrono::steady_clock::now();
                auto dauerSekunden = std::chrono::duration_cast<std::chrono::seconds>(endZeit - startZeit).count();

                std::wstring dauerInfo = L"Dauer: " + std::to_wstring(dauerSekunden) + L"s";
                std::wstring zusatzInfo = dauerInfo;

                if (!code.empty()) {
                    zusatzInfo = L"Join-Code: " + code + L" (" + dauerInfo + L")";
                }

                sendeDiscordNachricht(server.exeName, webhookURL, false, zusatzInfo);

                server.letzterAlarm = std::chrono::steady_clock::now();
            }
        }
        Sleep(10000);
    }
    return 0;
}