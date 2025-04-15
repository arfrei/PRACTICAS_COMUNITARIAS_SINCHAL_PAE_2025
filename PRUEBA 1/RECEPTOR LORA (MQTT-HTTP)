// Librerías necesarias
#include "LoRaWan_APP.h"                // Librería para LoRa
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"            // Librería para la pantalla OLED
#include <WiFi.h>                      // Librería WiFi para ESP32
#include <PubSubClient.h>             // Librería para cliente MQTT
#include <HTTPClient.h>               // Librería para enviar peticiones HTTP

// Configuración de la pantalla OLED
#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
#endif

// Configuración de LoRa
#define RF_FREQUENCY 915000000
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 11
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 10
#define LED_PIN 35                     // Pin para LED indicador

// Variables LoRa
bool paqueteRecibido = false;
char rxpacket[BUFFER_SIZE];           // Buffer de recepción
static RadioEvents_t RadioEvents;
int altura;
int16_t rssi, rxSize;
bool lora_idle = true;

// Configuración de WiFi
const char* ssid = "";
const char* password = "";
bool estadoWiFi = false;

// Configuración de MQTT (ThingSpeak)
const char* mqttServer = "mqtt3.thingspeak.com";
const int mqttPort = 1883;
const char* mqttClientID = "";
const char* mqttUsername = "";
const char* mqttPassword = "";
const char* mqttTopic = "channels/channelId/publish/fields/field1"; //Reemplazar por el Id del canal
WiFiClient espClient;
PubSubClient client(espClient);
bool estadoMQTT = false;

// Configuración de HTTP
String serverUrl = "http://localhost:5000/guardar_altura"; //Reemplazar localhost por la ip fija del servidor Flask
bool estadoHTTP = false;

// Variables para control de timeout y reconexión
unsigned long ultimoPaqueteMillis = 0;
unsigned long intervaloTimeout = 25000;
bool falloLoraReportado = false;
unsigned long ultimoIntentoWiFi = 0;
const unsigned long intervaloWiFi = 10000;

// Actualiza pantalla OLED con información del estado actual
void actualizarPantalla(bool loraOK) {
  display.clear();
  if (loraOK) {
    display.drawString(0, 0, "SINCHAL RECEPTOR");
    display.drawString(0, 20, "Altura: " + String(altura) + " cm");
  } else {
    display.drawString(0, 0, "NO HAY SEÑAL LORA");
    display.drawString(0, 20, "Altura: -21 cm");
  }
  display.drawString(0, 35, estadoWiFi ? "WiFi: OK" : "WiFi: FAIL");
  display.drawString(64, 35, estadoMQTT ? "MQTT: OK" : "MQTT: FAIL");
  display.drawString(0, 50, estadoHTTP ? "HTTP: OK" : "HTTP: FAIL");
  display.display();
}

// Reconexión a WiFi si se pierde la conexión
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    estadoWiFi = false;
    if (millis() - ultimoIntentoWiFi > intervaloWiFi) {
      Serial.println("Intentando conectar a WiFi...");
      WiFi.begin(ssid, password);
      ultimoIntentoWiFi = millis();
    }
  } else {
    estadoWiFi = true;
  }
}

// Reconexión a MQTT si es necesario
void reconnectMQTT() {
  if (estadoWiFi && !client.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println("MQTT conectado.");
      estadoMQTT = true;
    } else {
      Serial.println("MQTT fallo: " + String(client.state()));
      estadoMQTT = false;
    }
  } else if (client.connected()) {
    estadoMQTT = true;
  } else {
    estadoMQTT = false;
  }
}

// Verifica si el servidor HTTP está disponible
void verificarHTTP() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient httpClient;
    HTTPClient http;

    if (http.begin(httpClient, serverUrl)) {
      int httpCode = http.GET();
      if (httpCode > 0) {
        estadoHTTP = true;
      } else {
        estadoHTTP = false;
      }
      http.end();
    } else {
      estadoHTTP = false;
    }
  } else {
    estadoHTTP = false;
  }
}

// Envía los datos por MQTT y HTTP
void enviarDatos(int valor) {
  String payload = String(valor);

  // Enviar por MQTT
  if (client.connected()) {
    client.publish(mqttTopic, payload.c_str());
    Serial.println("Publicado MQTT: " + payload);
    estadoMQTT = true;
  } else {
    estadoMQTT = false;
  }

  // Enviar por HTTP
  estadoHTTP = false;
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient httpClient;
    HTTPClient http;
    if (http.begin(httpClient, serverUrl)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "altura=" + String(valor);
      int httpCode = http.POST(postData);
      if (httpCode > 0) {
        Serial.println("HTTP respuesta: " + http.getString());
        estadoHTTP = true;
      } else {
        Serial.println("Error HTTP POST: " + String(httpCode));
        verificarHTTP();  // Verificar el estado HTTP si falla
      }
      http.end();
    } else {
      Serial.println("Error iniciando conexión HTTP");
      verificarHTTP();  // Verificar si no se pudo iniciar la conexión
    }
  }
}

// Configuración inicial del ESP32
void setup() {
  Serial.begin(115200);
  Mcu.begin();
  pinMode(LED_PIN, OUTPUT);

  display.init();
  display.clear();
  display.display();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  // Configurar eventos de radio LoRa
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  // Conexión inicial a WiFi
  WiFi.begin(ssid, password);
  ultimoIntentoWiFi = millis();

  // Configurar servidor MQTT
  client.setServer(mqttServer, mqttPort);

  Serial.println("Setup completo.");
}

// Variables para controlar chequeo periódico de HTTP
unsigned long ultimoChequeoHTTP = 0;
const unsigned long intervaloChequeoHTTP = 60000;  // 60 segundos

void loop() {
  reconnectWiFi();       // Reintentar conexión WiFi si es necesario
  reconnectMQTT();       // Reintentar conexión MQTT si es necesario
  client.loop();         // Mantiene el cliente MQTT en funcionamiento

  // Si no hay transmisión activa, ponemos a escuchar el receptor LoRa
  if (lora_idle) {
    lora_idle = false;
    Radio.Rx(0);
  }

  // Parpadeo del LED al recibir paquete
  if (paqueteRecibido) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    paqueteRecibido = false;
  }

  Radio.IrqProcess();  // Procesar interrupciones LoRa

  // Verificar si la señal LoRa está activa (basado en el tiempo del último paquete)
  bool loraOK = millis() - ultimoPaqueteMillis <= intervaloTimeout;

  // Si se pierde señal LoRa y no se ha reportado el fallo
  if (!loraOK && !falloLoraReportado) {
    altura = -21;
    enviarDatos(-21);  // Se envía una altura especial para indicar pérdida de señal
    Serial.println("Fallo LoRa: enviado -21");
    falloLoraReportado = true;
  }

  // Verificar el estado HTTP cada minuto, incluso si no hay datos LoRa
  if (millis() - ultimoChequeoHTTP >= intervaloChequeoHTTP) {
    verificarHTTP();
    ultimoChequeoHTTP = millis();
  }

  // Actualizar pantalla OLED
  actualizarPantalla(loraOK);
  delay(10);
}

// Función que se ejecuta cuando se recibe un paquete LoRa
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssiVal, int8_t snr) {
  rssi = rssiVal;
  rxSize = size;
  memset(rxpacket, 0, sizeof(rxpacket));
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  paqueteRecibido = true;

  int valorRecibido;
  if (sscanf(rxpacket, "%d", &valorRecibido) == 1) {
    altura = valorRecibido;  
    Serial.print("Altura: ");
    Serial.println(altura);
    enviarDatos(altura);              // Enviar datos por MQTT y HTTP
    ultimoPaqueteMillis = millis();   // Se actualiza el tiempo del último paquete recibido
    falloLoraReportado = false;       // Se restablece el estado de fallo LoRa
  }
}
