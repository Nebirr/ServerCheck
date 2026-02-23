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
    std::wstring displayName; 
    std::wstring exeName;     
    std::wstring batPfad;
    std::wstring logPfad;
    std::wstring logSuche;
    std::chrono::steady_clock::time_point letzterAlarm;
};


std::string wstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring extrahiereLogWert(std::wstring logPfad, std::wstring sucheW) {
    if (logPfad == L"NONE" || sucheW == L"NONE" || logPfad.empty()) return L"";

    std::string suche = wstringToUtf8(sucheW);

    std::ifstream file(logPfad);
    std::string line;
    std::string gefundenesResultat = "";

    if (file.is_open()) {
        while (std::getline(file, line)) {
            size_t pos = line.find(suche);
            if (pos != std::string::npos) {
                gefundenesResultat = line.substr(pos + suche.length());

                gefundenesResultat.erase(gefundenesResultat.find_last_not_of(" \n\r\t") + 1);
            }
        }
        file.close();
    }
    return std::wstring(gefundenesResultat.begin(), gefundenesResultat.end());
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
        std::wstring tempDisplayName, tempExe, tempPfad, tempLog, tempSuche;

        while (std::getline(configFile, tempDisplayName) &&
            std::getline(configFile, tempExe) &&
            std::getline(configFile, tempPfad) &&
            std::getline(configFile, tempLog) &&
            std::getline(configFile, tempSuche)) {

            if (!tempExe.empty()) {
                serverListe.push_back({
                    tempDisplayName,
                    tempExe,
                    tempPfad,
                    tempLog,
                    tempSuche,
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

            std::wstring code = extrahiereLogWert(server.logPfad, server.logSuche);
            if (!code.empty()) {
                info = L"Online. " + server.logSuche + L": " + code;
            }

            sendeDiscordNachricht(server.displayName, webhookURL, false, info);
        }
    }

    std::wcout << L"\nBeobachtung startet jetzt im Intervall..." << std::endl;
    Sleep(2000);

    while (true) {

        updateWebseite(serverListe);

        for (auto& server : serverListe) {

            if (!istProzessAktiv(server.exeName)) {

                std::wcout << L"[CHECK] " << server.displayName << L" scheint offline. Prüfe erneut..." << std::endl;
                Sleep(3000);

                if (istProzessAktiv(server.exeName)) {
                    std::wcout << L"[INFO] Fehlalarm abgefangen. " << server.displayName << L" ist stabil." << std::endl;
                    continue;
                }


                sendeDiscordNachricht(server.displayName, webhookURL, true);


                auto startZeit = std::chrono::steady_clock::now();
                std::string restartCmd = "start \"\" \"" + wstringToUtf8(server.batPfad) + "\"";
                system(restartCmd.c_str());

                std::wcout << L"\n[RESTART] " << server.displayName << L" wird neu gestartet..." << std::endl;


                std::wstring code = L"";
                if (server.logPfad != L"NONE") {
                    int maxVersuche = 15;
                    int aktuellerVersuch = 0;

                    while (code.empty() && aktuellerVersuch < maxVersuche) {
                        Sleep(15000);


                        std::wstring temp = extrahiereLogWert(server.logPfad, server.logSuche);


                        if (!temp.empty() && temp.length() >= 5) {
                            code = temp;
                        }

                        aktuellerVersuch++;
                        std::wcout << L"Scan für " << server.displayName << L" #" << aktuellerVersuch << L"..." << std::endl;
                    }


                    if (code.empty()) {
                        std::wcout << L"[INFO] Bisher kein Code gefunden. Starte finale 20s Wartezeit für " << server.displayName << L"..." << std::endl;
                        Sleep(20000);

                        code = extrahiereLogWert(server.logPfad, server.logSuche);

                        if (!code.empty()) {
                            std::wcout << L"[SUCCESS] Code im letzten Anlauf gefunden!" << std::endl;
                        }
                    }
                }


                auto endZeit = std::chrono::steady_clock::now();
                auto dauerSekunden = std::chrono::duration_cast<std::chrono::seconds>(endZeit - startZeit).count();

                std::wstring zusatzInfo;
                if (!code.empty()) {

                    zusatzInfo = server.logSuche + L": " + code + L" (Dauer: " + std::to_wstring(dauerSekunden) + L"s)";
                }
                else {

                    zusatzInfo = L"Server online (Kein neuer " + server.logSuche + L" gefunden) (Dauer: " + std::to_wstring(dauerSekunden) + L"s)";
                }


                sendeDiscordNachricht(server.displayName, webhookURL, false, zusatzInfo);


                server.letzterAlarm = std::chrono::steady_clock::now();
            }
        }

        Sleep(10000);

    }

    return 0;
}