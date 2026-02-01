#include "Leaderboard.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <string>

#pragma comment(lib, "winhttp.lib")

// URL設定
const std::wstring Leaderboard::SERVER_HOST = L"script.google.com";
// ★あなたのIDが入ったURLのままにしてください
const std::wstring Leaderboard::SERVER_PATH = L"/macros/s/AKfycbwuCi3A2bVHpLrbTOMAT0Lzmlx9nAH5SMdir-9lqmFTCtjxKjL2EMJjaadJKqJ0kIjL4A/exec";

void Leaderboard::SendScore(const std::string& name, float time, int kills) {
	std::thread([=]() {
		HINTERNET hSession = WinHttpOpen(L"GameClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (hSession) {
			HINTERNET hConnect = WinHttpConnect(hSession, SERVER_HOST.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
			if (hConnect) {
				HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", SERVER_PATH.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
				if (hRequest) {
					std::string json = "{\"name\":\"" + name + "\", \"time\":" + std::to_string(time) + ", \"kills\":" + std::to_string(kills) + "}";

					DWORD dwOptionValue = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
					WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &dwOptionValue, sizeof(dwOptionValue));

					DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
					WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

					std::wstring headers = L"Content-Type: application/json\r\n";

					if (WinHttpSendRequest(hRequest, headers.c_str(), headers.length(), (LPVOID)json.c_str(), (DWORD)json.length(), (DWORD)json.length(), 0)) {
						WinHttpReceiveResponse(hRequest, NULL);
						// 送信成功（何も表示しない）
					}
					WinHttpCloseHandle(hRequest);
				}
				WinHttpCloseHandle(hConnect);
			}
			WinHttpCloseHandle(hSession);
		}
		}).detach();
}

void Leaderboard::FetchRanking(std::function<void(const std::vector<RankingData>&)> onComplete) {
	std::thread([=]() {
		std::vector<RankingData> results;
		HINTERNET hSession = WinHttpOpen(L"GameClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (hSession) {
			HINTERNET hConnect = WinHttpConnect(hSession, SERVER_HOST.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
			if (hConnect) {
				HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", SERVER_PATH.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
				if (hRequest) {
					DWORD dwOptionValue = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
					WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &dwOptionValue, sizeof(dwOptionValue));

					DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
					WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

					if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
						if (WinHttpReceiveResponse(hRequest, NULL)) {
							DWORD dwSize = 0;
							DWORD dwDownloaded = 0;
							std::string response;
							do {
								dwSize = 0;
								WinHttpQueryDataAvailable(hRequest, &dwSize);
								if (dwSize > 0) {
									char* pszOutBuffer = new char[dwSize + 1];
									if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
										pszOutBuffer[dwDownloaded] = 0;
										response += pszOutBuffer;
									}
									delete[] pszOutBuffer;
								}
							} while (dwSize > 0);

							// CSVパース
							std::stringstream ss(response);
							std::string line;
							while (std::getline(ss, line)) {
								std::stringstream ls(line);
								std::string segment;
								std::vector<std::string> seglist;
								while (std::getline(ls, segment, ',')) {
									seglist.push_back(segment);
								}
								if (seglist.size() >= 3) {
									try {
										RankingData d;
										d.name = seglist[0];
										d.time = std::stof(seglist[1]);
										d.kills = std::stoi(seglist[2]);
										results.push_back(d);
									}
									catch (...) {}
								}
							}
						}
					}
					WinHttpCloseHandle(hRequest);
				}
				WinHttpCloseHandle(hConnect);
			}
			WinHttpCloseHandle(hSession);
		}
		if (onComplete) onComplete(results);
		}).detach();
}