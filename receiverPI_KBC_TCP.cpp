#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <chrono>

// Match C++ Structures 
enum CommandType { COORDINATES = 1, GRIPPER = 2 };

#pragma pack(push, 1) // Ensures cross-platform memory alignment
struct CommandPacket {
    int type;
    float x;
    float y;
    float z;
    int gripperAction;
};

struct ResponsePacket {
    char statusMessage[64];
    int limitReached; 
};
#pragma pack(pop)

// Artificial Bounds & State Variables 
float currentX = 0.0f, currentY = 0.0f, currentZ = 0.0f;
const float LIMIT_MIN = -5.0f;
const float LIMIT_MAX = 5.0f;
int currentGripperState = -1;  // -1 = Uninitialized, 0 = Closed, 1 = Open

// Forward Declarations
void processMovement(const CommandPacket& packet, ResponsePacket& res);
void processGripper(const CommandPacket& packet, ResponsePacket& res);

int main() {
    const int port = 8080;
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "setsockopt failed\n";
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed\n";
        return -1;
    }

    std::cout << "Server listening on port " << port << "..." << std::endl;

    while (true) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            std::cerr << "Accept connection failed\n";
            continue;
        }

        std::cout << "Client Connected.\n";
        bool wasMovingLastTick = false;
        auto lastPacketTime = std::chrono::steady_clock::now();

        while (true) {
            CommandPacket packet;
            ssize_t bytesRead = recv(client_fd, &packet, sizeof(CommandPacket), MSG_DONTWAIT);

            if (bytesRead == sizeof(CommandPacket)) {
                lastPacketTime = std::chrono::steady_clock::now(); 
                ResponsePacket response = {0};

                if (packet.type == COORDINATES) {
                    processMovement(packet, response);
                    wasMovingLastTick = true;
                } 
                else if (packet.type == GRIPPER) {
                    processGripper(packet, response);
                }

                send(client_fd, &response, sizeof(ResponsePacket), 0);
            } 
            else if (bytesRead == 0) {
                break;
            }

            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastPacketTime).count();

            if (wasMovingLastTick && (elapsedTime > 90)) {
                ResponsePacket idleResponse = {0};
                std::strcpy(idleResponse.statusMessage, "Status: Idle");
                idleResponse.limitReached = 0;

                send(client_fd, &idleResponse, sizeof(ResponsePacket), 0);
                wasMovingLastTick = false; 
            }

            usleep(5000); 
        }

        close(client_fd);
        std::cout << "Client Disconnected.\n";
    }

    close(server_fd);
    return 0;
}

// Logic Processors

void processMovement(const CommandPacket& packet, ResponsePacket& res) {
    res.limitReached = 0;

    // Evaluate X Axis
    if (packet.x != 0.0f) {
        float nextX = currentX + packet.x;
        if (nextX < LIMIT_MIN) {
            std::strcpy(res.statusMessage, "Limit reached: -X limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else if (nextX > LIMIT_MAX) {
            std::strcpy(res.statusMessage, "Limit reached: +X limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else {
            currentX = nextX;
            std::strcpy(res.statusMessage, packet.x > 0 ? "Moving in +X direction" : "Moving in -X direction");
            // MOVEMENT HERE
        }
    }
    // Evaluate Y Axis
    else if (packet.y != 0.0f) {
        float nextY = currentY + packet.y;
        if (nextY < LIMIT_MIN) {
            std::strcpy(res.statusMessage, "Limit reached: -Y limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else if (nextY > LIMIT_MAX) {
            std::strcpy(res.statusMessage, "Limit reached: +Y limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else {
            currentY = nextY;
            std::strcpy(res.statusMessage, packet.y > 0 ? "Moving in +Y direction" : "Moving in -Y direction");
            // MOVEMENT HERE
        }
    }
    // Evaluate Z Axis
    else if (packet.z != 0.0f) {
        float nextZ = currentZ + packet.z;
        if (nextZ < LIMIT_MIN) {
            std::strcpy(res.statusMessage, "Limit reached: -Z limit reached (Descend)!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else if (nextZ > LIMIT_MAX) {
            std::strcpy(res.statusMessage, "Limit reached: +Z limit reached (Ascend)!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else {
            currentZ = nextZ;
            std::strcpy(res.statusMessage, packet.z > 0 ? "Moving in +Z direction (Ascend)" : "Moving in -Z direction (Descend)");
            // MOVEMENT HERE
        }
    }
}

void processGripper(const CommandPacket& packet, ResponsePacket& res) {
    res.limitReached = 0;

    if (packet.gripperAction == 1) { // Requesting Open
        if (currentGripperState == 1) {
            std::strcpy(res.statusMessage, "Gripper status: Gripper already open!");
        } else {
            currentGripperState = 1;
            std::strcpy(res.statusMessage, "Gripper action: Opening gripper...");
            // OPEN GRIPPER
        }
    } 
    else if (packet.gripperAction == 0) { // Requesting Close
        if (currentGripperState == 0) {
            std::strcpy(res.statusMessage, "Gripper status: Gripper already closed!");
        } else {
            currentGripperState = 0;
            std::strcpy(res.statusMessage, "Gripper action: Closing gripper...");
            // CLOSE GRIPPER
        }
    }
}