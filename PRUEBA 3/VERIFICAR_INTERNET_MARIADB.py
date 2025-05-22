import mysql.connector
import time
import socket
from datetime import datetime

# Configuración de la base de datos
db_config = {
    "host": "localhost",
    "user": "root",
    "password": "sinchaldatosdb2025",  
    "database": "sensores"
}

def insertar_dato_falla():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()
        sql = "INSERT INTO datos (timestamp, nivel, presion) VALUES (NOW(), %s, %s)"
        cursor.execute(sql, (-45, -45))  
        conn.commit()
        cursor.close()
        conn.close()
        print(f"[{datetime.now()}] Sin Internet. Insertado -45 en nivel y presion.")
    except Exception as e:
        print("Error al insertar en la base de datos:", e)

def hay_internet():
    try:
        socket.create_connection(("1.1.1.1", 80), timeout=2)
        return True
    except OSError:
        return False

# Evitar múltiples inserciones
insertado = False

# Bucle principal
while True:
    if not hay_internet():
        if not insertado:
            insertar_dato_falla()
            insertado = True
    else:
        if insertado:
            print(f"[{datetime.now()}] Internet restaurado.")
        insertado = False

    time.sleep(60)  # Verifica cada 1 min
