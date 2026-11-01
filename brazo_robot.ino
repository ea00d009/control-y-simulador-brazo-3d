#include <Servo.h>
#include <EEPROM.h>

// Pines en el Sensor Shield del Arduino Nano (G = Marron/Negro, V = Rojo, S = Naranja/Amarillo)
const int NUM_SERVOS = 4;
const int SERVO_PINS[NUM_SERVOS] = {8, 9, 10, 11};
const char* SERVO_NAMES[NUM_SERVOS] = {"Base", "Hombro", "Codo", "Pinza"};

Servo servos[NUM_SERVOS];
bool isAttached[NUM_SERVOS] = {false, false, false, false};

// Angulos actuales y objetivos
int currentAngle[NUM_SERVOS] = {90, 90, 90, 90};
int targetAngle[NUM_SERVOS]  = {90, 90, 90, 90};

// Limites de fabrica (amplios para servos de 180 grados)
const int FACTORY_MIN[NUM_SERVOS] = {0, 10, 10, 10};
const int FACTORY_MAX[NUM_SERVOS] = {180, 170, 170, 170};
const int FACTORY_SPEED = 15;

// Limites activos (cargados desde EEPROM o modificados en vivo)
int minAngle[NUM_SERVOS] = {0, 10, 10, 10};
int maxAngle[NUM_SERVOS] = {180, 170, 170, 170};

// Velocidad de suavizado: milisegundos por cada grado de movimiento
int stepDelayMs = 15;
unsigned long lastStepTime = 0;

// Direcciones de memoria EEPROM para persistencia en el Arduino Nano
const byte EEPROM_MAGIC = 0xEA; // Marca de validez en direccion 0
const int ADDR_MAGIC = 0;
const int ADDR_SPEED = 1;
const int ADDR_MIN   = 2; // 4 bytes (2..5)
const int ADDR_MAX   = 6; // 4 bytes (6..9)

String inputBuffer = "";

void saveSettingsToEEPROM() {
  EEPROM.update(ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.update(ADDR_SPEED, (byte)constrain(stepDelayMs, 0, 100));
  for (int i = 0; i < NUM_SERVOS; i++) {
    EEPROM.update(ADDR_MIN + i, (byte)constrain(minAngle[i], 0, 180));
    EEPROM.update(ADDR_MAX + i, (byte)constrain(maxAngle[i], 0, 180));
  }
}

void loadSettingsFromEEPROM() {
  if (EEPROM.read(ADDR_MAGIC) == EEPROM_MAGIC) {
    stepDelayMs = constrain((int)EEPROM.read(ADDR_SPEED), 0, 100);
    for (int i = 0; i < NUM_SERVOS; i++) {
      int mn = constrain((int)EEPROM.read(ADDR_MIN + i), 0, 180);
      int mx = constrain((int)EEPROM.read(ADDR_MAX + i), 0, 180);
      if (mn < mx) {
        minAngle[i] = mn;
        maxAngle[i] = mx;
      }
    }
  } else {
    saveSettingsToEEPROM();
  }
}

void resetFactorySettings() {
  stepDelayMs = FACTORY_SPEED;
  for (int i = 0; i < NUM_SERVOS; i++) {
    minAngle[i] = FACTORY_MIN[i];
    maxAngle[i] = FACTORY_MAX[i];
    targetAngle[i] = constrain(targetAngle[i], minAngle[i], maxAngle[i]);
  }
  saveSettingsToEEPROM();
}

void printStatus() {
  Serial.print("{\"angles\":[");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(currentAngle[i]);
    if (i < NUM_SERVOS - 1) Serial.print(",");
  }
  Serial.print("],\"targets\":[");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(targetAngle[i]);
    if (i < NUM_SERVOS - 1) Serial.print(",");
  }
  Serial.print("],\"attached\":[");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(isAttached[i] ? "true" : "false");
    if (i < NUM_SERVOS - 1) Serial.print(",");
  }
  Serial.print("],\"min\":[");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(minAngle[i]);
    if (i < NUM_SERVOS - 1) Serial.print(",");
  }
  Serial.print("],\"max\":[");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(maxAngle[i]);
    if (i < NUM_SERVOS - 1) Serial.print(",");
  }
  Serial.print("],\"speed\":");
  Serial.print(stepDelayMs);
  Serial.println("}");
}

void setServoTarget(int idx, int angle) {
  if (idx < 0 || idx >= NUM_SERVOS) return;
  angle = constrain(angle, minAngle[idx], maxAngle[idx]);
  targetAngle[idx] = angle;

  if (!isAttached[idx]) {
    currentAngle[idx] = angle;
    servos[idx].write(currentAngle[idx]);
    servos[idx].attach(SERVO_PINS[idx]);
    isAttached[idx] = true;
  } else if (stepDelayMs == 0) {
    currentAngle[idx] = angle;
    servos[idx].write(currentAngle[idx]);
  }
}

void detachAll() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (isAttached[i]) {
      servos[i].detach();
      isAttached[i] = false;
    }
    targetAngle[i] = currentAngle[i];
  }
  Serial.println("{\"event\":\"DETACHED_ALL\"}");
}

void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;

  // X = Parada / Soltar todos los servos
  if (cmd == "X" || cmd == "STOP" || cmd == "DETACH") {
    detachAll();
    printStatus();
    return;
  }

  // RESET = Restaurar limites y velocidad de fabrica en EEPROM
  if (cmd == "RESET" || cmd == "FACTORY") {
    resetFactorySettings();
    Serial.println("{\"event\":\"FACTORY_RESET\"}");
    printStatus();
    return;
  }

  // STATUS = Consultar estado actual
  if (cmd == "STATUS" || cmd == "?") {
    printStatus();
    return;
  }

  // V:<ms> = Cambiar velocidad (ms por grado) y guardar en EEPROM
  if (cmd.startsWith("V:")) {
    stepDelayMs = constrain(cmd.substring(2).toInt(), 0, 100);
    saveSettingsToEEPROM();
    printStatus();
    return;
  }

  // S<0..3>:<angulo> = Mover un servo individual (ej: S0:90)
  if (cmd.startsWith("S") && cmd.indexOf(':') == 2) {
    int idx = cmd.charAt(1) - '0';
    int angle = cmd.substring(3).toInt();
    setServoTarget(idx, angle);
    printStatus();
    return;
  }

  // D<0..3> = Soltar un servo individual (ej: D0)
  if (cmd.startsWith("D") && cmd.length() == 2) {
    int idx = cmd.charAt(1) - '0';
    if (idx >= 0 && idx < NUM_SERVOS && isAttached[idx]) {
      servos[idx].detach();
      isAttached[idx] = false;
      targetAngle[idx] = currentAngle[idx];
    }
    printStatus();
    return;
  }

  // A:<s0>,<s1>,<s2>,<s3> = Mover los 4 servos a una pose (ej: A:90,90,90,90)
  if (cmd.startsWith("A:")) {
    String rest = cmd.substring(2);
    for (int i = 0; i < NUM_SERVOS; i++) {
      int comma = rest.indexOf(',');
      String part = (comma == -1) ? rest : rest.substring(0, comma);
      if (part.length() > 0) {
        setServoTarget(i, part.toInt());
      }
      if (comma == -1) break;
      rest = rest.substring(comma + 1);
    }
    printStatus();
    return;
  }

  // L<0..3>:<min>,<max> = Configurar limites de seguridad de un servo y persistir en EEPROM
  if (cmd.startsWith("L") && cmd.indexOf(':') == 2) {
    int idx = cmd.charAt(1) - '0';
    int comma = cmd.indexOf(',');
    if (idx >= 0 && idx < NUM_SERVOS && comma > 3) {
      int mn = constrain(cmd.substring(3, comma).toInt(), 0, 180);
      int mx = constrain(cmd.substring(comma + 1).toInt(), 0, 180);
      if (mn < mx) {
        minAngle[idx] = mn;
        maxAngle[idx] = mx;
        targetAngle[idx] = constrain(targetAngle[idx], mn, mx);
        saveSettingsToEEPROM();
      }
    }
    printStatus();
    return;
  }
}

void setup() {
  Serial.begin(115200);
  loadSettingsFromEEPROM();
  Serial.println("{\"event\":\"READY\",\"pins\":[8,9,10,11]}");
  printStatus();
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }

  // Movimiento suavizado grado a grado
  if (stepDelayMs > 0 && (millis() - lastStepTime >= (unsigned long)stepDelayMs)) {
    lastStepTime = millis();
    for (int i = 0; i < NUM_SERVOS; i++) {
      if (isAttached[i] && currentAngle[i] != targetAngle[i]) {
        if (currentAngle[i] < targetAngle[i]) {
          currentAngle[i]++;
        } else {
          currentAngle[i]--;
        }
        servos[i].write(currentAngle[i]);
      }
    }
  }
}
