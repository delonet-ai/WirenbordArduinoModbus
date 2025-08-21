#include <EEPROM.h>
#include <ModbusRTU.h>    // Alexander Emelianov ModbusRTU
#include <ServoSmooth.h>  // https://github.com/AlexGyver/ServoSmooth

// ---------------- RS-485 / Modbus ----------------
#define DE_RE_PIN 2        // DE/RE на D2
#define SLAVE_ID  128

// ---------------- Сервы ----------------
#define SERVO_COUNT 10
// Выпуск: D8..D12, Впуск: A0..A4
const uint8_t SERVO_PINS[SERVO_COUNT] = {8, 9, 10, 11, 12, A0, A1, A2, A3, A4};

// Общие параметры серво (в микросекундах и у.ед. библиотеки)
const int16_t SERVO_MIN   = 100;   // µs
const int16_t SERVO_MAX   = 2500;   // µs
const int16_t SERVO_DEF   = 1500;   // µs (стартовая/дефолтная)
const int     SERVO_SPEED = 50;     // скорость ServoSmooth
const float   SERVO_ACCEL = 0.1;   // ускорение ServoSmooth

// ---------------- EEPROM layout ----------------
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_MAGIC      0xA55Au
#define EEPROM_DATA_ADDR  (EEPROM_MAGIC_ADDR + sizeof(uint16_t))  // 10 * int16_t дальше

// ---------------- Глобальные объекты ----------------
ModbusRTU mb;
ServoSmooth servos[SERVO_COUNT];
int16_t servoPos[SERVO_COUNT];       // текущие целевые позиции (сырые, µs)

// Отложенная запись EEPROM
volatile uint16_t eepromDirtyMask = 0;
uint32_t lastEEWriteMs = 0;

// ---------------- Утилиты ----------------
static inline int16_t clamp16(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------------- Коллбэк записи HREG ----------------
// Вызывается библиотекой при записи Holding-регистра
bool onSetServo(TRegister* reg, uint16_t val) {
  // Найти индекс по указателю регистра
uint16_t idx = reg->address.address;
if (idx >= SERVO_COUNT) return false;

  // Приводим значение к int16 и ограничиваем по диапазону сервы
  int16_t v = (int16_t)val;
  v = clamp16(v, SERVO_MIN, SERVO_MAX);

  // Обновляем целевую позицию и запускаем плавное движение
  servoPos[idx] = v;
  servos[idx].setTarget(v);

  // Подтверждаем запись для Modbus (ВАЖНО!)
  reg->value = (uint16_t)v;

  // Отложенная запись в EEPROM, чтобы не блокировать ответ
  eepromDirtyMask |= (1u << idx);

  return true;
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

    // Создаём Holding-регистры и вешаем коллбэк именно на указатель
mb.addHreg(i, (uint16_t)servoPos[i]);

  }
}

// ---------------- Главный цикл ----------------
void loop() {
  // Обработка Modbus
  mb.task();

  // Плавное движение серв
  for (uint8_t i = 0; i < SERVO_COUNT; i++) servos[i].tick();
// Применяем изменения, пришедшие по Modbus (без колбэков)
for (uint8_t i = 0; i < SERVO_COUNT; i++) {
  uint16_t raw = mb.Hreg(i);
  int16_t v = (int16_t)raw;
  if (v < SERVO_MIN) v = SERVO_MIN;
  if (v > SERVO_MAX) v = SERVO_MAX;
  if (v != servoPos[i]) {
    servoPos[i] = v;
    servos[i].setTarget(v);
    eepromDirtyMask |= (1u << i);   // отложим запись EEPROM
  }
}
  // Отложенная запись EEPROM (не чаще, чем каждые ~20 мс, по одному регистру за проход)
  if (eepromDirtyMask && (millis() - lastEEWriteMs > 20)) {
    // индекс первого помеченного (минимальный)
    uint8_t i = 0;
    while (i < SERVO_COUNT && ((eepromDirtyMask & (1u << i)) == 0)) i++;

    if (i < SERVO_COUNT) {
      EEPROM.put(EEPROM_DATA_ADDR + i * sizeof(int16_t), servoPos[i]);
      eepromDirtyMask &= ~(1u << i);
      lastEEWriteMs = millis();
    }
  }
}
