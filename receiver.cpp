#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
 WSADATA wsa;
 //Turn on Windows network system
 if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

 //virtual plug
 SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

 sockaddr_in server;
 server.sin_family = AF_INET;
 //Listen to all available connections
 server.sin_addr.s_addr = INADDR_ANY; 
 //Convert port number 8080 into network format
 server.sin_port = htons(8080);

 //Register socket to lock onto port 8080
 if (bind(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
    std::cout << "Bind failed!" << std::endl;
    return 1;
 }

 std::cout << "Reveiver Read. Waiting for data..." << std::endl;

 int receivedNum = 0;
 sockaddr_in from;
 
 //loop to look for incoming data
 while (true) {
   int fromLen = sizeof(from);
   // wait here until data arrives
    int bytes = recvfrom(s, (char*)&receivedNum, sizeof(receivedNum), 0,(sockaddr*)&from, &fromLen);
    if(bytes > 0) {
        std::cout << "Received Integer: " << receivedNum << std::endl;
    }
 }

 //Turn off and delete the virtual plug
 closesocket(s);
 //Shut down the Windows network system
 WSACleanup();
 return 0;
}
