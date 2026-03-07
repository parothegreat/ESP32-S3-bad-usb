#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "usb_keyboard.hpp"

const char* ssid     = "your_wifi_ssid";              // ← pastikan SSID benar
const char* password = "your_wifi_password";      // ← pastikan password benar

HIDkeyboard keyboard;
AsyncWebServer server(80);

// ─── Helper ────────────────────────────────────────────────────────────────

void sendChar(char c) {
  uint8_t const conv_table[128][2] = { HID_ASCII_TO_KEYCODE };
  if (c < 0 || c > 127) return;

  uint8_t modifier = conv_table[c][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
  uint8_t keycode  = conv_table[c][1];

  keyboard.sendKey(keycode, modifier);
  delay(50);
  keyboard.sendKey(0, 0);
  delay(20);
}

void sendString(const String& str) {
  for (size_t i = 0; i < str.length(); i++) {
    sendChar(str[i]);
  }
}

bool waitHIDReady(int timeoutMs = 5000) {
  int elapsed = 0;
  while (!tud_hid_n_ready(0)) {
    delay(10);
    elapsed += 10;
    if (elapsed >= timeoutMs) {
      Serial.println("[HID] ✗ Timeout — skipping.");
      return false;
    }
  }
  return true;
}

// ─── HID Actions ──────────────────────────────────────────────────────────

void openCMD() {
  if (!waitHIDReady()) return;
  keyboard.sendKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
  delay(200);
  keyboard.sendKey(0, 0);
  delay(300);
  sendString("cmd\n");
}

void openNotepad() {
  if (!waitHIDReady()) return;
  keyboard.sendKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
  delay(200);
  keyboard.sendKey(0, 0);
  delay(300);
  sendString("notepad\n");
}

void openBrowser() {
  openCMD();
  delay(1000);
  keyboard.sendKey(0, 0);
  delay(300);
  sendString("start https://www.microsoft.com\n");
}

void runSystemInfo() {
  openCMD();
  delay(1000);
  sendString("systeminfo\n");
}

void shutdownComputer() {
  openCMD();
  delay(1000);
  sendString("shutdown -s -f -t 60\n");
}

void initialPayload() {
  if (!waitHIDReady(5000)) return;
  keyboard.sendKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
  delay(500);
  keyboard.sendKey(0, 0);
  delay(500);
  sendString("cmd\n");
  delay(1000);
  sendString("echo BadUSB device connected successfully\n");
}

// ─── WiFi ─────────────────────────────────────────────────────────────────

void scanAndConnect() {
  Serial.println("[WiFi] Scanning networks...");

  int n = WiFi.scanNetworks();
  int32_t bestRSSI    = -999;
  uint8_t bestBSSID[6];
  int     bestChannel = 0;
  bool    found       = false;

  for (int i = 0; i < n; i++) {
    Serial.print("  [");
    Serial.print(i);
    Serial.print("] ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" | ");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" dBm | Ch: ");
    Serial.println(WiFi.channel(i));

    if (WiFi.SSID(i) == ssid && WiFi.RSSI(i) > bestRSSI) {
      bestRSSI    = WiFi.RSSI(i);
      bestChannel = WiFi.channel(i);
      memcpy(bestBSSID, WiFi.BSSID(i), 6);
      found = true;
    }
  }

  WiFi.scanDelete();

  if (!found) {
    Serial.println("[WiFi] ✗ SSID not found!");
    return;
  }

  Serial.printf("[WiFi] Connecting | RSSI: %d dBm | Ch: %d\n", bestRSSI, bestChannel);

  // connect dengan BSSID + channel spesifik agar lebih reliable
  WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK);  
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
    Serial.print(".");
    if (attempts % 10 == 0) {
      Serial.printf(" [status:%d]", WiFi.status());
    }
  }
  Serial.println();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);

  scanAndConnect();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] ✓ Connected!");
    Serial.println("[WiFi] IP   : " + WiFi.localIP().toString());
    Serial.println("[WiFi] RSSI : " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("[WiFi] ✗ Failed (status: " + String(WiFi.status()) + ")");
    Serial.println("  1=NO_SSID | 3=CONNECTED | 4=FAIL | 6=DISCONNECTED");
  }
}

// ─── Web Server ───────────────────────────────────────────────────────────

void setupServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>ESP32 HID Control</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body { font-family: sans-serif; padding: 20px; }
          button { padding: 10px 20px; margin: 5px; font-size: 16px; cursor: pointer; }
          input  { padding: 10px; font-size: 16px; width: 200px; }
        </style>
      </head>
      <body>
        <h2>ESP32-S3 HID Control</h2>
        <button onclick="fetch('/cmd')">Open CMD</button><br><br>
        <button onclick="fetch('/notepad')">Open Notepad</button><br><br>
        <button onclick="fetch('/browser')">Open Browser</button><br><br>
        <button onclick="fetch('/systeminfo')">Run systeminfo</button><br><br>
        <button onclick="fetch('/shutdown')">Shutdown PC</button><br><br>
        <input type="text" id="text" placeholder="Custom text">
        <button onclick="sendText()">Send Text</button>
        <script>
          function sendText() {
            let txt = document.getElementById('text').value;
            fetch('/custom?text=' + encodeURIComponent(txt));
          }
        </script>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/cmd",        HTTP_GET, [](AsyncWebServerRequest *r){ openCMD();         r->send(200, "text/plain", "CMD executed");       });
  server.on("/notepad",    HTTP_GET, [](AsyncWebServerRequest *r){ openNotepad();      r->send(200, "text/plain", "Notepad opened");      });
  server.on("/browser",    HTTP_GET, [](AsyncWebServerRequest *r){ openBrowser();      r->send(200, "text/plain", "Browser opened");      });
  server.on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest *r){ runSystemInfo();    r->send(200, "text/plain", "Systeminfo executed"); });
  server.on("/shutdown",   HTTP_GET, [](AsyncWebServerRequest *r){ shutdownComputer(); r->send(200, "text/plain", "Shutdown started");    });

  server.on("/custom", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("text")) {
      sendString(request->getParam("text")->value() + "\n");
      request->send(200, "text/plain", "Text sent");
    } else {
      request->send(400, "text/plain", "Missing 'text' parameter");
    }
  });

  server.begin();
}

// ─── Setup ────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== ESP32-S3 BadUSB ===");

  keyboard.begin();
  Serial.println("[HID] Keyboard initialized");

  connectWiFi();
  setupServer();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[Server] ✓ http://" + WiFi.localIP().toString() + "/");
  } else {
    Serial.println("[Server] ⚠ Started without WiFi");
  }

  delay(1000);
  initialPayload();
}

// ─── Loop ─────────────────────────────────────────────────────────────────

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Lost, reconnecting...");
    connectWiFi();
  }
  delay(10000);
}