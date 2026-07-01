#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <windows.h> 

#pragma comment(lib, "ws2_32.lib")

enum CommandType { COORDINATES = 1, GRIPPER = 2 };

struct CommandPacket {
    int type;
    float x;
    float y;
    float z;
    int gripperAction;
};

// Matches ESP32 Response Structure
struct ResponsePacket {
    char statusMessage[64];
    int limitReached;
};

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.178.36", (void*)&dest.sin_addr.s_addr);

    if (connect(s, (sockaddr*)&dest, sizeof(dest)) == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // Set socket to NON-BLOCKING to stop recv() from freezing loop when no keys are pressed
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);

    std::cout << "Connected! Use Arrows (Move), A/D (Ascend/Descend), O/C (Gripper), ESC (Exit).\n" << std::endl;

    bool oPressed = false;
    bool cPressed = false;

    while (true) {
        CommandPacket packet = { 0 };
        bool shouldSend = false;

        // Movement
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) { packet.x -= 1.0f; shouldSend = true; }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { packet.x += 1.0f; shouldSend = true; }
        if (GetAsyncKeyState(VK_UP) & 0x8000) { packet.y += 1.0f; shouldSend = true; }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) { packet.y -= 1.0f; shouldSend = true; }
        if (GetAsyncKeyState('A') & 0x8000) { packet.z += 1.0f; shouldSend = true; }
        if (GetAsyncKeyState('D') & 0x8000) { packet.z -= 1.0f; shouldSend = true; }

        if (shouldSend) packet.type = COORDINATES;

        // Gripper
        if (GetAsyncKeyState('O') & 0x8000) {
            if (!oPressed) {
                packet.type = GRIPPER;
                packet.gripperAction = 1;
                shouldSend = true;
                oPressed = true;
            }
        }
        else { oPressed = false; }

        if (GetAsyncKeyState('C') & 0x8000) {
            if (!cPressed) {
                packet.type = GRIPPER;
                packet.gripperAction = 0;
                shouldSend = true;
                cPressed = true;
            }
        }
        else { cPressed = false; }

        // Send out data if required
        if (shouldSend) {
            send(s, (char*)&packet, sizeof(packet), 0);
        }

        //Read incoming feedback from ESP32 asynchronously
        ResponsePacket response = { 0 };
        int bytesReceived = recv(s, (char*)&response, sizeof(ResponsePacket), 0);

        if (bytesReceived == sizeof(ResponsePacket)) {
            // Carriage return (\r) clears the current line for clean streaming output
            std::cout << "\r[ESP32 Response]: " << response.statusMessage << "                               \r" << std::flush;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        Sleep(50);
    }

    closesocket(s);
    WSACleanup();
    return 0;
}