// Archivo para crear una tarea en el task scheduler de windows (Ejecuta automaticamente el servidor)
@echo off
cd /d C:\Users\Usuario\Nombre_Carpeta
call env\Scripts\activate
python LORALTURA.py
