# 🚗 Auto con ESP32: control por joystick y detección de obstáculos

![Plataforma](https://img.shields.io/badge/plataforma-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![Motores](https://img.shields.io/badge/motores-DC%20%2B%20L298N-orange)
![Estado](https://img.shields.io/badge/estado-funcional-brightgreen)
![Licencia](https://img.shields.io/badge/licencia-[completar]-lightgrey)

Vehículo con dos motores DC controlado por un ESP32. La velocidad y el sentido de marcha se manejan con un joystick analógico, y un sensor ultrasónico HC-SR04 mide la distancia frontal para esquivar obstáculos de forma automática. El estado del sistema se reporta por puerto serie.

Trabajo de Laboratorio N°1 de Arquitectura y Sistemas de Elaboración de Datos II (ASED II).

Proyecto colaborativo. Repositorio original: [Emma1512/Proyecto-vehiculo-de-motores-DC](https://github.com/Emma1512/Proyecto-vehiculo-de-motores-DC)

---

## 📑 Tabla de contenidos

- [Descripción general](#-descripción-general)
- [Características](#-características)
- [Funcionamiento](#-funcionamiento)
- [Hardware](#-hardware)
- [Esquema de conexiones](#-esquema-de-conexiones)
- [Software](#-software)
- [Instalación y puesta en marcha](#-instalación-y-puesta-en-marcha)
- [Parámetros configurables](#-parámetros-configurables)
- [Calibración del joystick](#-calibración-del-joystick)
- [Reporte por puerto serie](#-reporte-por-puerto-serie)
- [Demostración](#-demostración)
- [Estructura del repositorio](#-estructura-del-repositorio)
- [Mejoras futuras](#-mejoras-futuras)
- [Autores](#-autores)
- [Contexto académico](#-contexto-académico)
- [Licencia](#-licencia)

---

## 📋 Descripción general

El ESP32 lee dos entradas: la distancia frontal (HC-SR04) y la posición del eje Y del joystick. Con eso decide si el auto avanza, retrocede, gira o se detiene, y comanda dos motores DC a través de un puente H doble L298N con señales PWM.

## ✨ Características

- Control de velocidad y sentido (adelante / atrás / detenido) con el joystick.
- Dos velocidades fijas: media (50 %) y máxima (100 %, con el joystick casi a fondo).
- Zona muerta configurable para que el auto no se mueva con el joystick suelto.
- Esquive automático de obstáculos: retrocede y gira en el lugar.
- Estado seguro: si el sensor falla o da una medición inválida, los motores se detienen.
- Reporte periódico por puerto serie (distancia, valor del ADC, PWM y estado).

## ⚙️ Funcionamiento

El auto trabaja con cuatro estados:

| Estado | Cuándo ocurre |
|---|---|
| AVANZANDO | Camino libre y joystick hacia adelante |
| RETROCEDIENDO | Camino libre y joystick hacia atrás, o maniobra de esquive |
| GIRANDO | Segunda parte de la maniobra de esquive |
| SEGURO | El sensor no responde o la medición es inválida: motores detenidos |

Lógica de cada ciclo:

1. Se mide la distancia con el HC-SR04.
2. Si la medición es inválida, el auto pasa a estado SEGURO y se detiene.
3. Si la distancia es mayor a 15 cm, manda el joystick:
   - Hacia arriba: avanza.
   - Hacia abajo: retrocede.
   - Soltado (zona muerta): detenido.
4. Si la distancia es de 15 cm o menos, el auto esquiva: retrocede 400 ms y gira en el lugar 500 ms, a velocidad fija, sin importar la posición del joystick.

## 🧰 Hardware

| Componente | Cantidad | Función |
|---|---|---|
| ESP32 | 1 | Microcontrolador |
| Sensor ultrasónico HC-SR04 | 1 | Medición de distancia frontal |
| Joystick analógico | 1 | Control de velocidad y sentido |
| Puente H L298N | 1 | Manejo de los dos motores DC |
| Motores DC | 2 | Tracción (izquierdo y derecho) |
| Chasis y ruedas | 1 | Estructura del vehículo |
| Fuente de alimentación | [Fuente VCC 12V] | [completar: baterías para motores y alimentación del ESP32] |
| Divisor de tensión para ECHO | [Divisor de tensión de 5V a 3.3V] | El ECHO del HC-SR04 entrega 5 V y el ESP32 trabaja a 3.3 V |


| Elemento | Señal | Pin del ESP32 |
|---|---|---|
| HC-SR04 | TRIG | GPIO 2 |
| HC-SR04 | ECHO | GPIO 4 |
| Joystick | VRY (eje Y, velocidad y sentido) | GPIO 35 |
| Joystick | VRX (eje X, sin uso por ahora) | GPIO 34 |
| Joystick | SW (botón, sin uso por ahora) | GPIO 33 |

Puente H L298N:

| Motor | Señal | Pin del ESP32 |
|---|---|---|
| Izquierdo (canal A) | ENA (PWM) | GPIO 25 |
| Izquierdo (canal A) | IN1 | GPIO 26 |
| Izquierdo (canal A) | IN2 | GPIO 27 |
| Derecho (canal B) | ENB (PWM) | GPIO 13 |
| Derecho (canal B) | IN3 | GPIO 14 |
| Derecho (canal B) | IN4 | GPIO 32 |

Recordá unir las masas (GND) del ESP32, del L298N y de la alimentación de los motores.

## 💻 Software

- Lenguaje: C++ (Arduino)
- Entorno de desarrollo: Arduino IDE
- Placa: ESP32 (paquete de placas de Espressif). El código usa `analogWrite`, disponible en el core de ESP32 versión 3.x.
- Librerías externas: ninguna.

## 🚀 Instalación y puesta en marcha

1. Cloná el repositorio:

   ```bash
   git clone https://github.com/joelfusaro/Proyecto-Auto-a-control-remoto-sensor_Arquitectura-y-Sistemas-de-Elaboraci-n-de-Datos-II.git
   cd Proyecto-Auto-a-control-remoto-sensor_Arquitectura-y-Sistemas-de-Elaboraci-n-de-Datos-II
   ```

2. Instalá el soporte para ESP32 en el Arduino IDE (Gestor de placas).
3. Armá el circuito según el esquema de conexiones.
4. Seleccioná la placa y el puerto, compilá y cargá el programa.
5. Abrí el monitor serie a 115200 baudios para ver el estado del sistema.
6. Calibrá el centro del joystick (ver sección siguiente).

## 🎛️ Parámetros configurables

| Constante | Valor | Descripción |
|---|---|---|
| DISTANCIA_LIMITE_CM | 15.0 | Distancia a la que se activa el esquive |
| TIEMPO_MAX_ECO_US | 25000 | Tiempo máximo de espera del eco (aprox. 4 m) |
| INTERVALO_ENVIO_MS | 300 | Período del reporte por puerto serie |
| ADC_CENTRO | 2655 | Valor del ADC con el joystick en reposo |
| ADC_ZONA_MUERTA | 300 | Margen alrededor del centro considerado "soltado" |
| TIEMPO_RETROCESO_MS | 400 | Duración del retroceso en el esquive |
| TIEMPO_GIRO_MS | 500 | Duración del giro en el esquive |
| VELOCIDAD_ESQUIVE | 200 | PWM fijo durante el esquive |
| VELOCIDAD_MEDIA | 127 | PWM al mover el joystick con empuje moderado (50 %) |
| VELOCIDAD_MAXIMA | 255 | PWM con el joystick casi a fondo (100 %) |
| PORCENTAJE_PARA_TOPE | 0.85 | Fracción del recorrido a partir de la cual se aplica velocidad máxima |

## 🕹️ Calibración del joystick

El joystick en reposo no entrega exactamente la mitad del rango del ADC (2048 sobre 4095), y el valor varía de un joystick a otro. Si el auto se mueve solo con el joystick suelto:

1. Abrí el monitor serie y dejá el joystick sin tocar.
2. Anotá el valor de `ADC` que se reporta.
3. Reemplazá `ADC_CENTRO` en el código por ese valor (en este montaje, 2655) y volvé a cargar el programa.

## 📟 Reporte por puerto serie

Cada 300 ms se informa el estado, por ejemplo:

```
Distancia: 42.50 cm | ADC: 1200 | PWM: 127 | Estado: AVANZANDO
```

Si la medición es inválida, la distancia se muestra como `INVALIDA` y el estado pasa a `SEGURO`.

## 🎥 Demostración


## 📁 Estructura del repositorio

```
.
├── src/           # Código fuente (.ino)
├── docs/          # Informe, esquemas y capturas
├── imagenes/      # Fotos del montaje
└── README.md
```

[Ajustar a la estructura real del repositorio]

## 🔭 Mejoras futuras

- Reescribir las maniobras con `millis()` en lugar de `delay()`, para que el auto siga leyendo el sensor mientras esquiva.
- Usar el eje X del joystick para girar y el botón SW para alguna función extra.

## 👥 Autores

| Nombre | GitHub |
|---|---|
| Joel Fusaro | [@joelfusaro](https://github.com/joelfusaro) |
| Emma | [@Emma1512](https://github.com/Emma1512) |
| López Camila Marisol |
| Ponce Brisa |

## 🎓 Contexto académico

- Materia: Arquitectura y Sistemas de Elaboración de Datos II (ASED II)
- Institución: Universidad Nacional de Avellaneda
- Año: 2026
- Entrega: Trabajo de Laboratorio N°1

## 📄 Licencia

[Elegir una licencia, por ejemplo MIT, y reemplazar el badge de arriba, o eliminar esta sección]
