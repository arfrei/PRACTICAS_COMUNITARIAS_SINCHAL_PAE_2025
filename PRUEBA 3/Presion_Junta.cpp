#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <WiFiClient.h>
#include <PubSubClient.h>

// -------------------- Configuración WiFi --------------------
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// -------------------- MQTT ThingSpeak --------------------
const char* mqttServer = "mqtt3.thingspeak.com";
const int mqttPort = 1883;
const char* mqttClientID = "";
const char* mqttUsername = "";
const char* mqttPassword = "";
const char* mqttTopic = "channels//publish/fields/field2";

// -------------------- MQTT Mosquitto (local) --------------------
const char* mqttServerLocal = "";
const int mqttPortLocal = 1883;
const char* mqttUserLocal = "miusuario";
const char* mqttPasswordLocal = "";
const char* mqttTopicLocal = "presion";

// Clientes MQTT
WiFiClient espClientThingSpeak;
WiFiClient espClientLocal;
PubSubClient client(espClientThingSpeak);
PubSubClient clientLocal(espClientLocal);

// -------------------- Sensor --------------------
const int sensorPin = 35;
const int retardo = 100;
int lectura;

// Calibración
const float voltage0 = 0.35, pressure0 = 0;
const float voltage1 = 0.47, pressure1 = 16;
const float voltage2 = 0.64, pressure2 = 34;
const float voltage3 = 0.70, pressure3 = 42;
const float voltage4 = 0.82, pressure4 = 60;
const float voltage5 = 0.88, pressure5 = 66;

// ADC ESP32
const int adcResolution = 4095;
const float adcMaxVoltage = 3.3;

// Filtro promedio
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
long total = 0;

String valorPendiente = "";

// -------------------- Funciones --------------------
void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println(" conectado.");
}

void conectarMQTT() {
  client.setServer(mqttServer, mqttPort);
  if (!client.connected()) {
    Serial.print("Conectando a MQTT ThingSpeak...");
    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println(" conectado.");
    } else {
      Serial.println(" fallo.");
    }
  }
}

void conectarMQTTLocal() {
  clientLocal.setServer(mqttServerLocal, mqttPortLocal);
  if (!clientLocal.connected()) {
    Serial.print("Conectando a MQTT Local...");
    if (clientLocal.connect("ESP32Client", mqttUserLocal, mqttPasswordLocal)) {
      Serial.println(" conectado.");
    } else {
      Serial.println(" fallo.");
    }
  }
}

void enviarDatosMQTT(int valor) {
  String payload = String(valor);

  // ThingSpeak
  if (client.connected()) {
    client.publish(mqttTopic, payload.c_str());
    Serial.println("Publicado en ThingSpeak: " + payload);
  } else {
    Serial.println("Fallo en ThingSpeak.");
  }

  delay(2000); // Espera antes de enviar a Mosquitto

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

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  conectarWiFi();
  conectarMQTT();
  conectarMQTTLocal();

  for (int i = 0; i < numReadings; i++) readings[i] = 0;
}

// -------------------- Loop --------------------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!client.connected()) {
    conectarMQTT();
  }

  if (!clientLocal.connected()) {
    conectarMQTTLocal();
  }

  client.loop();
  clientLocal.loop();

  // Reintento si hay valor pendiente
  if (!valorPendiente.isEmpty() && clientLocal.connected()) {
    if (clientLocal.publish(mqttTopicLocal, valorPendiente.c_str())) {
      Serial.println("Reintento exitoso: " + valorPendiente);
      valorPendiente = "";
    }
  }

  // Lectura del sensor
  total -= readings[readIndex];
  readings[readIndex] = analogRead(sensorPin) + 148;
  total += readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  int averageReading = total / numReadings;
  float measuredVoltage = averageReading * adcMaxVoltage / adcResolution;

  float pressure;
if (measuredVoltage <= voltage0) {
  pressure = pressure0;
} else if (measuredVoltage < voltage1) {
  pressure = pressure0 + (measuredVoltage - voltage0) * (pressure1 - pressure0) / (voltage1 - voltage0);
} else if (measuredVoltage < voltage2) {
  pressure = pressure1 + (measuredVoltage - voltage1) * (pressure2 - pressure1) / (voltage2 - voltage1);
} else if (measuredVoltage < voltage3) {
  pressure = pressure2 + (measuredVoltage - voltage2) * (pressure3 - pressure2) / (voltage3 - voltage2);
} else if (measuredVoltage < voltage4) {
  pressure = pressure3 + (measuredVoltage - voltage3) * (pressure4 - pressure3) / (voltage4 - voltage3);
} else {
  pressure = pressure4 + (measuredVoltage - voltage4) * (pressure5 - pressure4) / (voltage5 - voltage4);
}

  Serial.print("Presión: ");
  Serial.print(pressure, 1);
  Serial.println(" psi");

  enviarDatosMQTT((int)pressure);
  delay(60000); // cada 1 min
}
