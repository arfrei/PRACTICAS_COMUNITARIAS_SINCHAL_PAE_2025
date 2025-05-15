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

// -------------------- Configuración LED Indicador --------------------
#define LED_PIN 2  // Pin para el LED azul de comunicación (GPIO2 es el LED integrado en muchas placas ESP32)
#define LED_ON LOW   

// -------------------- Configuración Red WiFi --------------------
#define WIFI_SSID "JUNTA DE AGUA"
#define WIFI_PASSWORD "juntadeagua-2325"

// -------------------- Configuración MQTT (ThingSpeak) --------------------
const char* mqtt_server = "mqtt3.thingspeak.com";
const int mqtt_port = 1883;
const char* mqttClientID = "Oyk6MgcFHTEdNzMEJQsLEzs";
const char* mqttUsername = "Oyk6MgcFHTEdNzMEJQsLEzs";
const char* mqttPassword = "dslPNQ0RZ7JkKh4q+BQO7pLH";
const char* mqttTopic = "channels/2941382/publish/fields/field2";

WiFiClient espClient;
PubSubClient client(espClient);

// -------------------- MQTT Mosquitto (local) --------------------
const char* mqttServerLocal = "192.168.1.135"; // IP de tu broker local
const int mqttPortLocal = 1883;
const char* mqttUserLocal = "miusuario";
const char* mqttPasswordLocal = "brokerraspsinchal2025";
const char* mqttTopicLocal = "presion";

WiFiClient espClientLocal;
PubSubClient clientLocal(espClientLocal);

String valorPendiente = ""; // Para reintento

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

// Variables para control de tiempo de envío
unsigned long lastSampleTime = 0;
unsigned long sampleInterval = 5000;  // Tomar muestras cada 5 segundos
unsigned long lastPublishTime = 0;
unsigned long publishInterval = 60000; // Publicar cada 60 segundos (1 minuto)
float currentPressure = 0;
bool systemStabilized = false;
int stabilizationCount = 0;
const int requiredStabilizationSamples = 3; // Número de muestras estables requeridas

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
  int intentos = 0;
  while (!client.connected() && intentos < 5) {
    Serial.print("Conectando a MQTT...");
    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println(" conectado.");
      // Publicar un mensaje de prueba para verificar la comunicación
      if (client.publish(mqttTopic, "Iniciando")) {
        Serial.println("Comunicación MQTT verificada - LED debería encenderse");
      } else {
        Serial.println("Alerta: Conectado pero no puede publicar - Verificar credenciales");
      }
    } else {
      intentos++;
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos.");
      delay(5000);
    }
  }
  
  if (!client.connected()) {
    Serial.println("No se pudo conectar a MQTT después de varios intentos.");
    Serial.println("El LED azul debería estar APAGADO en este momento.");
  }
}

void conectarMQTTLocal() {
  clientLocal.setServer(mqttServerLocal, mqttPortLocal);
  int intentos = 0;
  while (!clientLocal.connected() && intentos < 5) {
    Serial.print("Conectando a MQTT Local...");
    if (clientLocal.connect("ESP32Client", mqttUserLocal, mqttPasswordLocal)) {
      Serial.println(" conectado a Mosquitto.");
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(clientLocal.state());
      Serial.println(" reintentando en 5 segundos.");
      intentos++;
      delay(5000);
    }
  }

  if (!clientLocal.connected()) {
    Serial.println("No se pudo conectar a MQTT Local después de varios intentos.");
  }
}

float calculatePressure(float measuredVoltage) {
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
  return pressure;
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  
  // Esperar un momento para que el serial se inicialice completamente
  delay(2000);
  Serial.println("\n\n----- INICIANDO SISTEMA -----");
  
  // Inicializar WiFi con registro detallado
  Serial.println("Iniciando conexión WiFi...");
  wifi_and_connection_init();
  
  // Iniciar conexión MQTT
  Serial.println("Iniciando conexión MQTT...");
  conectarMQTT();
  
  conectarMQTTLocal();

  // Inicializar array de lecturas
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  
  Serial.println("Sistema iniciado. Estabilizando lecturas...");
  Serial.println("Observa el LED azul - debería encenderse durante la comunicación MQTT");
}

// -------------------- Loop principal --------------------
void loop() {
  unsigned long currentMillis = millis();
  
  // Verificar conexión MQTT y mantener la conexión activa
  if (!client.connected()) {
    Serial.println("Conexión MQTT perdida - El LED azul debería estar APAGADO");
    conectarMQTT();
  }
  client.loop();  // Este es crucial para mantener la conexión MQTT activa
  
  clientLocal.loop(); // Mantener la conexión con Mosquitto

if (!clientLocal.connected()) {
  Serial.println("Conexión a Mosquitto perdida");
  conectarMQTTLocal();
}

// Reintento si había valor pendiente
if (!valorPendiente.isEmpty() && clientLocal.connected()) {
  if (clientLocal.publish(mqttTopicLocal, valorPendiente.c_str())) {
    Serial.println("Reintento exitoso (Mosquitto): " + valorPendiente);
    valorPendiente = "";
  }
}

  // Calcular presión en intervalos regulares sin importar si se publica o no
  if (currentMillis - lastSampleTime >= sampleInterval) {
    lastSampleTime = currentMillis;
    
    // Actualizar promedio móvil
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(sensorPin) + 148;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % numReadings;

    int averageReading = total / numReadings;
    float measuredVoltage = (float)averageReading * adcMaxVoltage / adcResolution;

    // Calcular presión
    float newPressure = calculatePressure(measuredVoltage);
    
    // Imprimir información de depuración
    Serial.print("ADC: ");
    Serial.print(averageReading);
    Serial.print(" → ");
    Serial.print(measuredVoltage, 3);
    Serial.print("V → ");
    Serial.print(newPressure, 1);
    Serial.println(" psi");
    
    // Verificar si el sistema se ha estabilizado
    if (!systemStabilized) {
      if (newPressure > 0) {
        stabilizationCount++;
        if (stabilizationCount >= requiredStabilizationSamples) {
          systemStabilized = true;
          Serial.println("Sistema estabilizado!");
        }
      } else {
        stabilizationCount = 0; // Reiniciar contador si volvemos a cero
      }
    }
    
    // Actualizar la presión actual
    currentPressure = newPressure;
  }

  // Publicar a ThingSpeak cada minuto, pero solo si el sistema está estabilizado
  if (systemStabilized && (currentMillis - lastPublishTime >= publishInterval)) {
    lastPublishTime = currentMillis;
    
    // Verificar nuevamente la conexión antes de publicar
    if (!client.connected()) {
      Serial.println("Reconectando a MQTT antes de publicar...");
      conectarMQTT();
    }
    
    // Enviar a ThingSpeak (field2)
    String payload = String(int(currentPressure));
    
    Serial.println("Intentando publicar en MQTT - El LED azul debería ENCENDERSE ahora");
    
    // Intenta publicar hasta 3 veces si falla
    bool publicado = false;
    for (int i = 0; i < 3 && !publicado; i++) {
      if (client.publish(mqttTopic, payload.c_str())) {
        Serial.println("Publicado MQTT: " + payload + " (minuto: " + String(currentMillis / 60000) + ")");
        Serial.println("Comunicación exitosa - El LED azul debería estar ENCENDIDO");
        publicado = true;
      } else {
        Serial.println("Intento " + String(i+1) + " - Fallo al publicar MQTT.");
        delay(1000);
      }
    }
    
    if (!publicado) {
      Serial.println("Fallo persistente al publicar MQTT - El LED azul debería estar APAGADO");
      Serial.println("Verifica la conexión a internet y las credenciales de ThingSpeak");
    }

    // Publicar también en Mosquitto
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
  
  delay(100); // Pequeño delay para no sobrecargar el CPU
}
