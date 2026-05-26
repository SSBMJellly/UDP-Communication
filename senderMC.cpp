#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    //Turn on Windows network system
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    //Create a virtual plug
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return 1;

    //===START TIMEOUT CONFIGURATION===
    // Set timeout duration to 2 seconds
    DWORD timeoutVal = 2000;

    // Apply the receive timeout option to virtual plug
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutVal, sizeof(timeoutVal)) == SOCKET_ERROR) {
        std::cout << "Failed to set socket timeout option!" << std::endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }
    //===END TIMEOUT CONFIGURATION===

    // Setup Destination
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8080);

    inet_pton(AF_INET, "192.168.178.36", (void*)&dest.sin_addr.s_addr);

    int inputNum;

    //loop to keep asking for input
    while (true) {
        std::cout << "Enter a number (1-4) (-1 to exit): ";
        // Check input is an actual number
        if (!(std::cin >> inputNum)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }
        if (inputNum == -1) {
            break;
        }
        if (inputNum >= 1 && inputNum <= 4) {
            std::cout << "Sending: " << inputNum << std::endl;
            // Send number to destination address
            sendto(s, (char*)&inputNum, sizeof(inputNum), 0, (sockaddr*)&dest, sizeof(dest));

            //===START CONFIRMATION RECEIVER===
            char buffer[50] = { 0 }; // Buffer to hold text reply
            sockaddr_in from;
            int fromLen = sizeof(from);

            std::cout << "Waiting for confirmation..." << std::endl;

            int bytes = recvfrom(s, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&from, (int*)&fromLen);

            if (bytes > 0) {
                buffer[bytes] = '\0'; // Ensure string ends properly
                std::cout << "PC Received Confirmation: " << buffer << std::endl;
            }
            else if (bytes == SOCKET_ERROR) {
                int errorCode = WSAGetLastError();
                // WSAETIMEDOUT (10060) occurs when the 2 seconds expire without data
                if (errorCode == WSAETIMEDOUT) {
                    std::cout << "Error: Confirmation timed out! No response from ESP32." << std::endl;
                }
                else {
                    std::cout << "Receive failed! Error code: " << errorCode << std::endl;
                }
            }
            //===END CONFIRMATION RECEIVER===

        }
        else {
            std::cout << "Invalid! Only 1, 2, 3, or 4 allowed." << std::endl;
        }
    }

    //Turn off and delete virtual plug
    closesocket(s);
    //Shut down Windows network
    WSACleanup();
    return 0;
}
