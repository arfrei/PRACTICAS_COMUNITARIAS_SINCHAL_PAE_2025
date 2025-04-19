# PRACTICAS_COMUNITARIAS_SINCHAL_PAE_2025
En este repositario se adjuntan todos los códigos utilizados en el proyecto de prácticas comunitarias.

# INTRODUCCION 
La Junta Administrativa de Agua Potable Regional de Valdivia (JAAPRV) en Sinchal abastece a 
cinco comunidades, asegurando el suministro de un recurso esencial. Para mejorar la eficiencia del 
sistema y minimizar riesgos como el desabastecimiento y el desperdicio de agua, se ha 
implementado un sistema de monitoreo en el tanque reservorio de la localidad. 

Este sistema cuenta con un transmisor de nivel (4-20mA), cuya información es recolectada por una LoRa WiFi 32 (V3). 
La alimentación del sistema proviene de una batería de 12V recargada mediante energía solar, lo que permite su funcionamiento autónomo. 
Sin embargo, para optimizar su desempeño, es necesario mejorar el acceso a la información del tanque por parte de 
los miembros de la Junta y establecer una comunicación eficiente entre el tanque reservorio y el edificio de la Junta del Agua. 
Para ello, es fundamental adoptar tecnologías de monitoreo en tiempo real, que permitan una 
supervisión precisa y confiable de los niveles de almacenamiento, el flujo de distribución y 
posibles fugas. La integración de estos sistemas facilitará una toma de decisiones informada, 
optimizando el uso del agua y asegurando su disponibilidad continua para la comunidad. 

# SOLUCIONES
En este repositario iré subiendo ciertos códigos para las distintas soluciones.

1: Comunicación LoRa, ThingSpeak y Comunicación Servidor Flask-SQL Server

![TRANSMISOR SINCHAL](https://github.com/user-attachments/assets/7dcb656f-8584-4494-b649-321fc42216ef)
![RECEPTOR SINCHAL](https://github.com/user-attachments/assets/cb6b00aa-6bd9-474f-b10c-0e6ba387c183)
![THINGSPEAK SINCHAL NIVEL](https://github.com/user-attachments/assets/0b18c6d7-af1d-4256-92ac-64b2604af9dc)
![SERVIDOR FLASK SINCHAL](https://github.com/user-attachments/assets/333e7dd2-7270-44f1-8bef-fbd0ddbb0d47)
![BASE DE DATOS SINCHAL](https://github.com/user-attachments/assets/7d74d9d8-dfd4-4369-b03e-20e6ae54e61e)

Se añadió el sensor de presión de la junta de agua al Dashboard:

![SENSOR PRESION](https://github.com/user-attachments/assets/6f618200-97e0-4494-a8c8-264e025d929d)
![DASHBOARD THINGSPEAK](https://github.com/user-attachments/assets/66219a46-8c75-408c-b532-3f2d4171e95a)

2: Comunicación LoRa, MySQL y Grafana
