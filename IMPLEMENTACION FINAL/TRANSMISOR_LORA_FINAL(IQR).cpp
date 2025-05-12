// Librerías necesarias
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"

// OLED
#ifdef WIRELESS_STICK_V3
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_64_32, RST_OLED);
#else
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
#endif

// Sensor
#define LEVEL_SENSOR 4

// LoRa config
#define RF_FREQUENCY           915000000
#define TX_OUTPUT_POWER        21
#define LORA_BANDWIDTH         0
#define LORA_SPREADING_FACTOR  11
#define LORA_CODINGRATE        1
#define LORA_PREAMBLE_LENGTH   8
#define LORA_SYMBOL_TIMEOUT    0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON   false
#define TX_TIMEOUT_VALUE       8000
#define RX_TIMEOUT_VALUE       3000
#define BUFFER_SIZE            10

// Variables físicas
int alturaMaxima = 500;
float valorsinsumergir = 0.60;
int portValue;
float nivelVoltios;
float diferenciaVoltaje;
float valorMaximoAbsoluto;
int alturaActual;

// IQR y Lecturas
#define NUM_LECTURAS 6
int lecturas[NUM_LECTURAS];

// LoRa
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
bool lora_idle = true;
static RadioEvents_t RadioEvents;

void OnTxDone(void);
void OnTxTimeout(void);

// === FUNCIONES AUXILIARES ===

// Ordenar un array (Bubble sort)
void ordenarArray(int *arr, int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}

// Calcular media de valores dentro del IQR
float calcularIQRMedia(int *arr, int n) {
    ordenarArray(arr, n);
    float Q1 = (arr[1] + arr[2]) / 2.0;
    float Q3 = (arr[3] + arr[4]) / 2.0;
    float IQR = Q3 - Q1;
    float LI = Q1 - 1.5 * IQR;
    float LS = Q3 + 1.5 * IQR;

    int suma = 0;
    int count = 0;

    for (int i = 0; i < n; i++) {
        if (arr[i] >= LI && arr[i] <= LS) {
            suma += arr[i];
            count++;
        }
    }

    if (count == 0) return -1; // evitar división por cero
    return ((float)suma / count);
}

// === SETUP ===
void setup() {
    Serial.begin(115200);
    Mcu.begin();

    display.init();
    display.clear();
    display.display();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // LoRa
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
}

// === LOOP PRINCIPAL ===
void loop() {
    if (lora_idle) {
        for (int i = 0; i < NUM_LECTURAS; i++) {
            portValue = analogRead(LEVEL_SENSOR);
            nivelVoltios = (portValue * 3.3) / 4095.0;
            diferenciaVoltaje = (nivelVoltios - valorsinsumergir);
            valorMaximoAbsoluto = 3.0 - valorsinsumergir;
            alturaActual = ((diferenciaVoltaje * alturaMaxima) / valorMaximoAbsoluto) + 2;

            lecturas[i] = alturaActual;

            // Mostrar en OLED
            display.clear();
            display.drawString(0, 0, "SINCHAL POZO");
            display.drawString(0, 20, "Voltaje: " + String(nivelVoltios, 2) + " V");
            display.drawString(0, 30, "Altura: " + String(alturaActual) + " cm");
            display.display();

            delay(10000); // 10 segundos entre lecturas
        }

        float mediaFiltrada = calcularIQRMedia(lecturas, NUM_LECTURAS);
        int mediaTruncada = int(mediaFiltrada);

        display.clear();
        display.drawString(0, 0, "SINCHAL POZO");
        display.drawString(0, 20, "Altura Promedio: "+ String(mediaTruncada) + " cm");
        display.display();

        sprintf(txpacket, "%d", mediaTruncada);
        Serial.printf("\r\nEnviando: \"%s\", length %d\n", txpacket, strlen(txpacket));
        Radio.Send((uint8_t *)txpacket, strlen(txpacket));
        lora_idle = false;
        delay(5000);
    }

    Radio.IrqProcess();
}

void OnTxDone(void) {
    Serial.println("TX done...");
    lora_idle = true;
}

void OnTxTimeout(void) {
    Radio.Sleep();
    Serial.println("TX Timeout...");
    lora_idle = true;
}
