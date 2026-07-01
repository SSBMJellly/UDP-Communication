#include <WiFi.h>

// Network Configuration 
const char* ssid = "WIFI NAME";
const char* password = "WIFI PASSWORD";
const int port = 8080;

WiFiServer server(port);

// Match C++ Structures 
enum CommandType { COORDINATES = 1, GRIPPER = 2 };

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

// Artificial Bounds & State Variables 
float currentX = 0.0f, currentY = 0.0f, currentZ = 0.0f;
const float LIMIT_MIN = -5.0f;
const float LIMIT_MAX = 5.0f;
int currentGripperState = -1;  // -1 = Uninitialized, 0 = Closed, 1 = Open

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    server.begin();
}

void loop() {
    WiFiClient client = server.available();

    if (client) {
        Serial.println("Client Connected.");
        bool wasMovingLastTick = false;
        unsigned long lastPacketTime = millis();

        while (client.connected()) {
            // Process incoming commands
            if (client.available() >= sizeof(CommandPacket)) {
                CommandPacket packet;
                client.read((uint8_t*)&packet, sizeof(CommandPacket));
                lastPacketTime = millis(); // Refresh timeout clock

                ResponsePacket response = {0}; 

                if (packet.type == COORDINATES) {
                    processMovement(packet, response);
                    wasMovingLastTick = true;
                } 
                else if (packet.type == GRIPPER) {
                    processGripper(packet, response);
                }

                // Send response back instantly
                client.write((uint8_t*)&response, sizeof(ResponsePacket));
            }

            // Continuous key monitoring
            // If no new packets while moving
            if (wasMovingLastTick && (millis() - lastPacketTime > 90)) {
                ResponsePacket idleResponse = {0};
                strcpy(idleResponse.statusMessage, "Status: Idle");
                idleResponse.limitReached = 0;

                client.write((uint8_t*)&idleResponse, sizeof(ResponsePacket));
                wasMovingLastTick = false; // Prevent spamming idle message
            }
            
            delay(5); // loop padding
        }
        Serial.println("Client Disconnected.");
    }
}

// Logic Processors

void processMovement(const CommandPacket& packet, ResponsePacket& res) {
    res.limitReached = 0;

    // Evaluate X Axis
    if (packet.x != 0.0f) {
        float nextX = currentX + packet.x;
        if (nextX < LIMIT_MIN) {
            strcpy(res.statusMessage, "Limit reached: -X limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else if (nextX > LIMIT_MAX) {
            strcpy(res.statusMessage, "Limit reached: +X limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else {
            currentX = nextX;
            strcpy(res.statusMessage, packet.x > 0 ? "Moving in +X direction" : "Moving in -X direction");
            // MOVEMENT HERE
        }
    }
    // Evaluate Y Axis
    else if (packet.y != 0.0f) {
        float nextY = currentY + packet.y;
        if (nextY < LIMIT_MIN) {
            strcpy(res.statusMessage, "Limit reached: -Y limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else if (nextY > LIMIT_MAX) {
            strcpy(res.statusMessage, "Limit reached: +Y limit reached!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else {
            currentY = nextY;
            strcpy(res.statusMessage, packet.y > 0 ? "Moving in +Y direction" : "Moving in -Y direction");
            // MOVEMENT HERE
        }
    }
    // Evaluate Z Axis
    else if (packet.z != 0.0f) {
        float nextZ = currentZ + packet.z;
        if (nextZ < LIMIT_MIN) {
            strcpy(res.statusMessage, "Limit reached: -Z limit reached (Descend)!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else if (nextZ > LIMIT_MAX) {
            strcpy(res.statusMessage, "Limit reached: +Z limit reached (Ascend)!");
            res.limitReached = 1;
            // LIMIT REACHED
        } else {
            currentZ = nextZ;
            strcpy(res.statusMessage, packet.z > 0 ? "Moving in +Z direction (Ascend)" : "Moving in -Z direction (Descend)");
            // MOVEMENT HERE
        }
    }
}

void processGripper(const CommandPacket& packet, ResponsePacket& res) {
    res.limitReached = 0;

    if (packet.gripperAction == 1) { // Requesting Open
        if (currentGripperState == 1) {
            strcpy(res.statusMessage, "Gripper status: Gripper already open!");
        } else {
            currentGripperState = 1;
            strcpy(res.statusMessage, "Gripper action: Opening gripper...");
            // OPEN GRIPPER
        }
    } 
    else if (packet.gripperAction == 0) { // Requesting Close
        if (currentGripperState == 0) {
            strcpy(res.statusMessage, "Gripper status: Gripper already closed!");
        } else {
            currentGripperState = 0;
            strcpy(res.statusMessage, "Gripper action: Closing gripper...");
            // CLOSE GRIPPER
        }
    }
}
