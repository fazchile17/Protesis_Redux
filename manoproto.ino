#include <Servo.h>

// ===== CONFIGURACIÓN DE PINES =====
#define EMG_PIN A0        // Pin analógico para la señal EMG
#define SERVO_PIN 9       // Pin digital para el servo
#define LO_PLUS 2         // Pin para detección de electrodos (opcional)
#define LO_MINUS 3        // Pin para detección de electrodos (opcional)

// ===== CONFIGURACIÓN DE PARÁMETROS =====
#define SAMPLE_RATE 1000  // Frecuencia de muestreo en Hz
#define WINDOW_SIZE 10    // Tamaño de la ventana para la media móvil
#define THRESHOLD 150     // Umbral para detectar contracción (ajustar según necesidad)
#define OFFSET 512        // Valor de offset para centrar la señal (2.5V = 512 en ADC de 10 bits)

// ===== VARIABLES GLOBALES =====
Servo emgServo;           // Objeto para controlar el servo
int rawValue;             // Valor crudo del sensor
int filteredValue;        // Valor filtrado
int window[WINDOW_SIZE];  // Array para la media móvil
int windowIndex = 0;      // Índice actual de la ventana
bool isContracted = false;// Estado de contracción
bool manoAbierta = true;  // Estado de la mano (true = abierta, false = cerrada)
bool ultimoEstado = false;// Último estado de contracción detectado

// ===== FUNCIONES DE FILTRADO =====
void initWindow() {
  for(int i = 0; i < WINDOW_SIZE; i++) {
    window[i] = 0;
  }
}

int getMovingAverage(int newValue) {
  window[windowIndex] = newValue;
  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  
  long sum = 0;
  for(int i = 0; i < WINDOW_SIZE; i++) {
    sum += window[i];
  }
  return sum / WINDOW_SIZE;
}

// ===== FUNCIONES DE ANÁLISIS =====
bool detectContraction(int value) {
  return value > THRESHOLD;
}

// ===== FUNCIONES DE ACTUACIÓN =====
void updateServo(bool contracted) {
  if(contracted && !ultimoEstado) {  // Solo cambia cuando hay un flanco ascendente
    manoAbierta = !manoAbierta;      // Alterna el estado de la mano
    if(manoAbierta) {
      emgServo.write(0);    // Abre la mano
    } else {
      emgServo.write(180);  // Cierra la mano
    }
  }
  ultimoEstado = contracted;
}

void printStatus(bool contracted, int value) {
  Serial.print("Valor filtrado: ");
  Serial.print(value);
  Serial.print(" | Estado: ");
  Serial.print(contracted ? "Contracción detectada" : "Reposo");
  Serial.print(" | Mano: ");
  Serial.println(manoAbierta ? "Abierta" : "Cerrada");
}

void setup() {
  Serial.begin(115200);
  
  pinMode(EMG_PIN, INPUT);
  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);
  
  emgServo.attach(SERVO_PIN);
  emgServo.write(0);  // Posición inicial (mano abierta)
  
  initWindow();
  
  Serial.println("Sistema EMG iniciado");
  Serial.println("Esperando señal...");
}

void loop() {
  if(digitalRead(LO_PLUS) == HIGH || digitalRead(LO_MINUS) == HIGH) {
    Serial.println("¡Electrodos desconectados!");
    delay(100);
    return;
  }
  
  rawValue = analogRead(EMG_PIN);
  rawValue = rawValue - OFFSET;
  rawValue = abs(rawValue);
  filteredValue = getMovingAverage(rawValue);
  
  isContracted = detectContraction(filteredValue);
  updateServo(isContracted);
  
  printStatus(isContracted, filteredValue);
  
  delay(1000/SAMPLE_RATE);
}
