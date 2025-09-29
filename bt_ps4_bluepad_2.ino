#include <Bluepad32.h>
#include <Wire.h>
#include <U8g2lib.h>

// --------------------- Display ---------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

// --------------------- Pines motores ----------------
const int motor1a = 12;
const int motor1b = 27;
const int motor2a = 33;
const int motor2b = 26;
const int enableA = 14;
const int enableB = 25;

// --------------------- PWM (LEDC) -------------------
const int PWM_CH_A = 0;
const int PWM_CH_B = 1;
const int PWM_FREQ = 20000;   // 20 kHz (silencioso y rápido)
const int PWM_RES  = 8;       // 8 bits (0-255)

// --------------------- Otros pines -------------------
const int LED_PIN      = 4;
const int PAIR_BTN_PIN = 0;   // BOOT

// --------------------- Estados ----------------------
int impr_pant = 0;
ControllerPtr controllers[BP32_MAX_GAMEPADS];

// ---------- Utilidades ----------
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
  int t = 60;
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < 6; i++) { digitalWrite(LED_PIN, HIGH); delay(t); digitalWrite(LED_PIN, LOW); delay(t); }
  digitalWrite(LED_PIN, HIGH);
}

// ---------- Pantallas ----------
void drawPS4Waiting() {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(3, 10, "ESPERANDO CONEX");
  display.drawStr(3, 30, "PRENDE EL MANDO ");
  display.setFont(u8g2_font_ncenB14_tr);
  display.drawStr(3, 50, "  DE   PS4");
  display.sendBuffer();
}

void drawPS4Connected() {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(3, 10, "CONECTADO A");
  display.drawStr(3, 30, "      Mando de");
  display.setFont(u8g2_font_ncenB14_tr);
  display.drawStr(3, 50, "      PS4");
  display.sendBuffer();
}

// ---------- Bluepad32 Callbacks ----------
void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] == nullptr) { controllers[i] = ctl; impr_pant = 0; return; }
  }
}
void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] == ctl) { controllers[i] = nullptr; return; }
  }
}

// ---------- Control con mando (baja latencia) ----------
void gamepadProcess() {
  ControllerPtr ctl = nullptr;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers[i] && controllers[i]->isConnected() && controllers[i]->hasData()) {
      ctl = controllers[i];
      break;
    }
  }

  if (ctl) {
    if (impr_pant == 0) { drawPS4Connected(); impr_pant = 1; }

    // Ejes: -511..512 (Bluepad32)
    int x = ctl->axisX();
    int y = ctl->axisY();

    // Deadzone más baja => respuesta más inmediata
    const int DZ = 24;
    bool move = false;

    // Escalado lineal a 0..255 (8 bits PWM)
    auto scale = [](int v){
      int a = abs(v);
      if (a < 0) a = 0; if (a > 512) a = 512;
      return map(a, 0, 512, 0, 255);
    };

    // Prioridad: mezcla arcade simple (sin anular ejes)
    // Adelante/atrás:
    if (abs(y) > DZ) {
      uint8_t sp = scale(y);
      if (y < 0) {
        // Adelante
        digitalWrite(motor1a, HIGH); digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, LOW);  digitalWrite(motor2b, HIGH);
      } else {
        // Atrás
        digitalWrite(motor1a, LOW);  digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, HIGH); digitalWrite(motor2b, LOW);
      }
      pwmWriteA(sp); pwmWriteB(sp);
      move = true;
    }

    // Giro con mezcla: si X supera deadzone, mezclamos sobre lo anterior
       // Giro con mezcla: si X supera deadzone y no hay Y, giro sobre el lugar
    if (abs(x) > DZ && !move) {
      uint8_t spx = scale(x);
      if (x > 0) {
        // Derecha
        digitalWrite(motor1a, LOW);  digitalWrite(motor1b, HIGH);
        digitalWrite(motor2a, LOW);  digitalWrite(motor2b, HIGH);
      } else {
        // Izquierda
        digitalWrite(motor1a, HIGH); digitalWrite(motor1b, LOW);
        digitalWrite(motor2a, HIGH); digitalWrite(motor2b, LOW);
      }
      pwmWriteA(spx); pwmWriteB(spx);
      move = true;
    }

    // Mezcla diferencial correcta (desde los ejes directamente):
    if (abs(x) > DZ || abs(y) > DZ) {
      // Normalizamos a -1..1
      float fx = constrain((float)x / 512.0f, -1.0f, 1.0f);
      float fy = constrain((float)(-y) / 512.0f, -1.0f, 1.0f); // invertimos Y para “arriba=positivo”
      // Mezcla arcade: left = fy + fx ; right = fy - fx
      float left  = fy + fx;
      float right = fy - fx;
      left  = constrain(left,  -1.0f, 1.0f);
      right = constrain(right, -1.0f, 1.0f);

      // Direcciones
      if (left >= 0) { digitalWrite(motor1a, HIGH); digitalWrite(motor1b, LOW);  }
      else           { digitalWrite(motor1a, LOW);  digitalWrite(motor1b, HIGH); }
      if (right >= 0){ digitalWrite(motor2a, LOW);  digitalWrite(motor2b, HIGH); }
      else           { digitalWrite(motor2a, HIGH); digitalWrite(motor2b, LOW);  }

      uint8_t spL = (uint8_t)(fabs(left)  * 255.0f);
      uint8_t spR = (uint8_t)(fabs(right) * 255.0f);
      pwmWriteA(spL);
      pwmWriteB(spR);
      move = true;
    }

    if (!move) stopMotors();

  } else {
    if (impr_pant != 2) { drawPS4Waiting(); impr_pant = 2; }
    stopMotors();
  }
}

void ps4config_Bluepad32() {
  BP32.setup(&onConnectedController, &onDisconnectedController);

  pinMode(PAIR_BTN_PIN, INPUT_PULLUP);
  if (digitalRead(PAIR_BTN_PIN) == LOW) {
    BP32.forgetBluetoothKeys();   // SOLO para re-emparejar (PS+Share)
    delay(200);
  }
  BP32.enableVirtualDevice(false);

  drawPS4Waiting();
  impr_pant = 0;
}

// --------------------- Setup / Loop ---------------------
void setup() {
  Titilar();
  Serial.begin(115200);
  delay(10);

  display.begin();
  display.clearBuffer();

  pinMode(motor1a, OUTPUT);
  pinMode(motor1b, OUTPUT);
  pinMode(motor2a, OUTPUT);
  pinMode(motor2b, OUTPUT);

  // PWM nativo (LEDC) en los enables
  ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(enableA, PWM_CH_A);
  ledcAttachPin(enableB, PWM_CH_B);
  stopMotors();

  // Info Bluepad32
  Serial.printf("Bluepad32 FW: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BT Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  ps4config_Bluepad32();
}

void loop() {
  // Actualizamos lo más rápido posible, sin bloquear
  if (BP32.update()) {
    gamepadProcess();
  }
  // Cede CPU al RTOS sin frenar lazo de control
  delay(1); // o vTaskDelay(1);
}
