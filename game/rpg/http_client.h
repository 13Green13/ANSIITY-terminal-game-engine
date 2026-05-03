#pragma once

#include <string>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// Minimal synchronous HTTP GET client using WinHTTP
class HttpClient {
    std::wstring _host;
    int _port;

public:
    HttpClient(const std::string& host, int port)
        : _port(port) {
        _host = std::wstring(host.begin(), host.end());
    }

    // Returns response body as string, or empty on failure
    std::string get(const std::string& path) {
        std::wstring wpath(path.begin(), path.end());

        HINTERNET hSession = WinHttpOpen(L"ANSIITY/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return "";

        // Set timeouts: resolve 2s, connect 2s, send 2s, receive 3s
        WinHttpSetTimeouts(hSession, 2000, 2000, 2000, 3000);

        HINTERNET hConnect = WinHttpConnect(hSession, _host.c_str(),
            (INTERNET_PORT)_port, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return "";
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET",
            wpath.c_str(), NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        BOOL result = WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

        if (!result || !WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        std::string body;
        DWORD bytesAvailable = 0;
        DWORD bytesRead = 0;
        char buffer[4096];

        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            DWORD toRead = (bytesAvailable < sizeof(buffer)) ? bytesAvailable : sizeof(buffer);
            if (WinHttpReadData(hRequest, buffer, toRead, &bytesRead)) {
                body.append(buffer, bytesRead);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return body;
    }
};
