#include <EEPROM.h>
#include <ESP8266WiFi.h>

#define EEPROM_SIZE 64

const char* ssid = "SSID_OF_ORGANISATIONAL_NETWORK";
const char* password = "PASSWORD";

WiFiServer server(80);

const String API_KEY = "YOUR_API_KEY";

// Static IP configuration
IPAddress local_IP(YOUR_API_KEY);
IPAddress gateway(GATEWAY);
IPAddress subnet(SUBNET_MASK);
IPAddress primaryDNS(PRIMARY_DNS);
IPAddress secondaryDNS(SECONDARY_DNS);

// Room-to-GPIO mapping
struct Room {
  String name;
  int gpio;
};
Room roomMappings[] = {
  {"VIB109", 5},
  {"VIB110", 4},
  {"VIB111", 14},  // Add more rooms here
};

void saveAPIkey(String apiKey) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < apiKey.length(); i++) {
    EEPROM.write(i, apiKey[i]);
  }
  EEPROM.write(apiKey.length(), '\0');
  EEPROM.commit();
  Serial.println("API key saved.");
}

String readAPIkey() {
  EEPROM.begin(EEPROM_SIZE);
  String apiKey = "";
  char c;
  for (int i = 0; i < EEPROM_SIZE; i++) {
    c = EEPROM.read(i);
    if (c == '\0') break;
    apiKey += c;
  }
  return apiKey;
}

int getGPIOfromRoom(String roomName) {
  for (Room room : roomMappings) {
    if (room.name == roomName) {
      return room.gpio;
    }
  }
  return -1;  // Return -1 if room is not found
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

//   Configure static IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure static IP");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  WiFi.setSleep(false);  // Disable Wi-Fi sleep


  for (Room room : roomMappings) {
    pinMode(room.gpio, OUTPUT);
    digitalWrite(room.gpio, HIGH);
  }

  server.begin();
  Serial.println("Server started at IP: " + WiFi.localIP().toString());

  saveAPIkey(API_KEY);
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println(WiFi.localIP());
    Serial.println("New Client connected.");
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') {
          break;
        }
      }
    }

    Serial.println("Request received: " + request);

    // Parse HTTP GET request
    if (request.indexOf("GET /relay") != -1) {
      String apiKey = "";
      String room = "";
      String state = "";

      // Extract parameters
      int apiKeyIndex = request.indexOf("apikey=");
      int roomIndex = request.indexOf("room=");
      int stateIndex = request.indexOf("state=");

      if (apiKeyIndex != -1) {
        apiKey = request.substring(apiKeyIndex + 7, request.indexOf('&', apiKeyIndex));
      }
      if (roomIndex != -1) {
        room = request.substring(roomIndex + 5, request.indexOf('&', roomIndex));
      }
      if (stateIndex != -1) {
        state = request.substring(stateIndex + 6, request.indexOf(' ', stateIndex));
      }

      Serial.println("API Key: " + apiKey);
      Serial.println("Room: " + room);
      Serial.println("State: " + state);

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();

      if (apiKey != readAPIkey()) {
        client.println("403 Forbidden: Invalid API key");
      } else if (room == "" || state == "") {
        client.println("400 Bad Request: Missing 'room' or 'state' parameter");
      } else {
        int gpioPin = getGPIOfromRoom(room);
        if (gpioPin == -1) {
          client.println("400 Bad Request: Invalid room number! Check the available rooms.");
        } else {
          if (state == "ON") {
            digitalWrite(gpioPin, LOW);
            client.println("200 OK: Electricity supply turned ON for room " + room);
          } else if (state == "OFF") {
            digitalWrite(gpioPin, HIGH);
            client.println("200 OK: Electricity supply turned OFF for room " + room);
          } else {
            client.println("400 Bad Request: Invalid state! Use ON or OFF only.");
          }
        }
      }
    } else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("404 Not Found: Invalid endpoint.");
    }

    delay(10);
    client.stop();
    Serial.println("Client disconnected.");
  }
}
