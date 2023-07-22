#include <Arduino.h>
#include <DMXSerial.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <SPI.h>

char host[] = "red-old.lambda8.at";

char pathKiosk[] = "/gw/kiosk/";
char pathUnten[] = "/gw/unten/";
char pathLoge[] = "/gw/loge/";
char pathTreppe[] = "/gw/treppe/";

// Set the static IP address to use if the DHCP fails to assign
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

EthernetClient client;
EthernetServer server(80);

void httpRequest(char* host, char* path);
void httpResponse(EthernetClient client);

void writeChannels(uint16_t channels[], uint8_t size, uint8_t data);

uint16_t kiosk[] = {106, 113, 120, 127, 134, 141};
uint16_t unten[] = {491};
uint16_t loge[] = {492, 494};
uint16_t treppe[] = {493};

uint8_t sizes[] = {6, 1, 2, 1};

uint8_t color[] = {255, 117, 17};

void setup() {
  DMXSerial.init(DMXController);

  EEPROM.get(0, color[0]);
  EEPROM.get(1, color[1]);
  EEPROM.get(2, color[2]);

  if (Ethernet.begin(mac) == 0) {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      while (true) {
        delay(1);  // do nothing, no point running without Ethernet hardware
      }
    }
    Ethernet.begin(mac, ip, myDns);
  }
  server.begin();

  delay(1000);
}

int StrToHex(const char str[]) { return (int)strtol(str, 0, 16); }

boolean requested[] = {false, false, false};
uint32_t requestTimer = 0;
void loop() {
  if (millis() - requestTimer > 10000) {
    requestTimer = millis();
    requested[0] = false;
    requested[1] = false;
    requested[2] = false;
    if (client.connect(host, 80)) {
      httpRequest(host, pathTreppe);
    }
  } else if (millis() - requestTimer > 7500 && !requested[0]) {
    requested[0] = true;
    if (client.connect(host, 80)) {
      httpRequest(host, pathLoge);
    }
  } else if (millis() - requestTimer > 5000 && !requested[1]) {
    requested[1] = true;
    if (client.connect(host, 80)) {
      httpRequest(host, pathUnten);
    }
  } else if (millis() - requestTimer > 2500 && !requested[2]) {
    requested[2] = true;
    if (client.connect(host, 80)) {
      httpRequest(host, pathKiosk);
    }
  }

  int8_t x = -1;
  while (client.available()) {
    String line = client.readStringUntil('\r');

    if (line.startsWith("\nlocation: kiosk")) x = 0;
    if (line.startsWith("\nlocation: unten")) x = 1;
    if (line.startsWith("\nlocation: loge")) x = 2;
    if (line.startsWith("\nlocation: treppe")) x = 3;

    if (line.length() < 2 && x > -1) {
      uint8_t data = client.readString().toInt();
      if (x == 0) {
        for (auto&& chan : kiosk) {
          DMXSerial.write(chan + 0, map(data, 0, 255, 0, color[0]));  // R
          DMXSerial.write(chan + 1, map(data, 0, 255, 0, color[1]));  // G
          DMXSerial.write(chan + 2, map(data, 0, 255, 0, color[2]));  // B
        }
      } else if (x == 1) {
        writeChannels(unten, sizes[1], data);
      } else if (x == 2) {
        writeChannels(loge, sizes[2], data);
      } else if (x == 3) {
        writeChannels(treppe, sizes[3], data);
      }
      DMXSerial.write(512, 0);
    };
  }

  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\r');
        if (line.startsWith("GET /?color=%23")) {
          color[0] = StrToHex(line.substring(15, 17).c_str());
          color[1] = StrToHex(line.substring(17, 19).c_str());
          color[2] = StrToHex(line.substring(19, 21).c_str());

          EEPROM.update(0, color[0]);
          EEPROM.update(1, color[1]);
          EEPROM.update(2, color[2]);
        }

        if (line.length() < 2) {
          httpResponse(client);
          break;
        }
      }
    }
    delay(5);
    client.stop();
  }

  delay(1);
}

void writeChannels(uint16_t channels[], uint8_t size, uint8_t data) {
  for (uint8_t i = 0; i < size; i++) {
    DMXSerial.write(channels[i], data);
  }
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

void httpResponse(EthernetClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.print("<!DOCTYPE html>");
  client.print("<html><head>");
  client.print("    <meta charset=\"UTF-8\" />");
  client.print("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
  client.print("    <title>Document</title>");
  client.print("  </head>");
  client.print("  <body>");
  client.print("    <form>");
  client.print("      <input type=\"color\" name=\"color\" value=\"#");
  if (color[0] < 16) client.print("0");
  client.print(color[0], 16);
  if (color[1] < 16) client.print("0");
  client.print(color[1], 16);
  if (color[2] < 16) client.print("0");
  client.print(color[2], 16);
  client.print("\"/>");
  client.print("      <input type=\"submit\" />");
  client.print("    </form>");
  client.println("</body></html>");
}