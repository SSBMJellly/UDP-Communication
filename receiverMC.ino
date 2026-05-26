#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi network credentials
const char* ssid     = "FRITZ!Box 7362 SL";
const char* password = "johnsonsabine69";

const unsigned int port = 8080;
WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n--- ESP32-S3 UDP Receiver Starting ---");

  //Power on the internal Wi-Fi chip
  #if defined(ARDUINO_USB_CDC_ON_BOOT)
    delay(1000); 
  #endif

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.disconnect(true); // Clear old corrupted network states
  delay(1000);
  
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);

  // Connection loop with a 15-second timeout counter
  int timeout_counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout_counter++;
    
    if(timeout_counter > 30) { // 30 * 500ms = 15 seconds
      Serial.println("\n[ERROR] Connection failed. Restarting ESP32...");
      ESP.restart(); // Forces the board to try booting fresh
    }
  }

  Serial.println("\nWiFi connected successfully!");
  Serial.print("Receiver IP address: ");
  Serial.println(WiFi.localIP()); 

  udp.begin(port);
  Serial.print("Receiver Ready. Listening on UDP Port: ");
  Serial.println(port);
}

void loop() {
  int receivedNum = 0;
  int packetSize = udp.parsePacket();
  
  if (packetSize > 0) {
    int bytes = udp.read((char*)&receivedNum, sizeof(receivedNum));
    
    if (bytes > 0) {
        Serial.print("Received Integer: ");
        Serial.println(receivedNum);

        if (receivedNum >= 1 && receivedNum <= 4) {
             String replyMessage = "number " + String(receivedNum) + " received";
             
             udp.beginPacket(udp.remoteIP(), udp.remotePort());
             udp.write((const uint8_t*)replyMessage.c_str(), replyMessage.length());
             udp.endPacket();
             
             Serial.print("Sent confirmation back to: ");
             Serial.println(udp.remoteIP());
        }
    }
  }
  delay(10); 
}
