import mysql.connector
from datetime import datetime

# Configuración de la base de datos
db_config = {
    "host": "localhost",
    "user": "root",
    "password": "tu_contraseña",  #contraseña real
    "database": "sensores"
}

def insertar_inicio():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()
        sql = "INSERT INTO datos (timestamp, nivel, presion) VALUES (NOW(), %s, %s)"
        cursor.execute(sql, (-66, -66))
        conn.commit()
        cursor.close()
        conn.close()
        print(f"[{datetime.now()}] Insertado valor de reinicio (-66, -66)")
    except Exception as e:
        print(f"Error al insertar en la base de datos: {e}")

if __name__ == "__main__":
    insertar_inicio()
