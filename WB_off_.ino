/*
 * 
  WB some standard registers add
  v1.1
  Тут добавлены coil 0..5 включающие gpio. Перечислены ниже как "Выходы для Coil 00-05"  
*/

//Для работы с EEPROM
#include <EEPROM.h>

//https://github.com/emelianov/modbus-esp8266
#include <ModbusRTU.h>

//Объект md
ModbusRTU mb;

#define RXTX_PIN 12

#define LED_BUILTIN 13

//Адреса в EEPROM для хранения регистров
#define h80eeprom 0 //Modbus address
#define h6Eeeprom 1 //110 speed port
#define h6Feeprom 2 //111 parity bit
#define h70eeprom 3 //112 stop bit



//Выходы для Coil 00-05
#define Coil00  A4
#define Coil01  A3
#define Coil02  A2
#define Coil03  A1
#define Coil04  A0
#define Coil05  LED_BUILTIN

//Массив для хранения выходов, связанных с coil
static uint8_t coilOutList[] = {Coil00, Coil01, Coil02, Coil03, Coil04, Coil05};

//тут описываем прототипы функций. Чтобы при создании структуры уже были.
void h80setup(void);
uint16_t h80set(TRegister* reg, uint16_t val);

void h6Esetup(void); //скорость.
uint16_t h6Eset(TRegister* reg, uint16_t val);

void h6Fsetup(void); //четность
uint16_t h6Fset(TRegister* reg, uint16_t val);

void h70setup(void); // стопбиты
uint16_t h70set(TRegister* reg, uint16_t val);

void i68setup(void); // время работы
uint16_t i68set(TRegister* reg, uint16_t val);

//User function
void c00setup(void); // coil relay output
uint16_t c00set(TRegister* reg, uint16_t val);
uint16_t c00get(TRegister* reg, uint16_t val);

void hc8setup(void); // просто регистр
//void h111setup(void);
//void h112setup(void);

/*
void ftest80(int);
void ftest104(int);
*/

typedef void (* funcPtr) (); 
// Теперь создадим массив длиной три 
// и сложем в него указатели на функции.
// Массив имеет тип, который мы только что создали
//FuncPtr funcArray[2] = {h80setup, pf104};

//Все эти функции будут вызваны при запуске
//Сюда дописываем и свои тоже
const funcPtr funcArr[] = {
  h80setup,
  h6Esetup,
  h6Fsetup,
  h70setup,
  i68setup,
  //User function
  c00setup,
  hc8setup
  };

  //User function
  //c00setup,
  //hc8setup

/*
//Структура
struct structRegistersModbus {
  const byte rtype;             //Тип регистра. 1:coil 2:input 3:holding
  const int address;           //Адрес регистра
  const byte param2;           //
  const byte param3;           //
  const void (* FuncPtr) (int);//Указатель на функцию, обрабатывающую регистр
};

const structRegistersModbus  allRegs[] = {
  {3, 80,  33,  0, &f80 },
  {1, 104, 34,  0, &f104 },
  {2, 1,   35,  35, &f80 },
  {3, 2,   36,  36, &f80 },
  {4, 3,   37,  37, &f80 },
  {5, 4,   38,  38, &f80 },
  {1, 104, 34,  0, &f104 },
  {2, 1,   35,  35, &f80 },
  {3, 2,   36,  36, &f80 },
  {4, 3,   37,  37, &f80 },
  {5, 4,   38,  38, &f80 },
  {1, 104, 34,  0, &f104 },
  {2, 1,   35,  35, &f80 },
  {3, 2,   36,  36, &f80 },
  {4, 3,   37,  37, &f80 },
  {5, 4,   38,  38, &f80 },
  {1, 104, 34,  0, &f104 },
  {2, 1,   35,  35, &f80 },
  {3, 2,   36,  36, &f80 },
  {4, 3,   37,  37, &f80 },
  {5, 4,   38,  38, &f80 },

};
*/

/*
//Структура
struct structTest {
  const byte type; 
  const int address;
  const byte param2;
  const byte param3;
  const void (* FuncPtr) (int);
};

const structTest Test[] PROGMEM = {
  {0, 80, 33,  0, &ftest80 },
  {1, 104, 34,  0, &ftest104 },
  {2, 0, 0,  35, 0 },
  {0, 0, 0,  35, 0 },
  {1, 0, 0,  37, 0 },
  {2, 0, 0,  38, 0 },
};
*/

/*
//const structRegistersModbus menu[] PROGMEM = {
//  {3, 0, 0,  0, pf80 },
//  {3, 0, 0,  0, pf80 },
//};
*/

byte modbusNeedStart = 0; //флаг необходимости перезапуска Modbus

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  
  Serial.begin(9600);
  Serial.println("Start!");

  //переключаем GPIO в выходы 2do - в цикле надо
  //pinMode(LED_BUILTIN, OUTPUT);
  //for (byte i=0; i<(sizeof(coilOutList)/sizeof(coilOutList[0]); i++){
    for (byte i=0; i < (sizeof(coilOutList)/sizeof(coilOutList[0])); i++){
    pinMode(coilOutList[i], OUTPUT);
  }
  
  //Считаем количество элементов массива.
  //const int funcArrLenght = sizeof(funcArr)/sizeof(funcArr[0]);
  Serial.print("funcArrLenght=");
  Serial.println(sizeof(funcArr)/sizeof(funcArr[0]));
  //тут в цикле пройдем по всем элементам funcArr и запустим функцию.

  for (byte i=0; i < (sizeof(funcArr)/sizeof(funcArr[0])); i++){
    Serial.print(i);
    Serial.println("start");
    delay(40);
    funcArr[i]();
    Serial.println("startED");
    //Serial.print((byte)allRegs[i].rtype);
    //Serial.print("   ");
    //Serial.println((int)allRegs[i].address);

  }


//Настройка канала A таймера0
OCR0A = 128; //Устанавливаем регистр совпадения
TIMSK0 |= (1 << OCIE0A);  // включение прерываний по совпадению для 0 таймера, канал A
  
//Serial.println("STARTED!");delay (100);
}



//uint32_t i = 65280;
//byte *ptri = (byte*)&i;
//  Serial.print("byte0=: ");
//  Serial.println(*ptri);
//  Serial.print("byte1=: ");
//  Serial.println(*(ptri+1));

// the loop function runs over and over again forever
void loop() {
  if (modbusNeedStart){
    //Serial.println("modbusNeedStart!!!");
    modbusStart();//Запускаем Modbus
    modbusNeedStart = 0;
    //Serial.print("mb.Hreg(128) "); Serial.println(mb.Hreg(128));
  }
  mb.task();


  
  //digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(50);                       // wait
  //digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(50);     // wait
//  Serial.print("mb.Hreg(128) "); Serial.println(mb.Hreg(128));
  
/*
  if (Serial.available()){
    if (Serial.read() == '1'){
      // Обращаемся к текущему элементу массива,
      // тем самым делая вызов функции по очереди 
        //funcArray[i]();
      // Это тернарный оператор. Он проверяет на истинность
      // выражение после знака = и перед знаком ?.
      // если оно истинно, то переменной присваивается значение
      // после знака ?, если ложно - то значение после знака :.
      // здесь он нужен, чтобы листать индекс массива от 0 до 
      // длины массива минус один.  
      i = (i < 1) ? i+1 : 0;
    }
  }
  */

  //Serial.println(pgm_read_byte(allRegs[0].type));
//  Serial.print("type: ");
//  Serial.println(allRegs[0].type);
//  Serial.print("FuncPtr: ");
//  allRegs[0].FuncPtr(1);
//  allRegs[1].FuncPtr(1);
  //allRegs[0].FuncPtr();
  //Test[0].FuncPtr(15);
  //Test[1].FuncPtr(22);
}





// А вот и описания функций.
//
//Адрес 128 **************
void h80setup(){
  //Serial.println("enter h80setup");
  //Надо прочитать из EEPROM байт.
  byte readByte = eeRead(h80eeprom, 1);
  if (0 == readByte){//Если прочитанное равно нулю
    eeWrite(h80eeprom, 1, 1); //Записываем в EEprom 1
    readByte = 1;
  };
  //Serial.print("address=");Serial.println(readByte);
  mb.addHreg(128); //Создаем holding регистр с адресом 128
  mb.Hreg(128, readByte);//Устанавливаем значение
  mb.onSetHreg(128, h80set); // Add callback on Hreg 128 value set
  modbusNeedStart=1;//нужно перезапустить Modbus
}
uint16_t h80set(TRegister* reg, uint16_t val){
  //Serial.println("enter h80set");
  //Надо прочитать из EEPROM байт.
  byte readByte = eeRead(h80eeprom, 1);
  if (0!=val || 248>val){//Если прочитанное НЕ равно новому, не ноль и в диапазоне адресов
    eeWrite(h80eeprom, 1, val); //Записываем в EEprom 1
    modbusNeedStart=1;//нужно перезапустить Modbus
    return val;
  }
  else{
    return readByte; //Если значение неверно - просто оставляем старое
  }
}

//Скорость 110 **************
void h6Esetup(){
  //Serial.println("enter h6Esetup");
  //Надо прочитать из EEPROM байт.
  byte readByte = eeRead(h6Eeeprom, 1);
//  Serial.print("readByte");Serial.println(readByte);
  mb.addHreg(110); //Создаем holding регистр с адресом 110
  uint16_t speedReg = 0;
  switch(readByte){
    case 1:
      speedReg = 12;
      break;
    case 2:
      speedReg = 24;
      break;
    case 4:
      speedReg = 48;
      break;
    case 9:
      speedReg = 96;
      break;
    case 19:
      speedReg = 192;
      break;
    case 57:
      speedReg = 576;
      break;
    case 115:
      speedReg = 1152;
      break;
  }
  if (0 == speedReg){//Если прочитанное ни с чем не совпало
    eeWrite(h6Eeeprom, 1, 9); //Записываем в EEprom 9 (9600)
    speedReg = 96;
  }
  mb.Hreg(110, speedReg);//Устанавливаем значение
  mb.onSetHreg(110, h6Eset); // Add callback on Hreg 128 value set
  modbusNeedStart=1;//нужно перезапустить Modbus
}
uint16_t h6Eset(TRegister* reg, uint16_t val){
  //Serial.println("enter h6Eset");
  uint8_t toeeprom = 0;
  //Serial.print("val=");Serial.println(val);
  switch(val){
    case 12:
      toeeprom = 1;
      break;
    case 24:
      toeeprom = 2;
      break;
    case 48:
      toeeprom = 4;
      break;
    case 96:
      toeeprom = 9;
      break;
    case 192:
      toeeprom = 19;
      break;
    case 576:
      toeeprom = 57;
      break;
    case 1152:
      toeeprom = 115;
      break;
  }
  if (0 == toeeprom){//Если записанное не совпало со списком скоростей - то ой.
    eeWrite(h6Eeeprom, 1, toeeprom); //Записываем в EEprom 1
    modbusNeedStart=1;//нужно перезапустить Modbus
    return val;
  }
  else{
    return mb.Hreg(110); //Если значение неверно - просто оставляем старое
  }
}

//Четность 111 **************
void h6Fsetup(){
  //Serial.println("enter h6Esetup (111)");
  //Надо прочитать из EEPROM байт.
  byte readByte = eeRead(h6Feeprom, 1);
  //Serial.print("readByte h6Feeprom");Serial.println(readByte);
  //Serial.print("h6Feeprom=");Serial.println(h6Feeprom);
  //Serial.print("readByte 0 ");Serial.println(eeRead(0, 1));
  //Serial.print("readByte 1 ");Serial.println(eeRead(1, 1));
  //Serial.print("readByte 2 ");Serial.println(eeRead(2, 1));
  //Serial.print("readByte 3 ");Serial.println(eeRead(3, 1));
  
  mb.addHreg(111); //Создаем holding регистр с адресом 111
  uint8_t temp = 4;
  switch(readByte){
    case 1:
      temp = 0;
      break;
    case 2:
      temp = 1;
      break;
    case 3:
      temp = 2;
      break;
  }
  if (4 == temp){//Если прочитанное ни с чем не совпало
    eeWrite(h6Feeprom, 1, 1); //Записываем в EEprom 3 (2 стопбита)
    temp = 0;
  }
   /* 0 — нет бита чётности (none),
    1 — нечетный (odd),
    2 — четный (even) */
  //Serial.print("mb.Hreg(111, temp) ");Serial.println(temp);
  mb.Hreg(111, temp);//Устанавливаем значение четности
  mb.onSetHreg(111, h6Fset); // Add callback on Hreg 111 value set
  modbusNeedStart=1;//нужно перезапустить Modbus
}

uint16_t h6Fset(TRegister* reg, uint16_t val){
  //Serial.println("enter h6Fset (111)");
  uint8_t toeeprom = 0;
  switch(val){
    case 0:
      toeeprom = 1;
      break;
    case 1:
      toeeprom = 2;
      break;
    case 2:
      toeeprom = 3;
      break;
  }
  if (0!=toeeprom){//Если записанное не совпало со списком четностей
    eeWrite(h6Feeprom, 1, toeeprom); //Записываем в EEprom 1
    modbusNeedStart=1;//нужно перезапустить Modbus
    return val;
  }
  else{
    return mb.Hreg(111); //Если значение неверно - просто оставляем старое
  }
}

void h70setup(){
  //Serial.println("enter h70setup (112)");
  //Надо прочитать из EEPROM байт.
  byte readByte = eeRead(h70eeprom, 1);
  //Serial.print("readByte");Serial.println(readByte);
  mb.addHreg(112); //Создаем holding регистр с адресом 112
  uint8_t temp = 0;
  switch(readByte){
    case 1:
      temp = 1;
      break;
    case 2:
      temp = 2;
      break;
  }
  if (0 == temp){//Если прочитанное ни с чем не совпало
    eeWrite(h70eeprom, 1, 2); //Записываем в EEprom 9 (9600)
    temp = 2;
  }
  /*1 — 1 стопбит
    2 — 2 стопбит*/
  mb.Hreg(112, temp);//Устанавливаем значение четности
  mb.onSetHreg(112, h70set); // Add callback on Hreg 112 value set
  modbusNeedStart=1;//нужно перезапустить Modbus
}

uint16_t h70set(TRegister* reg, uint16_t val){
  //Serial.println("enter h70set (112)");
  uint8_t toeeprom = 0;
  //Serial.print("val=");Serial.println(val);
  switch(val){
    case 1:
      toeeprom = 1;
      break;
    case 2:
      toeeprom = 2;
      break;
  }
  if (0 != toeeprom){//Если записанное не совпало со списком четностей
    eeWrite(h70eeprom, 1, toeeprom); //Записываем в EEprom 1
    modbusNeedStart=1;//нужно перезапустить Modbus
    return val;
  }
  else{
    return mb.Hreg(112); //Если значение неверно - просто оставляем старое
  }
}


void i68setup(){
  Serial.println("enter h68setup (104)");
  //
  mb.addIreg(104, 0, 2); //Создаем input регистрЫ с адресом 104-105 записывая в них "0"
  //mb.Ireg(104, 0);//Устанавливаем значение счетчика. Только надо делать это не тут.
  //mb.Ireg(105, 0);//Устанавливаем значение счетчика. Только надо делать это не тут.
  mb.onGetIreg(104, i68get, 2);  // Add single callback for multiple Inputs. It will be called for each of these inputs value get
  //modbusNeedStart=1;//нужно перезапустить Modbus
}

uint16_t i68get(TRegister* reg, uint16_t val){
  //Serial.println("enter h68get (104)");
  //Serial.print(reg->address.address);
  //
  //mb.Ireg(104, TCCR0A);//Устанавливаем значение счетчика. Только надо делать это не тут.
  //так, надо взять ulong значение millis(), разделить его на 1000.
  uint32_t tempSecunds = millis()/1000;
  //Получим ulong секунд. Старшую часть записываем в 104 а младшую в 105
  //просто берем указатель16-разрядный на старшую часть, для начала.
  uint16_t* ptrTemp = (uint16_t*)&tempSecunds;
  if(reg->address.address == 104)
    //return OCR0A;
    return *(ptrTemp+1);
  if(reg->address.address == 105)
    return *(ptrTemp);
  return 256;
}


ISR(TIMER0_COMPA_vect)
{
     //digitalWrite(13, !digitalRead(13));
}



/*
void ftest80(int i){
  Serial.print("PACANI");
  Serial.println(i*2);
}

void ftest104(int i){
  Serial.print("VASCHE");
  Serial.println(i*2);
}
*/


void modbusStart(){
  //Serial.begin(9600, SERIAL_8N2); //четность и стопбиты - менять!
  //mb.begin(&Serial, RXTX_PIN); //запускаем на указанном порту Modbus
  
  uint32_t speedReg = 0;
  switch(mb.Hreg(110)){
    case 12:
      speedReg = 1200;
      break;
    case 24:
      speedReg = 2400;
      break;
    case 48:
      speedReg = 4800;
      break;
    case 96:
      speedReg = 9600;
      break;
    case 192:
      speedReg = 19200;
      break;
    case 576:
      speedReg = 57600;
      break;
    case 1152:
      speedReg = 115200;
      break;
  }

  // SERIAL_8N2 - это дефайн, смотреть можно на 
  // https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/HardwareSerial.h
  // с 68 строки
  // SERIAL_8N1 0x06
  // SERIAL_8N2 0x0E
  // SERIAL_8E1 0x26
  // SERIAL_8E2 0x2E
  // SERIAL_8O1 0x36
  // SERIAL_8O2 0x3E
  // видно что получается суммой четности 
  // N - 0x00
  // E - 0x20
  // O - 0x30
  // и стопбитов
  // 1 - 0x06
  // 2 - 0x0E
  uint8_t serialParam=0;
  switch(mb.Hreg(111)){
    case 0:
      serialParam = 0x0;
      break;
    case 1:
      serialParam = 0x30;
      break;
    case 2:
      serialParam = 0x20;
      break;
  }
    switch(mb.Hreg(112)){
    case 1:
      serialParam += 0x06;
      break;
    case 2:
      serialParam += 0x0E;
      break;
  }
  
  Serial.begin(speedReg, serialParam); //четность и стопбиты - менять!
  //Serial.begin(speedReg, SERIAL_8N2);
  mb.begin(&Serial, RXTX_PIN); //запускаем на указанном порту Modbus
  
  Serial.print("Modbus speed ");Serial.println(mb.Hreg(110));
  mb.setBaudrate(speedReg);// Это не скорость порта, это для задержек, смотри в исходник библиотеки.
  mb.slave(mb.Hreg(128));
  //mb.slave(1); // for debug
  //Serial.print("Modbus speed ");Serial.print(mb.Hreg(110));Serial.print(" speed ");Serial.println(speedReg);
  Serial.print("mb.Hreg(111) parity:");Serial.println(mb.Hreg(111));
  Serial.print("mb.Hreg(112) stopbit");Serial.println(mb.Hreg(112));
  Serial.print("Modbus parameter ");Serial.println(serialParam);
  //Serial.print("Modbus parameter SERIAL_8N2:");Serial.println(SERIAL_8N2);
  Serial.print("Modbus started with ");Serial.println(mb.Hreg(128));
}


uint32_t eeRead (int addr, byte lenght){
  //Возвращает считанное из EEPROM значение
  //Первый (младший) байт читается с addr, количество считывемых байт [1..2] задается в lenght
  uint32_t retVal = 0;
  byte *ptrb = (byte*)&retVal; //Указатель приведен к byte, чтобы обращаться к байтам int отдельно
    for (byte i = 0; i<lenght; i++){
      *(ptrb+i) = EEPROM.read(addr);
    }
  return *ptrb;
}

void eeWrite (uint32_t addr, byte lenght, uint32_t val){
  //Пишет в EEPROM значение
  //Первый (младший) байт пишется в addr, количество записываемых байт [1..2] задается в lenght
  byte *ptrb = (byte*)&val; //Указатель приведен к byte, чтобы обращаться к байтам int отдельно
  for (byte i = 0; i<lenght; i++){
      //Serial.print("write ");
      //Serial.print(*(ptrb+i));
      //Serial.print("write 1");
      //Serial.print(*(ptrb+0));
      EEPROM.update(addr, *(ptrb+i));
    }
}


//User register:

void c00setup(){
  Serial.println("enter c00setup (00)");
  //
  mb.addCoil(0, 0, 6); //Создаем coil регистрЫ с адресом 00-05 записывая в них "0"
  mb.onGetCoil(0, c00get, 6);  // Add single callback for multiple coils. It will be called for each of these coils value get
  mb.onSetCoil(0, c00set, 6);  // Add single callback for multiple coils. It will be called for each of these coils value SET
  //modbusNeedStart=1;//нужно перезапустить Modbus
  Serial.println("eXIT c00setup (00)");
}

uint16_t c00get(TRegister* reg, uint16_t val){
  //Serial.println("enter c00get (00)");
  //Serial.print(reg->address.address);
  //
  //uint32_t tempSecunds = millis()/1000;
  //Получим ulong секунд. Старшую часть записываем в 104 а младшую в 105
  //просто берем указатель16-разрядный на старшую часть, для начала.
  //uint16_t* ptrTemp = (uint16_t*)&tempSecunds;

  //if(reg->address.address == 0)
  //  return COIL_VAL(1);
  //  //return *(ptrTemp+1);
  //if(reg->address.address == 1)
  //  //return *(ptrTemp);
  //  return COIL_VAL(1);
  return val;
}

uint16_t c00set(TRegister* reg, uint16_t val) {
  //Serial.print("enter c00set (00)");
  //Serial.print(reg->address.address);
  //Serial.println(COIL_BOOL(val));
  digitalWrite(coilOutList[reg->address.address], COIL_BOOL(val));
  return val;
}

void hc8setup(){
  Serial.println("enter hс8setup (200)");
  //Надо прочитать из EEPROM байт.
//  byte readByte = eeRead(h70eeprom, 1);
//  Serial.print("readByte");Serial.println(readByte);
  mb.addHreg(200); //Создаем holding регистр с адресом 200
}