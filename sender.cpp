#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    //Turn on the Windows network system
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    //Create a virtual plug
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return 1;

    // Setup Destination
    sockaddr_in dest; 
    dest.sin_family = AF_INET;
    //Convert destination port number 8080 into network format
    dest.sin_port = htons(8080);
    // Convert the receiver IP address into network format
    inet_pton(AF_INET, "192.168.X.X", &dest.sin_addr); 

    int inputNum;

    //loop to keep asking for user input
    while (true) {
        std::cout << "Enter a number (1-4) (-1 to exit): ";
        // 6. Check if the user typed an actual number
        if (!(std::cin >> inputNum)) {
            std::cin.clear(); // Clear the error flag
            std::cin.ignore(1000, '\n'); // Discard the invalid text input
            continue;
        }
        // Check if the number 
        if (inputNum == -1) {
        break;
        }
        if (inputNum >= 1 && inputNum <= 4) {
            std::cout << "Sending: " << inputNum << std::endl;
            // Senf the number to destination address
            sendto(s, (char*)&inputNum, sizeof(inputNum), 0, (sockaddr*)&dest, sizeof(dest));
        } else {
            std::cout << "Invalid! Only 1, 2, 3, or 4 allowed." << std::endl;
        }
    }

    //Turn off and delete the virtual plug
    closesocket(s);
    //Shut down the Windows network system
    WSACleanup();
    return 0;
}
