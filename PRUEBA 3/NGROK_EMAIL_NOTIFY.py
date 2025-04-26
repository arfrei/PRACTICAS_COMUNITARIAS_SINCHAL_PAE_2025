# ngrok_email_notify.py
import time
import requests
import smtplib
from email.mime.text import MIMEText

NGROK_API = "http://localhost:4040/api/tunnels"
DESTINATARIO = "tucorreo@ejemplo.com" #correo de mantenimiento de la junta

GMAIL_USER = "tunombre@gmail.com" #correo de mantenimiento de la junta
GMAIL_PASS = "tu_contraseña_de_app" #contrasena de aplicacion 

def get_ngrok_url():
    try:
        r = requests.get(NGROK_API)
        tunnels = r.json()["tunnels"]
        for t in tunnels:
            if t["proto"] == "https":
                return t["public_url"]
    except:
        pass
    return None

def send_email(url):
    msg = MIMEText(f"Tu URL pública de ngrok es:\n\n{url}")
    msg["Subject"] = "URL de ngrok"
    msg["From"] = GMAIL_USER
    msg["To"] = DESTINATARIO

    try:
        with smtplib.SMTP_SSL("smtp.gmail.com", 465) as server:
            server.login(GMAIL_USER, GMAIL_PASS)
            server.send_message(msg)
        print("Correo enviado exitosamente.")
    except Exception as e:
        print("Error al enviar correo:", e)

def main():
    print("Esperando ngrok...")
    url = None
    while not url:
        url = get_ngrok_url()
        time.sleep(2)

    print("URL encontrada:", url)
    send_email(url)

if __name__ == "__main__":
    main()
