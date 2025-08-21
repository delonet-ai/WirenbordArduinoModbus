#include <EEPROM.h>
#include <ModbusRTU.h>    // Alexander Emelianov ModbusRTU
#include <ServoSmooth.h>  // https://github.com/AlexGyver/ServoSmooth

// ---------------- RS-485 / Modbus ----------------
#define DE_RE_PIN 2        // DE/RE на D2
#define SLAVE_ID  128

// ---------------- Сервы ----------------
#define SERVO_COUNT 10
// Выпуск: D8..D12 (адреса 0..4), Впуск: A0..A4 (адреса 5..9)
const uint8_t SERVO_PINS[SERVO_COUNT] = {8, 9, 10, 11, 12, A0, A1, A2, A3, A4};

// Общие параметры серво (в микросекундах и у.ед. библиотеки)
const int16_t SERVO_MIN   = -100;   // µs (0%)
const int16_t SERVO_MAX   = 2500;   // µs (100%)
const int16_t SERVO_DEF   = 1500;   // µs (дефолт при первом запуске)
const int     SERVO_SPEED = 50;     // скорость ServoSmooth
const float   SERVO_ACCEL = 0.1f;   // ускорение ServoSmooth

// ---------------- EEPROM layout ----------------
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_MAGIC      0xA55Au
#define EEPROM_DATA_ADDR  (EEPROM_MAGIC_ADDR + sizeof(uint16_t))  // далее 10 * int16_t

// ---------------- Глобальные объекты/состояние ----------------
ModbusRTU mb;
ServoSmooth servos[SERVO_COUNT];
int16_t servoPos[SERVO_COUNT];       // текущие целевые позиции (сырые, µs)

// Отложенная запись EEPROM
volatile uint16_t eepromDirtyMask = 0;
uint32_t lastEEWriteMs = 0;

// Переключатель режима Fireplace (HREG 10)
uint16_t fireplace = 0;      // 0=выкл, 1=вкл
bool fireplacePrev = false;  // для детекта фронтов

// Индексы каналов для "Камина": Комната 3 и 4
const uint8_t EX3_IDX = 2;  // Вытяжка 3
const uint8_t EX4_IDX = 3;  // Вытяжка 4
const uint8_t IN3_IDX = 7;  // Приток 3
const uint8_t IN4_IDX = 8;  // Приток 4

// Снимок прежних значений для восстановления после "Камина"
int16_t snapEX3 = 0, snapEX4 = 0, snapIN3 = 0, snapIN4 = 0;

// ---------------- Утилиты ----------------
static inline int16_t clamp16(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------------- Инициализация ----------------
void setup() {
  // Линия Modbus (9600 8N2 — как в WB)
  Serial.begin(9600, SERIAL_8N2);
  mb.begin(&Serial, DE_RE_PIN);
  mb.slave(SLAVE_ID);

  // Загрузка/инициализация EEPROM
  uint16_t magic = 0;
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  if (magic != EEPROM_MAGIC) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
      servoPos[i] = SERVO_DEF;
      EEPROM.put(EEPROM_DATA_ADDR + i * sizeof(int16_t), servoPos[i]);
    }
    EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  } else {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
      EEPROM.get(EEPROM_DATA_ADDR + i * sizeof(int16_t), servoPos[i]);
      servoPos[i] = clamp16(servoPos[i], SERVO_MIN, SERVO_MAX);
    }
  }

  // Инициализация серв и регистров
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    servos[i].attach(SERVO_PINS[i], SERVO_MIN, SERVO_MAX);
    servos[i].setSpeed(SERVO_SPEED);
    servos[i].setAccel(SERVO_ACCEL);
    servos[i].setCurrent(servoPos[i]);  // старт без рывка
    servos[i].start();

    mb.addHreg(i, (uint16_t)servoPos[i]);  // HREG 0..9
  }

  // Переключатель Fireplace (HREG 10)
  mb.addHreg(10, fireplace);
}

// ---------------- Главный цикл ----------------
void loop() {
  // Modbus
  mb.task();

  // Плавное движение серв
  for (uint8_t i = 0; i < SERVO_COUNT; i++) servos[i].tick();

  // Считываем переключатель Fireplace и определяем переходы
  fireplace = mb.Hreg(10);
  bool fp = (fireplace != 0);

  // --- Переход 0 -> 1: включение "Камин"
  if (fp && !fireplacePrev) {
    // 1) Снимок текущих значений из регистров (сырых µs)
    snapEX3 = (int16_t)mb.Hreg(EX3_IDX);
    snapEX4 = (int16_t)mb.Hreg(EX4_IDX);
    snapIN3 = (int16_t)mb.Hreg(IN3_IDX);
    snapIN4 = (int16_t)mb.Hreg(IN4_IDX);

    // 2) Принудительно: ВЫТЯЖКА 3/4 = 0%, ПРИТОК 3/4 = 100%
    const int16_t forcedEx = SERVO_MIN;
    const int16_t forcedIn = SERVO_MAX;

    servoPos[EX3_IDX] = forcedEx; servos[EX3_IDX].setTarget(forcedEx); mb.Hreg(EX3_IDX, (uint16_t)forcedEx);
    servoPos[EX4_IDX] = forcedEx; servos[EX4_IDX].setTarget(forcedEx); mb.Hreg(EX4_IDX, (uint16_t)forcedEx);
    servoPos[IN3_IDX] = forcedIn; servos[IN3_IDX].setTarget(forcedIn); mb.Hreg(IN3_IDX, (uint16_t)forcedIn);
    servoPos[IN4_IDX] = forcedIn; servos[IN4_IDX].setTarget(forcedIn); mb.Hreg(IN4_IDX, (uint16_t)forcedIn);

    // ВНИМАНИЕ: принудительные значения не пишем в EEPROM
  }

  if (fp) {
    // --- Режим "Камин" удерживает только 3/4 комнаты в заданных значениях.
    const int16_t forcedEx = SERVO_MIN;
    const int16_t forcedIn = SERVO_MAX;

    // Если UI/кто-то переписал регистры — вернуть обратно, сервы тоже держим
    if ((int16_t)mb.Hreg(EX3_IDX) != forcedEx) mb.Hreg(EX3_IDX, (uint16_t)forcedEx);
    if ((int16_t)mb.Hreg(EX4_IDX) != forcedEx) mb.Hreg(EX4_IDX, (uint16_t)forcedEx);
    if ((int16_t)mb.Hreg(IN3_IDX) != forcedIn) mb.Hreg(IN3_IDX, (uint16_t)forcedIn);
    if ((int16_t)mb.Hreg(IN4_IDX) != forcedIn) mb.Hreg(IN4_IDX, (uint16_t)forcedIn);

    if (servoPos[EX3_IDX] != forcedEx) { servoPos[EX3_IDX] = forcedEx; servos[EX3_IDX].setTarget(forcedEx); }
    if (servoPos[EX4_IDX] != forcedEx) { servoPos[EX4_IDX] = forcedEx; servos[EX4_IDX].setTarget(forcedEx); }
    if (servoPos[IN3_IDX] != forcedIn) { servoPos[IN3_IDX] = forcedIn; servos[IN3_IDX].setTarget(forcedIn); }
    if (servoPos[IN4_IDX] != forcedIn) { servoPos[IN4_IDX] = forcedIn; servos[IN4_IDX].setTarget(forcedIn); }
  } else {
    // --- Нормальный режим: применяем изменения из HREG 0..9 (и сохраняем в EEPROM)
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
      uint16_t raw = mb.Hreg(i);
      int16_t v = clamp16((int16_t)raw, SERVO_MIN, SERVO_MAX);
      if (v != servoPos[i]) {
        servoPos[i] = v;
        servos[i].setTarget(v);
        eepromDirtyMask |= (1u << i);      // отложим запись EEPROM
      }
    }
  }

  // --- Переход 1 -> 0: выключение "Камин" — восстановить прежние значения у 3/4 комнат
  if (!fp && fireplacePrev) {
    // Вернуть снимок в регистры и сервы (EEPROM не трогаем: там и так прежние значения)
    servoPos[EX3_IDX] = clamp16(snapEX3, SERVO_MIN, SERVO_MAX);
    servoPos[EX4_IDX] = clamp16(snapEX4, SERVO_MIN, SERVO_MAX);
    servoPos[IN3_IDX] = clamp16(snapIN3, SERVO_MIN, SERVO_MAX);
    servoPos[IN4_IDX] = clamp16(snapIN4, SERVO_MIN, SERVO_MAX);

    servos[EX3_IDX].setTarget(servoPos[EX3_IDX]); mb.Hreg(EX3_IDX, (uint16_t)servoPos[EX3_IDX]);
    servos[EX4_IDX].setTarget(servoPos[EX4_IDX]); mb.Hreg(EX4_IDX, (uint16_t)servoPos[EX4_IDX]);
    servos[IN3_IDX].setTarget(servoPos[IN3_IDX]); mb.Hreg(IN3_IDX, (uint16_t)servoPos[IN3_IDX]);
    servos[IN4_IDX].setTarget(servoPos[IN4_IDX]); mb.Hreg(IN4_IDX, (uint16_t)servoPos[IN4_IDX]);
  }

  fireplacePrev = fp;

  // Отложенная запись EEPROM (по одному регистру ~раз в 20 мс)
  if (eepromDirtyMask && (millis() - lastEEWriteMs > 20)) {
    uint8_t i = 0;
    while (i < SERVO_COUNT && ((eepromDirtyMask & (1u << i)) == 0)) i++;
    if (i < SERVO_COUNT) {
      EEPROM.put(EEPROM_DATA_ADDR + i * sizeof(int16_t), servoPos[i]);
      eepromDirtyMask &= ~(1u << i);
      lastEEWriteMs = millis();
    }
  }
}
