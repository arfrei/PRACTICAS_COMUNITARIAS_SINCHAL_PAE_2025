#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "HT_SSD1306Wire.h" // Pantalla OLED

#ifdef WIRELESS_STICK_V3
SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);
#else
SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
#endif

// WiFi
const char* ssid = "TP-Link_0BA0";
const char* password = "19210360";

// MQTT
const char* mqtt_server = "192.168.0.105";
const int mqtt_port = 1883;
const char* mqtt_user = "miusuario";
const char* mqtt_password = "brokerraspsinchal2025";
const char* mqtt_topic = "sensor/altura";

WiFiClient espClient;
PubSubClient client(espClient);

// Estado
bool estadoWiFi = false;
bool estadoMQTT = false;
String valorPendiente = "";

// LED indicador
#define LED_PIN 35

void actualizarPantalla(int valor) {
  display.clear();
  display.drawString(0, 0, "ENVIO ALTURA");
  display.drawString(0, 20, "Altura: " + String(valor) + " cm");
  display.drawString(0, 40, estadoWiFi ? "WiFi: OK" : "WiFi: FAIL");
  display.drawString(64, 40, estadoMQTT ? "MQTT: OK" : "MQTT: FAIL");
  display.display();
}

void conectarWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi...");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
    estadoWiFi = WiFi.status() == WL_CONNECTED;
    Serial.println(estadoWiFi ? "\nWiFi conectado." : "\nFallo WiFi.");
  } else {
    estadoWiFi = true;
  }
}

void conectarMQTT() {
  if (!client.connected() && estadoWiFi) {
    Serial.print("Conectando a MQTT...");
    if (client.connect("HeltecClient", mqtt_user, mqtt_password)) {
      Serial.println("Conectado a MQTT.");
      estadoMQTT = true;
    } else {
      Serial.print("Fallo MQTT, estado: ");
      Serial.println(client.state());
      estadoMQTT = false;
    }
  } else if (client.connected()) {
    estadoMQTT = true;
  }
}

void enviarAltura(int altura) {
  String payload = String(altura);
  if (estadoMQTT) {
    if (client.publish(mqtt_topic, payload.c_str())) {
      Serial.println("Publicado: " + payload);
      valorPendiente = "";
    } else {
      Serial.println("Fallo al publicar, guardando en buffer.");
      valorPendiente = payload;
    }
  } else {
    Serial.println("MQTT desconectado, guardando en buffer.");
    valorPendiente = payload;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  display.init();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.display();

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  conectarWiFi();
  conectarMQTT();
  client.loop();

  if (estadoWiFi && estadoMQTT) {
    // Primero intenta reenviar el valor pendiente
    if (!valorPendiente.isEmpty()) {
      if (client.publish(mqtt_topic, valorPendiente.c_str())) {
        Serial.println("Reintento publicación: " + valorPendiente);
        valorPendiente = "";
      } else {
        Serial.println("Fallo al reenviar valor pendiente.");
      }
    } else {
      // Si no hay pendiente, genera y envía uno nuevo
      int altura = random(10, 101);
      String payload = String(altura);
      if (client.publish(mqtt_topic, payload.c_str())) {
        Serial.println("Publicado: " + payload);
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
      } else {
        Serial.println("Fallo al publicar, guardando en buffer.");
        valorPendiente = payload;
      }
      actualizarPantalla(altura);
    }
  }

  delay(15000); // Espera 15 segundos
}
