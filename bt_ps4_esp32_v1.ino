//andando ok bt y ps4
//esptool --chip esp32 --port COM4 erase_flash
#include "BluetoothSerial.h"
#include <ps4.h>
#include <ps4_int.h>
#include <PS4Controller.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);
BluetoothSerial SerialBT;
int ps4 = 0;
int bt = 0;;
int motor1a = 2;
int motor1b = 4;
int motor2a = 16;
int motor2b = 17;
int enableA = 15;
int enableB = 5;
int impr_pant = 0;
int led=13;

void setup() {
   Titilar();

Serial.print("prendo el led");
  display.begin();
  display.clearBuffer();

  // Configurar pines como salida
  pinMode(motor1a, OUTPUT);
  pinMode(motor1b, OUTPUT);
  pinMode(motor2a, OUTPUT);
  pinMode(motor2b, OUTPUT);
  pinMode(enableA, OUTPUT);
  pinMode(enableB, OUTPUT);


  // Inicializar motores apagados
  stopMotors();

  pinMode(5, INPUT); // Configura el pin 5 como entrada
  Serial.begin(115200);
  delay(100);        // Pequeña espera por estabilidad

  if (digitalRead(5) == HIGH) {
    bt = 1;
    ps4 = 0;
    btconfig();
  } else {
    ps4config();
    bt = 0;
    ps4 = 1;
  }
}

void loop() {
  digitalWrite(led, HIGH);
  if (bt == 1 && ps4 == 0) {
    btrutina();
  }
  else {
    ps4rutina();
  }
}

void ps4rutina() {
  if (PS4.isConnected()) {
    if (impr_pant == 0) {
      display.clearBuffer();
      display.setFont(u8g2_font_ncenB08_tr); // Fuente estándar
      display.drawStr(3, 10, "CONECTADO A");
      display.drawStr(3, 30, "      Mando de");
      display.setFont(u8g2_font_ncenB14_tr); // Fuente más grande
      display.drawStr(3, 50, "      PS4");
      display.sendBuffer(); // Muestra en pantalla
      impr_pant = 1;
    }
    bool isMoving = false;

    int stickX = PS4.LStickX(); // -128 a 127
    int stickY = PS4.LStickY();

    // Normalizar velocidad de -128 a 127 => 0 a 255
    int speedX = map(abs(stickX), 0, 128, 0, 255);
    int speedY = map(abs(stickY), 0, 128, 0, 255);

    // Filtrar interferencias entre ejes
    if (speedX > 127) stickY = 0;
    if (speedY > 127) stickX = 0;

    if (abs(stickX) > 30) {
      if (stickX > 30) {
        // Gira a la derecha
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        analogWrite(enableA, speedX);
        analogWrite(enableB, speedX);
        Serial.printf("Giro derecha: %d\n", stickX);
      } else {
        // Gira a la izquierda
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        analogWrite(enableA, speedX);
        analogWrite(enableB, speedX);
        Serial.printf("Giro izquierda: %d\n", stickX);
      }
      isMoving = true;
    }

    if (abs(stickY) > 30) {
      if (stickY < -30) {
        // Adelante
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        analogWrite(enableA, speedY);
        analogWrite(enableB, speedY);
        Serial.printf("Adelante: %d\n", stickY);
      } else {
        // Atrás
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        analogWrite(enableA, speedY);
        analogWrite(enableB, speedY);
        Serial.printf("Atrás: %d\n", stickY);
      }
      isMoving = true;
    }

    // Si no se mueve con stick, revisar botones
    if (!isMoving) {
      if (PS4.Up()) {
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        isMoving = true;
      } else if (PS4.Down()) {
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        isMoving = true;
      } else if (PS4.Left()) {
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        isMoving = true;
      } else if (PS4.Right()) {
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        isMoving = true;
      }
    }

    // Si no hay movimiento, detener motores
    if (!isMoving) {
      stopMotors();
    }
  } else {
    // Si pierde conexión, muestra pantalla de espera y resetea impr_pant
    if (impr_pant == 1) {
      ps4config();
      impr_pant = 0;
    }
    stopMotors();
  }
}void btrutina() {
  if (SerialBT.available()) {
    char c = SerialBT.read();
    Serial.write(c);  // Mostrar en el monitor serial

    switch (c) {
      case 'F': // Adelante
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        break;

      case 'B': // Atrás
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        break;

      case 'L': // Izquierda
        digitalWrite(motor1a, HIGH);
        digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, HIGH);
        digitalWrite(motor2b, LOW);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        break;

      case 'R': // Derecha
        digitalWrite(motor1a, LOW);
        digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, LOW);
        digitalWrite(motor2b, HIGH);
        analogWrite(enableA, 255);
        analogWrite(enableB, 255);
        break;

      case 'S': // Detener motores
        stopMotors();
        break;

      default:
        // Ignorar cualquier otro carácter
        break;
    }
  }
}

void btconfig() {
  display.setFont(u8g2_font_ncenB08_tr); // Fuente estándar
  display.drawStr(3, 10, "ESPERANDO CONEX");
  display.setFont(u8g2_font_ncenB14_tr); // Fuente más grande
  display.drawStr(3, 30, " CONECTAR");
  display.drawStr(3, 50, " BLUETOOTH");
  display.sendBuffer(); // Muestra en pantalla

  Serial.print("entre a btconfig");
  SerialBT.begin("robot1");        // Nombre del Bluetooth del ESP32
  Serial.println("Bluetooth listo. Puedes emparejar con 'ESP32_BT'");
}

void ps4config() {
  display.setFont(u8g2_font_ncenB08_tr); // Fuente estándar
  display.drawStr(3, 10, "ESPERANDO CONEX");
  display.drawStr(3, 30, "PRENDE EL MANDO ");
  display.setFont(u8g2_font_ncenB14_tr); // Fuente más grande
  display.drawStr(3, 50, "  DE   PS4");
  display.sendBuffer(); // Muestra en pantalla

  Serial.print("entre a ps4config");
  PS4.begin("ac:89:95:16:d5:ea"); // Reemplaza con tu MAC si es diferente
  Serial.begin(115200);
  Serial.println("Ready.");
}

void stopMotors() {
  digitalWrite(motor1a, LOW);
  digitalWrite(motor1b, LOW);
  digitalWrite(motor2a, LOW);
  digitalWrite(motor2b, LOW);
  analogWrite(enableA, 0);
  analogWrite(enableB, 0);
}
void Titilar (){
 int t=100;
    pinMode(led, OUTPUT);
 for(int i=1; i<8; i ++){
   digitalWrite(led, HIGH);
   delay(t);
   digitalWrite(led, LOW);
   delay(t);
 }
 digitalWrite(led, HIGH);
}
