#include <Arduino.h>
#include <DMXSerial.h>
#include <Ethernet.h>
#include <SPI.h>

char host[] = "red-old.lambda8.at";
char path[] = "/gw/kiosk/";

// Set the static IP address to use if the DHCP fails to assign
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

EthernetClient client;

void httpRequest(char* host, char* path);

uint16_t chanels[] = {106, 113, 120, 127, 134, 141};
uint8_t color[] = {255, 117, 17};

void setup() {
  DMXSerial.init(DMXController);

  if (Ethernet.begin(mac) == 0) {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    Ethernet.begin(mac, ip, myDns);
  }
  delay(1000);
}

uint32_t requestTimer = 0;
void loop() {
  if (millis() - requestTimer > 10000) {
    requestTimer = millis();
    if (client.connect(host, 80)) {
      httpRequest(host, path);
    }
  }

  if (client.available()) {
    String line = client.readStringUntil('\r');
    if (line.length() < 2) {
      uint8_t data = client.readString().toInt();

      for (auto&& chan : chanels) {
        DMXSerial.write(chan + 0, map(data, 0, 255, 0, color[0]));  // R
        DMXSerial.write(chan + 1, map(data, 0, 255, 0, color[1]));  // G
        DMXSerial.write(chan + 2, map(data, 0, 255, 0, color[2]));  // B
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