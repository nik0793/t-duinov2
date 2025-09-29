// ====== Robot por Bluetooth clasico (ESP32) ======
// Comandos por BT: 'F' (adelante), 'B' (atrás), 'L' (izquierda), 'R' (derecha), 'S' (stop)

#include <Wire.h>
#include <U8g2lib.h>
#include "BluetoothSerial.h"

// --------------------- Display ---------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

// --------------------- Pines motores ----------------
const int motor1a = 12;
const int motor1b = 27;
const int motor2a = 26;
const int motor2b = 33;
const int enableA = 14;  // PWM canal A
const int enableB = 25;  // PWM canal B

// --------------------- PWM (LEDC) -------------------
const int PWM_CH_A = 0;
const int PWM_CH_B = 1;
const int PWM_FREQ = 20000;   // 20 kHz
const int PWM_RES  = 8;       // 8 bits (0-255)

// --------------------- Otros pines -------------------
const int LED_PIN = 4;

// --------------------- Estados ----------------------
BluetoothSerial SerialBT;

// ===== Utilidades =====
inline void pwmWriteA(uint8_t v){ ledcWrite(PWM_CH_A, v); }
inline void pwmWriteB(uint8_t v){ ledcWrite(PWM_CH_B, v); }

void stopMotors() {
  digitalWrite(motor1a, LOW);
  digitalWrite(motor1b, LOW);
  digitalWrite(motor2a, LOW);
  digitalWrite(motor2b, LOW);
  pwmWriteA(0);
  pwmWriteB(0);
}

void Titilar () {
  int t = 80;
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, HIGH); delay(t);
    digitalWrite(LED_PIN, LOW);  delay(t);
  }
  digitalWrite(LED_PIN, HIGH);
}

// ===== Pantallas =====
void drawBTWaiting() {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(3, 10, "ESPERANDO CONEX");
  display.setFont(u8g2_font_ncenB14_tr);
  display.drawStr(3, 30, " CONECTAR");
  display.drawStr(3, 50, " BLUETOOTH");
  display.sendBuffer();
}

void setup() {
  Titilar();

  Serial.begin(115200);
  delay(50);

  display.begin();
  display.clearBuffer();

  // Pines motores
  pinMode(motor1a, OUTPUT);
  pinMode(motor1b, OUTPUT);
  pinMode(motor2a, OUTPUT);
  pinMode(motor2b, OUTPUT);

  // PWM nativo en enable A/B
  ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(enableA, PWM_CH_A);
  ledcAttachPin(enableB, PWM_CH_B);
  stopMotors();

  // Bluetooth clasico
  SerialBT.begin("Yo_Robot");  // Nombre visible por BT
  Serial.println("[BT] Listo. Nombre: Yo_Robot");
  drawBTWaiting();
}

void loop() {
  // Procesa comandos por Bluetooth
  if (SerialBT.available()) {
    char c = SerialBT.read();
    Serial.write(c);  // eco a monitor serie

    switch (c) {
      case 'F': // Adelante
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        pwmWriteA(255);
        pwmWriteB(255);
        break;

      case 'B': // Atrás
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        pwmWriteA(255);
        pwmWriteB(255);
        break;

      case 'L': // Izquierda (giro sobre el lugar)
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        pwmWriteA(255);
        pwmWriteB(255);
        break;

      case 'R': // Derecha (giro sobre el lugar)
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        pwmWriteA(255);
        pwmWriteB(255);
        break;

      case 'S': // Stop
      default:
        stopMotors();
        break;
    }
  }

  // Cede CPU al RTOS (baja latencia)
  delay(1);
}
