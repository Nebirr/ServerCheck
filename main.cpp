#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <sstream>
#include <chrono> 
#include <ctime>

struct ServerInfo{
	std::wstring exeName;
	std::wstring batPfad;
	std::chrono::steady_clock::time_point letzterAlarm;
};

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

void sendeDiscordNachricht(std::wstring serverName, const std::string& webhookURL) {
	if (webhookURL.empty() || webhookURL == "FEHLT") return; 

	std::wstring wNachricht = L"ALARM: Der Server [" + serverName + L"] ist abgestuerzt! oder wird wieder Hochgefahren.";
	std::wstring wBefehl = L"curl -H \"Content-Type: application/json\" -X POST -d \"{\\\"content\\\": \\\""
		+ wNachricht +
		L"\\\"}\" " + std::wstring(webhookURL.begin(), webhookURL.end()) + L" > NUL 2>&1";
	std::string finalerBefehl(wBefehl.begin(), wBefehl.end());
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
		std::cout << "Webhook geladen." << std::endl;
	}
	else {
		std::cout << "WARNUNG: webhook.txt nicht gefunden! Discord-Alarme deaktiviert." << std::endl;
	}

	std::wifstream configFile(configName);
	if (configFile.is_open()) {
		std::wcout << L"Lade Einstellungen aus config.txt..." << std::endl;
		std::wstring tempExe, tempPfad;

		while (std::getline(configFile, tempExe) && std::getline(configFile, tempPfad)) {
			if (!tempExe.empty() && !tempPfad.empty()) {
				serverListe.push_back({ tempExe, tempPfad, std::chrono::steady_clock::now() - std::chrono::minutes(11) });
			}
		}
		configFile.close();
	}

	if (serverListe.empty()) {
		std::wstring exeName, batPfad;
		std::wcout << L"Keine Konfiguration gefunden. Bitte ersten Server eingeben: " << std::endl;
		std::wcout << L"1. Name der .exe (z.B. Valheim.exe): ";
		std::getline(std::wcin, exeName);
		std::wcout << L"2. Pfad zur Start-Datei: ";
		std::getline(std::wcin, batPfad);

		serverListe.push_back({ exeName, batPfad, std::chrono::steady_clock::now() - std::chrono::minutes(11) });

		std::wofstream saveFile(configName);
		if (saveFile.is_open()) {
			saveFile << exeName << std::endl;
			saveFile << batPfad << std::endl;
			saveFile.close();
		}
	}

	std::wcout << L"\n Beobachtung von " << serverListe.size() << L"Server(n) startet..." << std::endl;
	Sleep(3000);

	while (true) {
		system("cls");
		std::wcout << L"--- Multi-Server Monitor ---" << std::endl;
		
		updateWebseite(serverListe);

		for (auto& server : serverListe) {
			if (istProzessAktiv(server.exeName)) {
				std::wcout << L"[ONLINE] " << server.exeName << std::endl;
			} else {
				std::wcout << L"[ALARM] " << server.exeName << L" fehlt! Starte neu..." << std::endl;

				auto jetzt = std::chrono::steady_clock::now();

				auto vergangen = std::chrono::duration_cast<std::chrono::minutes>(jetzt - server.letzterAlarm).count();

				if (vergangen >= 10) {
					sendeDiscordNachricht(server.exeName, webhookURL); 
					server.letzterAlarm = jetzt;
				}

				std::wstring wBefehl = L"start \"\" \"" + server.batPfad + L"\"";
				std::string finalerBefehl(wBefehl.begin(), wBefehl.end());
				system(finalerBefehl.c_str());
				std::wcout << L"Neustart eingeleitet fuer: " << server.exeName << std::endl;

				Sleep(30000);
			}
		}

		Sleep(60000);
	}
	return 0;
}