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
EthernetServer server(80);

void httpRequest(char* host, char* path);
void httpResponse(EthernetClient client);

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
  server.begin();

  delay(1000);
}

int StrToHex(const char str[]) { return (int)strtol(str, 0, 16); }

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

  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\r');
        if (line.startsWith("GET /?color=%23")) {
          color[0] = StrToHex(line.substring(15, 17).c_str());
          color[1] = StrToHex(line.substring(17, 19).c_str());
          color[2] = StrToHex(line.substring(19, 21).c_str());
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