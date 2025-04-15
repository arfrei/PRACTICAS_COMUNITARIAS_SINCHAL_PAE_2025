// CODIGO EN ENTORNO VIRTUAL PARA GENERAR SERVIDOR FLASK (COMUNICACION CON SQL SERVER)
from flask import Flask, request
import pyodbc

app = Flask(__name__)

# Cadena de conexión con Driver 18 y autenticación de Windows
conn_str = (
    "Driver={ODBC Driver 18 for SQL Server};"
    "Server=;"  # Cambia según tu configuración
    "Database=LORA;"
    "Trusted_Connection=yes;"
    "Encrypt=optional;"
    "TrustServerCertificate=yes;"
)

@app.route('/guardar_altura', methods=['POST'])
def guardar_altura():
    altura = request.form.get('altura')
    if altura is None:
        return "Falta el dato 'altura'", 400

    try:
        altura = int(altura)
        with pyodbc.connect(conn_str) as conn:
            cursor = conn.cursor()
            cursor.execute("INSERT INTO Lecturas (Altura) VALUES (?)", altura)
            conn.commit()
        return "Dato guardado con éxito", 200
    except Exception as e:
        return f"Error al guardar: {str(e)}", 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
