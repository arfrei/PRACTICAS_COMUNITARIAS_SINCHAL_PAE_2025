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

# Función para insertar en la base de datos
def insertar_dato(valor):
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()
        sql = "INSERT INTO datos (timestamp, nivel, presion) VALUES (NOW(), %s, %s)"
        cursor.execute(sql, (valor, None))  # presion queda como NULL
        conn.commit()
        cursor.close()
        conn.close()
        print(f"[{datetime.now()}] Insertado valor: {valor}")
    except Exception as e:
        print(f"Error al insertar en la base de datos: {e}")

# Callback cuando se recibe un mensaje
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        valor = float(payload)
        insertar_dato(valor)
    except ValueError:
        print(f"Dato recibido no es válido: {msg.payload}")

# Configurar cliente MQTT
client = mqtt.Client()
client.on_message = on_message

client.connect("localhost", 1883, 60)
client.subscribe("sensor/altura")

print("Esperando mensajes MQTT en 'sensor/altura'...")
client.loop_forever()
