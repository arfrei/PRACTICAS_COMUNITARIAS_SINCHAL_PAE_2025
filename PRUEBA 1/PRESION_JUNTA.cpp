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
#define WIFI_SSID "JUNTA DE AGUA"
#define WIFI_PASSWORD "juntadeagua-2325"

// -------------------- Configuración MQTT (ThingSpeak) --------------------
const char* mqtt_server = "mqtt3.thingspeak.com";
const int mqtt_port = 1883;
const char* mqttClientID = "BQUjJxc6JDMVMBQ8HA4wKzE";
const char* mqttUsername = "BQUjJxc6JDMVMBQ8HA4wKzE";
const char* mqttPassword = "8LzDhUTaLNcakESU7peQqAWA";
const char* mqttTopic = "channels/2909722/publish/fields/field2";

WiFiClient espClient;
PubSubClient client(espClient);

// -------------------- Sensor y calibración --------------------
const int sensorPin = 35; // pin valdivia
const int retardo = 100;
int lectura;

// Calibración
const float voltage0 = 0.35;
const float pressure0 = 0;
const float voltage1 = 0.47;
const float pressure1 = 16;
const float voltage2 = 0.64;
const float pressure2 = 34;
const float voltage3 = 0.70;
const float pressure3 = 42;
const float voltage4 = 0.82;
const float pressure4 = 60;

// ADC ESP32
const int adcResolution = 4095;
const float adcMaxVoltage = 3.3;

// Filtro promedio
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
long total = 0;

// -------------------- Funciones --------------------
void wifi_and_connection_init() {
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
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println(" conectado.");
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos.");
      delay(5000);
    }
  }
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  wifi_and_connection_init();
  conectarMQTT();

  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

// -------------------- Loop principal --------------------
void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();

  total = total - readings[readIndex];
  readings[readIndex] = analogRead(sensorPin) + 148;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  int averageReading = total / numReadings;
  float measuredVoltage = (float)averageReading * adcMaxVoltage / adcResolution;

  Serial.print("ADC promedio: ");
  Serial.print(averageReading);
  Serial.print(" → ");
  Serial.print(measuredVoltage, 3);
  Serial.println(" V");

  float pressure;
  if (measuredVoltage <= voltage0) {
    pressure = pressure0;
  } else if (measuredVoltage < voltage1) {
    float slope = (pressure1 - pressure0) / (voltage1 - voltage0);
    pressure = pressure0 + (measuredVoltage - voltage0) * slope;
  } else if (measuredVoltage <= voltage2) {
    float slope = (pressure2 - pressure1) / (voltage2 - voltage1);
    pressure = pressure1 + (measuredVoltage - voltage1) * slope;
  } else if (measuredVoltage <= voltage3) {
    float slope = (pressure3 - pressure2) / (voltage3 - voltage2);
    pressure = pressure2 + (measuredVoltage - voltage2) * slope;
  } else {
    float slope = (pressure4 - pressure3) / (voltage4 - voltage3);
    pressure = pressure3 + (measuredVoltage - voltage3) * slope;
  }

  Serial.print("Presión: ");
  Serial.print(pressure, 1);
  Serial.println(" psi");

  // Enviar a ThingSpeak (field2)
  String payload = String(int(pressure));
  if (client.publish(mqttTopic, payload.c_str())) {
    Serial.println("Publicado MQTT: " + payload);
  } else {
    Serial.println("Fallo al publicar MQTT.");
  }

  delay(10000); // 1 dato cada 10 segundos para ThingSpeak
}
