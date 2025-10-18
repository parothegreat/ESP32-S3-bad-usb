# 🧠 BadUSB Educativo con ESP32-S3 + PlatformIO

Este proyecto demuestra cómo un ESP32-S3 puede simular un teclado HID (Human Interface Device) para ejecutar comandos en un PC, controlado remotamente desde una interfaz web. Está diseñado exclusivamente para fines educativos y debe usarse en entornos seguros y controlados.

---

## ⚙️ Requisitos

### Hardware
- ESP32-S3 con soporte HID (Que tenga un puerto OTG)
- Cable USB-C para conexión al PC

### Software
- [PlatformIO](https://platformio.org/) en Visual Studio Code
- Arduino framework
- Librerías:
  - `ESPAsyncWebServer`
  - `WiFi`
  - `usb_keyboard.hpp` (parte de esptinyusb)

---

## 🚀 ¿Qué hace este proyecto?

Al conectar el ESP32-S3 al PC y acceder a su IP local, se presenta una interfaz web con botones que simulan acciones de teclado:

- Abrir CMD
- Abrir Bloc de notas
- Abrir navegador con URL específica
- Ejecutar `systeminfo`
- Apagar el PC
- Enviar texto personalizado

Estas acciones se ejecutan como si el ESP32-S3 fuera un teclado físico.

---

## 🧩 Estructura del código

### 1. 🔌 Conexión Wi-Fi

```cpp
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    Serial.println("\nConectado!");
    Serial.print("IP local: ");
    Serial.println(WiFi.localIP());
```

Establece la conexión del ESP32-S3 a la red local usando las credenciales definidas.

### 2. 🎹 Inicialización del teclado HID
```cpp
    keyboard.init();
    keyboard.begin(1);
```

Activa el modo HID para que el ESP32-S3 se comporte como un teclado USB.

### 3. 🧠 Funciones de simulación de teclado

#### Enviar caracteres individuales

```cpp
    void sendChar(char c)
```

Convierte un carácter en su código HID correspondiente y lo envía como pulsación de tecla.


#### Enviar cadenas completas

```cpp
    void sendChar(char c)
    void sendString(const String& str)
```

Envía una cadena de texto carácter por carácter como si se escribiera desde un teclado físico.

### 4. 🖥️ Payloads predefinidos

#### Abrir CMD

```cpp
    void ejecutarCMD()
```

Simula Win + R, escribe cmd y presiona Enter.

#### Abrir Bloc de notas

```cpp
    void abrirBlocNotas()
```

Simula Win + R, escribe notepad y presiona Enter.

#### Abrir navegador con URL

```cpp
    void abrirNavegador()
```

Abre CMD y ejecuta start https://www.microsoft.com.

#### Mostrar información del sistema

```cpp
    void ejecutarSystemInfo()
```

Ejecuta el comando systeminfo en la terminal.

#### Apagar el PC

```cpp
    void shutdownPC()
```

Ejecuta shutdown -s -f -t 60 para apagar el sistema en 60 segundos.

#### Payload inicial

```cpp
    void payloadInicial()
```

Envía un mensaje de confirmación al abrir CMD al iniciar el dispositivo. (Para que sea puramente una BadUsb acá es donde se pondrían todos los comandos que se ejecutarían al conectarse al dispositivo)

### 5. 🌐 Servidor web

#### Página principal

```cpp
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ ... });
```

Genera una interfaz HTML con botones para ejecutar cada función HID desde el navegador.

#### Endpoints funcionales

Cada botón en la interfaz web está vinculado a un endpoint como /cmd, /notepad, /browser, etc., que ejecuta la función correspondiente en el ESP32-S3.

#### Envío de texto personalizado

```cpp
    server.on("/custom", HTTP_GET, [](AsyncWebServerRequest *request){ ... });
```

Permite al usuario enviar texto arbitrario desde la interfaz web para que el ESP32 lo escriba en el PC.


## ⚠️ NOTA

En esta versión es importante estar conectado a una red Wi-Fi para que pueda seguir ejecutando el payload inicial y habilitar el servidor web.  
Si se elimina la lógica de conexión o se establece un tiempo máximo para conectarse, **solo se ejecutará el payload inicial**, ya que el resto de las funciones dependen de la red para recibir comandos desde la interfaz web.
