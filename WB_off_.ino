#include <EEPROM.h>
#include <ModbusRTU.h>
#include <ServoSmooth.h>

#define RXTX_PIN 2
#define SERVO_COUNT 10
const uint8_t servoPins[SERVO_COUNT] = {8, 9, 10, 11, 12, A0, A1, A2, A3, A4};

const int SERVO_MIN = -100;
const int SERVO_MAX = 2500;
const int SERVO_SPEED = 50;
const double SERVO_ACCEL = 0.1;

ModbusRTU mb;
ServoSmooth servos[SERVO_COUNT];
int16_t servoPos[SERVO_COUNT];

uint16_t onSetServo(TRegister* reg, uint16_t val) {
  uint8_t idx = reg->address;
  if (idx < SERVO_COUNT) {
    servoPos[idx] = (int16_t)val;
    EEPROM.put(idx * sizeof(int16_t), servoPos[idx]);
    servos[idx].setTarget(servoPos[idx]);
  }
  return val;
}

void setup() {
  Serial.begin(9600);
  mb.begin(&Serial, RXTX_PIN);
  mb.slave(128);

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    EEPROM.get(i * sizeof(int16_t), servoPos[i]);
    if (servoPos[i] < SERVO_MIN || servoPos[i] > SERVO_MAX) {
      servoPos[i] = SERVO_MIN;
    }
    servos[i].attach(servoPins[i], SERVO_MIN, SERVO_MAX, servoPos[i]);
    servos[i].setSpeed(SERVO_SPEED);
    servos[i].setAccel(SERVO_ACCEL);
    servos[i].setAutoDetach(false);
    servos[i].start();

    mb.addHreg(i, (uint16_t)servoPos[i]);
    mb.onSetHreg(i, onSetServo);
  }
}

void loop() {
  mb.task();
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    servos[i].tick();
  }
}

