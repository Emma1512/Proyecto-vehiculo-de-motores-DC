/*
  Trabajo de Laboratorio N°1 - Auto con ESP32
  ---------------------------------------------
  - Sensor ultrasónico HC-SR04: mide distancia frontal.
  - Joystick (reemplaza al potenciómetro): entrada analógica que
    regula la velocidad de avance.
  - Puente H doble L298N: controla 2 motores (izquierdo/derecho) por PWM.
  - Comunicación serie: reporta periódicamente distancia, ADC, PWM y estado.

  Nota para quien viene de C++ "de escritorio": Arduino/ESP32 no tiene
  una función main() que vos escribas. El framework llama automáticamente
  a setup() UNA vez al arrancar, y después llama a loop() en un bucle
  infinito. Es casi como si existiera un main oculto que hiciera:
      setup();
      while (true) { loop(); }
*/

// ==== PINES del sensor y joystick ====
const int TRIG_PIN = 2;
const int ECHO_PIN = 4;

const int VRY_PIN = 35;   // eje Y del joystick -> lo usamos como "velocidad"
const int VRX_PIN = 34;   // eje X (queda disponible)
const int SW_PIN  = 33;   // botón del joystick (queda disponible)

// ==== PINES puente H L298N ====
// Canal A - lado izquierdo
const int ENA = 25; // PWM velocidad motor izquierdo
const int IN1 = 26;
const int IN2 = 27;

// Canal B - lado derecho
const int ENB = 13; // PWM velocidad motor derecho
const int IN3 = 14;
const int IN4 = 32;

// ==== PARÁMETROS DEL SISTEMA ====
const float DISTANCIA_LIMITE_CM = 15.0;
const unsigned long TIEMPO_MAX_ECO_US = 25000UL; // timeout de pulseIn (~4 m)
const unsigned long INTERVALO_ENVIO_MS = 300UL;  // cada cuánto se reporta por serie

// Control de dirección por joystick (mejora agregada más allá de la consigna
// original, que solo pedía velocidad). El joystick en reposo no da
// exactamente 2048 (mitad matemática de 0-4095), varía un poco de joystick
// a joystick -- si notás que el auto se mueve solo con el stick soltado,
// medí el valor real en reposo con el Monitor Serie y poné ese número acá.
const int ADC_CENTRO = 2655; // medido en reposo con este joystick puntual
const int ADC_ZONA_MUERTA = 300; // margen alrededor del centro que se considera "soltado"

const unsigned long TIEMPO_RETROCESO_MS = 400UL;
const unsigned long TIEMPO_GIRO_MS = 500UL;
const int VELOCIDAD_ESQUIVE = 200; // velocidad fija para retroceder/girar; no depende del joystick porque acá manda la seguridad, no el usuario

// ==== ESTADOS DEL SISTEMA ====
// enum: como un "tipo con valores fijos" (similar a un enum class de C++,
// pero más simple, sin el "class" y con conversión implícita a int).
enum EstadoAuto {
  AVANZANDO,
  RETROCEDIENDO,
  GIRANDO,
  SEGURO // el sensor falla o mide algo inválido -> el auto se detiene
};

EstadoAuto estadoActual = SEGURO;
unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  detenerMotores();
}

void loop() {
  float distancia = medirDistanciaCM();

  // analogRead en ESP32 devuelve 0-4095 (ADC de 12 bits),
  // a diferencia del Arduino UNO clásico que devuelve 0-1023 (10 bits).
  int lecturaADC = analogRead(VRY_PIN);
  int pwm = 0; // se calcula más abajo según corresponda

  if (distancia < 0) {
    // Requisito 6: si el sensor no responde o la medición es inválida,
    // el sistema adopta un estado seguro (motores detenidos).
    estadoActual = SEGURO;
    detenerMotores();
    pwm = 0;
  } else if (distancia > DISTANCIA_LIMITE_CM) {
    // Camino libre: acá el joystick manda, tanto en velocidad como en
    // dirección. Hacia arriba (ADC bajo) = adelante; hacia abajo
    // (ADC alto) = atrás; centrado = detenido.
    int diferencia = lecturaADC - ADC_CENTRO;

    if (diferencia < -ADC_ZONA_MUERTA) {
      pwm = calcularVelocidad(lecturaADC);
      estadoActual = AVANZANDO;
      avanzar(pwm);
    } else if (diferencia > ADC_ZONA_MUERTA) {
      pwm = calcularVelocidad(lecturaADC);
      estadoActual = RETROCEDIENDO;
      retroceder(pwm);
    } else {
      // joystick soltado, cerca del centro: detenido
      pwm = 0;
      estadoActual = AVANZANDO;
      detenerMotores();
    }
  } else {
    // Maniobra elegida: retroceder un poco y después girar.
    // OJO: delay() acá bloquea el loop() durante la maniobra.
    // Para un TP está bien, pero si más adelante querés que el auto
    // "reaccione" mientras gira, se puede reescribir con millis()
    // en vez de delay() (avisame si querés esa versión).
    estadoActual = RETROCEDIENDO;
    pwm = VELOCIDAD_ESQUIVE; // para que el reporte serie muestre el valor real aplicado
    retroceder(pwm);
    delay(TIEMPO_RETROCESO_MS);

    estadoActual = GIRANDO;
    girar(pwm);
    delay(TIEMPO_GIRO_MS);
  }

  // Requisito 7: reporte periódico por serie.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    enviarEstado(distancia, lecturaADC, pwm);
    ultimoEnvio = millis();
  }
}

// ---------- MEDICIÓN DE DISTANCIA ----------
float medirDistanciaCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // pulseIn espera un pulso HIGH en ECHO_PIN y devuelve cuánto duró (en us).
  // Si pasan TIEMPO_MAX_ECO_US sin que llegue el pulso, devuelve 0.
  unsigned long duracion = pulseIn(ECHO_PIN, HIGH, TIEMPO_MAX_ECO_US);

  if (duracion == 0) {
    return -1.0; // medición inválida: nunca llegó el eco
  }

  float cm = duracion / 58.0; // fórmula estándar del HC-SR04

  if (cm <= 0 || cm > 400) {
    return -1.0; // fuera del rango físico del sensor
  }

  return cm;
}

// ---------- JOYSTICK ----------
// Se decidio 2 velocidades fijas, no una escala continua:
// cualquier empuje moderado (fuera de la zona muerta) = 50%,
// empujado casi hasta el fondo = 100%.
const int VELOCIDAD_MEDIA = 127;   // ~50% de 255
const int VELOCIDAD_MAXIMA = 255;  // 100%
const float PORCENTAJE_PARA_TOPE = 0.85; // a partir de qué % del recorrido se considera "a fondo"

int calcularVelocidad(int adc) {
  int distanciaAlCentro = abs(adc - ADC_CENTRO) - ADC_ZONA_MUERTA;
  if (distanciaAlCentro <= 0) return 0; // dentro de la zona muerta: no debería llegar hasta acá, pero por las dudas

  // Mismo ajuste de siempre: como el centro real no está a mitad de
  // camino entre 0 y 4095, usamos el recorrido más corto de los dos
  // lados como referencia de "a fondo", para que sea parejo en ambos
  // sentidos.
  int recorridoHaciaArriba = ADC_CENTRO;
  int recorridoHaciaAbajo = 4095 - ADC_CENTRO;
  int recorridoMaximo = min(recorridoHaciaArriba, recorridoHaciaAbajo) - ADC_ZONA_MUERTA;

  int umbralTope = recorridoMaximo * PORCENTAJE_PARA_TOPE;

  if (distanciaAlCentro >= umbralTope) {
    return VELOCIDAD_MAXIMA;
  } else {
    return VELOCIDAD_MEDIA;
  }
}

// ---------- CONTROL DE MOTORES ----------
void avanzar(int velocidad) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void retroceder(int velocidad) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void girar(int velocidad) {
  // Giro en el lugar: una rueda gira "adelante" y la otra "atrás".
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, velocidad);
  analogWrite(ENB, velocidad);
}

void detenerMotores() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ---------- COMUNICACIÓN SERIE ----------
void enviarEstado(float distancia, int adc, int pwm) {
  Serial.print("Distancia: ");
  if (distancia < 0) Serial.print("INVALIDA");
  else Serial.print(distancia);
  Serial.print(" cm | ADC: ");
  Serial.print(adc);
  Serial.print(" | PWM: ");
  Serial.print(pwm);
  Serial.print(" | Estado: ");
  Serial.println(nombreEstado(estadoActual));
}

const char* nombreEstado(EstadoAuto e) {
  switch (e) {
    case AVANZANDO:     return "AVANZANDO";
    case RETROCEDIENDO: return "RETROCEDIENDO";
    case GIRANDO:        return "GIRANDO";
    case SEGURO:          return "SEGURO";
    default:              return "DESCONOCIDO";
  }
}
