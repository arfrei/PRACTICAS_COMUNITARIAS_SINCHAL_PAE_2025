// Librerías necesarias
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Pantalla OLED
#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
#endif

// LoRa
#define RF_FREQUENCY 915000000
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 11
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define BUFFER_SIZE 10
#define LED_PIN 35

// LoRa variables
bool paqueteRecibido = false;
char rxpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;
int altura;
int16_t rssi, rxSize;
bool lora_idle = true;

// WiFi
const char* ssid = "";
const char* password = "";
bool estadoWiFi = false;

// MQTT ThingSpeak
const char* mqttServer = "mqtt3.thingspeak.com";
const int mqttPort = 1883;
const char* mqttClientID = "";
const char* mqttUsername = "";
const char* mqttPassword = "";
const char* mqttTopic = "channels//publish/fields/field1";
WiFiClient espClient;
PubSubClient client(espClient);
bool estadoMQTT = false;

// MQTT Mosquitto - Raspberry Pi
const char* mqttServerLocal = "";
const int mqttPortLocal = 1883;
const char* mqttUserLocal = "miusuario";
const char* mqttPasswordLocal = "";
const char* mqttTopicLocal = "nivel";
WiFiClient espClientLocal;
PubSubClient clientLocal(espClientLocal);
String valorPendiente = "";

// Timeout y reconexión
unsigned long ultimoPaqueteMillis = 0;
unsigned long intervaloTimeout = 150000; // 2 min 30 s
bool falloLoraReportado = false;
unsigned long ultimoIntentoWiFi = 0;
const unsigned long intervaloWiFi = 10000;

// Pantalla
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
  display.display();
}

// Reconectar WiFi
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

// Reconectar ThingSpeak
void reconnectMQTT() {
  if (estadoWiFi && !client.connected()) {
    Serial.print("Intentando conectar a MQTT ThingSpeak...");
    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println("MQTT ThingSpeak conectado.");
      estadoMQTT = true;
    } else {
      Serial.println("MQTT ThingSpeak fallo: " + String(client.state()));
      estadoMQTT = false;
    }
  } else if (client.connected()) {
    estadoMQTT = true;
  } else {
    estadoMQTT = false;
  }
}

// Reconectar Mosquitto
void reconnectMQTTLocal() {
  if (estadoWiFi && !clientLocal.connected()) {
    Serial.print("Intentando conectar a MQTT Local...");
    if (clientLocal.connect("HeltecClient", mqttUserLocal, mqttPasswordLocal)) {
      Serial.println("MQTT Local conectado.");
    } else {
      Serial.println("MQTT Local fallo: " + String(clientLocal.state()));
    }
  }
}

// Enviar datos
void enviarDatos(int valor) {
  String payload = String(valor);

  // ThingSpeak
  if (client.connected()) {
    client.publish(mqttTopic, payload.c_str());
    Serial.println("Publicado en ThingSpeak: " + payload);
    estadoMQTT = true;
  } else {
    estadoMQTT = false;
  }

  delay(2000); // Retardo antes de enviar a Mosquitto

  // Mosquitto
  if (clientLocal.connected()) {
    if (clientLocal.publish(mqttTopicLocal, payload.c_str())) {
      Serial.println("Publicado en Mosquitto: " + payload);
      valorPendiente = "";
    } else {
      Serial.println("Fallo al publicar en Mosquitto, guardando.");
      valorPendiente = payload;
    }
  } else {
    Serial.println("Mosquitto desconectado, guardando valor.");
    valorPendiente = payload;
  }
}

void setup() {
  Serial.begin(115200);
  Mcu.begin();
  pinMode(LED_PIN, OUTPUT);

  display.init();
  display.clear();
  display.display();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  WiFi.begin(ssid, password);
  ultimoIntentoWiFi = millis();

  client.setServer(mqttServer, mqttPort);
  clientLocal.setServer(mqttServerLocal, mqttPortLocal);

  Serial.println("Setup completo.");
}

void loop() {
  reconnectWiFi();
  reconnectMQTT();
  reconnectMQTTLocal();
  client.loop();
  clientLocal.loop();

  // Reintento si hay valor pendiente
  if (!valorPendiente.isEmpty() && clientLocal.connected()) {
    if (clientLocal.publish(mqttTopicLocal, valorPendiente.c_str())) {
      Serial.println("Reintento exitoso: " + valorPendiente);
      valorPendiente = "";
    } else {
      Serial.println("Fallo en reintento a Mosquitto.");
    }
  }

  if (lora_idle) {
    lora_idle = false;
    Radio.Rx(0);
  }

  if (paqueteRecibido) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    paqueteRecibido = false;
  }

  Radio.IrqProcess();

  bool loraOK = millis() - ultimoPaqueteMillis <= intervaloTimeout;

  if (!loraOK && !falloLoraReportado) {
    altura = -21;
    enviarDatos(-21);
    Serial.println("Fallo LoRa: enviado -21");
    falloLoraReportado = true;
  }

  actualizarPantalla(loraOK);
  delay(10);
}

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
    enviarDatos(altura);
    ultimoPaqueteMillis = millis();
    falloLoraReportado = false;
  }
}
