#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "usb_keyboard.hpp"

const char* ssid = "ssid_de_tu_red";
const char* password = "tu_contraseña";

esptinyusb::USBkeyboard keyboard;
AsyncWebServer server(80);



void sendChar(char c) {
  uint8_t const conv_table[128][2] = { HID_ASCII_TO_KEYCODE };
  if (c < 0 || c > 127) return;

  uint8_t modifier = conv_table[c][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
  uint8_t keycode = conv_table[c][1];

  keyboard.sendKey(modifier, keycode);
  delay(50);
  keyboard.sendKey();  // release
  delay(20);
}

void sendString(const String& str) {
  for (size_t i = 0; i < str.length(); i++) {
    sendChar(str[i]);
  }
}

void ejecutarCMD() {
  while (!tud_hid_n_ready(0)) delay(10);
  keyboard.sendKey(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R);
  delay(200);
  keyboard.sendKey();  // release
  delay(300);
  sendString("cmd\n");
}

void abrirBlocNotas() {
  keyboard.sendKey(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R);
  delay(200);
  keyboard.sendKey();
  delay(300);
  sendString("notepad\n");
}

void abrirNavegador() {
  ejecutarCMD();
  delay(1000);
  keyboard.sendKey();
  delay(300);
  sendString("start https://www.microsoft.com\n");
}

void ejecutarSystemInfo() {
  ejecutarCMD();
  delay(1000);
  sendString("systeminfo\n");
}

void shutdownPC() {
  ejecutarCMD();
  delay(1000);
  sendString("shutdown -s -f -t 60\n");
}

void payloadInicial() {
  while (!tud_hid_n_ready(0)) delay(10);

  keyboard.sendKey(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R);
  delay(500);
  keyboard.sendKey();  // release
  delay(500);

  sendString("cmd\n");
  delay(1000);

  sendString("echo Dispositivo BadUSB conectado correctamente\n");
}

void setup() {
  Serial.begin(115200);
  keyboard.init();
  keyboard.begin(1);

  // ← Se ejecuta automáticamente al iniciar
  delay(2000);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
  delay(2000);
  payloadInicial();  
  delay(5000);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head><title>ESP32 HID Control</title></head>
      <body>
        <h2>Control HID desde la red local</h2>
        <button onclick="fetch('/cmd')">Abrir CMD</button><br><br>
        <button onclick="fetch('/notepad')">Abrir Bloc de notas</button><br><br>
        <button onclick="fetch('/browser')">Abrir navegador</button><br><br>
        <button onclick="fetch('/systeminfo')">Ejecutar systeminfo</button><br><br>
        <button onclick="fetch('/shutdown')">Apagar PC</button><br><br>
        <input type="text" id="texto" placeholder="Texto personalizado">
        <button onclick="enviarTexto()">Enviar texto</button>
        <script>
          function enviarTexto() {
            let txt = document.getElementById('texto').value;
            fetch('/custom?text=' + encodeURIComponent(txt));
          }
        </script>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request){
    ejecutarCMD();
    request->send(200, "text/plain", "CMD ejecutado");
  });

  server.on("/notepad", HTTP_GET, [](AsyncWebServerRequest *request){
    abrirBlocNotas();
    request->send(200, "text/plain", "Bloc de notas abierto");
  });

  server.on("/browser", HTTP_GET, [](AsyncWebServerRequest *request){
    abrirNavegador();
    request->send(200, "text/plain", "Navegador abierto");
  });

  server.on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest *request){
    ejecutarSystemInfo();
    request->send(200, "text/plain", "Systeminfo ejecutado");
  });

  server.on("/shutdown", HTTP_GET, [](AsyncWebServerRequest *request){
    shutdownPC();
    request->send(200, "text/plain", "Apagado iniciado");
  });

  server.on("/custom", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("text")) {
      String txt = request->getParam("text")->value();
      sendString(txt + "\n");
      request->send(200, "text/plain", "Texto enviado con Enter");
    } else {
      request->send(400, "text/plain", "Falta parámetro 'text'");
    }
  });

  server.begin();
}

void loop() {}
