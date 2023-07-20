#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>

char server[] = "red-old.lambda8.at";

// Set the static IP address to use if the DHCP fails to assign
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

EthernetClient client;

void httpRequest(char* host, char* path);

uint16_t chanels[] = {106, 113, 120, 127, 134, 141};
uint8_t color[] = {255, 117, 17};

void setup() {
  Serial.begin(9600);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to configure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
}

uint32_t requestTimer = 0;
void loop() {
  if (millis() - requestTimer > 10000) {
    requestTimer = millis();
    Serial.print("connecting to ");
    Serial.print(server);
    Serial.println("...");
    // if you get a connection, report back via serial:
    if (client.connect(server, 80)) {
      Serial.print("connected to ");
      Serial.println(client.remoteIP());
      httpRequest(server, "/gw/kiosk/");
    } else {
      // if you didn't get a connection to the server:
      Serial.println("connection failed");
    }
  }

  if (client.available()) {
    String line = client.readStringUntil('\r');
    if (line.length() < 2) {
      uint8_t data = client.readString().toInt();
      Serial.println(data);

      for (auto&& chan : chanels) {
        Serial.print(chan);
        Serial.print(": ");
        Serial.println(map(data, 0, 255, 0, color[0]));
        Serial.print(chan + 1);
        Serial.print(": ");
        Serial.println(map(data, 0, 255, 0, color[1]));
        Serial.print(chan + 2);
        Serial.print(": ");
        Serial.println(map(data, 0, 255, 0, color[2]));
        Serial.println("");
      }
    };
  }

  delay(1);
}

void httpRequest(char* host, char* path) {
  client.print("GET ");
  client.print(path);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
}