import paho.mqtt.client as mqtt
import mysql.connector
from datetime import datetime

# Configuración de la base de datos
db_config = {
    "host": "localhost",
    "user": "root",
    "password": "tu_contraseña",  
    "database": "sensores"
}

# Función para insertar según el tipo de sensor
def insertar_dato(tipo, valor):
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()

        if tipo == "nivel":
            sql = "INSERT INTO datos (timestamp, nivel, presion) VALUES (NOW(), %s, %s)"
            cursor.execute(sql, (valor, None))
        elif tipo == "presion":
            sql = "INSERT INTO datos (timestamp, nivel, presion) VALUES (NOW(), %s, %s)"
            cursor.execute(sql, (None, valor))

        conn.commit()
        cursor.close()
        conn.close()
        print(f"[{datetime.now()}] Insertado {tipo}: {valor}")
    except Exception as e:
        print(f"Error al insertar en la base de datos: {e}")

# Callback al recibir mensaje
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        valor = float(payload)
        if msg.topic == "nivel":
            insertar_dato("nivel", valor)
        elif msg.topic == "presion":
            insertar_dato("presion", valor)
    except ValueError:
        print(f"Valor no válido recibido: {msg.payload}")

# Configurar cliente MQTT
client = mqtt.Client()
client.on_message = on_message

client.connect("localhost", 1883, 60)
client.subscribe("nivel")
client.subscribe("presion")

print("Escuchando topics: nivel y presion...")
client.loop_forever()
