//Librerias
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// Configuración de la pantalla OLED
#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32,
RST_OLED); // addr , freq , i2c group , resolution , rst
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64,
RST_OLED); // addr , freq , i2c group , resolution , rst
#endif

// Configuración del sensor
#define LEVEL_SENSOR 4      // Pin GPIO donde está conectado el Sensor

// LoRa config TX 
#define RF_FREQUENCY 915000000 // Hz
#define TX_OUTPUT_POWER 21      // dBm (Se podria cambiar a 20 o 19) (21+-1 es el max segun datasheet)
#define LORA_BANDWIDTH 0        // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 11 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5, 2: 4/6, 3: 4/7, 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define TX_TIMEOUT_VALUE      8000 //SE PUEDE CAMBIAR
#define RX_TIMEOUT_VALUE      3000 //SE PUEDE CAMBIAR
#define BUFFER_SIZE 10 // Define the payload size here

// Variables Fisicas
int alturaMaxima = 500; //en cm
float RaCisterna = 7.50; 
float valorsinsumergir = 0.60; 
int portValue; //ADC
float nivelVoltios; //Voltaje del sensor
float diferenciaVoltaje; //Voltaje acondicionado
float valorMaximoAbsoluto; //Voltaje Maximo acondicionado
int alturaActual; //Nivel del Reservorio

// Definicios Adicionales de LoRa
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

bool lora_idle = true;
static RadioEvents_t RadioEvents;

void OnTxDone(void);
void OnTxTimeout(void);

void setup() {
    Serial.begin(115200);
    Mcu.begin();

//Inicializar Pantalla OLED
    display.init();
    display.clear();
    display.display();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

//Inicializar LoRa
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
}

void loop() {
    if (lora_idle) {
        delay(10000);  // Actualizacion cada 10 segundos (SE PUEDE CAMBIAR PARA NO SATURAR DE DATOS)

        portValue = analogRead(LEVEL_SENSOR); // Leer el valor de GPIO 4

       //Acondicionamiento del Voltaje
        nivelVoltios = (portValue * 3.3) / 4095.0;
        diferenciaVoltaje = (nivelVoltios - valorsinsumergir);
        valorMaximoAbsoluto = 3.0 - valorsinsumergir;
        alturaActual = ((diferenciaVoltaje * alturaMaxima) / valorMaximoAbsoluto)+2; //Aproximacion Lineal + Factor de Correccion

        // Mostrar los datos en la pantalla OLED
        display.clear();
        display.drawString(0, 0, "SINCHAL POZO");
        display.drawString(0, 20, "Voltaje: " + String(nivelVoltios, 2) + " V");
        display.drawString(0, 30, "Altura: " + String(alturaActual) + " cm");
        display.display();

        // Crea el mensaje con la última cifra de la altura
        sprintf(txpacket, "%d", alturaActual);

        Serial.printf("\r\nsending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));

        Radio.Send((uint8_t *)txpacket, strlen(txpacket)); // Enviar por LoRa
        lora_idle = false;
    }
    Radio.IrqProcess();
}

void OnTxDone(void) {
    Serial.println("TX done......");
    lora_idle = true;
}

void OnTxTimeout(void) {
    Radio.Sleep();
    Serial.println("TX Timeout......");
    lora_idle = true;
}
