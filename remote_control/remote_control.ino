//Remote control part of the project
//uses HC-12 to send and receive between webasto and remote control
//autor Juho Taipale

#include <SoftwareSerial.h>
#include <EEPROM.h>

#define CHECK 12
SoftwareSerial mySerial(7, 8); // rx and tx pins

unsigned long memoryReadLong(int address) {
 unsigned long value = memoryReadInt(address);
 //shift read value up and then 'OR' with the value from next memory location
 value = value << 16;
 value = value | memoryReadInt(address+2);
 return value;
}

void memoryWriteInt(int address, int value) {
 EEPROM.write(address,highByte(value));
 EEPROM.write(address+1 ,lowByte(value));
}
 
void memoryWriteLong(int address, unsigned long value) {
 //write lower part into memory
 memoryWriteInt(address+2, word(value));
 //shift upper part down and write that to previous address
 value = value >> 16;
 memoryWriteInt(address, word(value));
}

unsigned int memoryReadInt(int address) {
 unsigned int value = word(EEPROM.read(address), EEPROM.read(address+1));
 return value;
}
struct __attribute__ ((packed)) data_packet {
  char code;
  unsigned long magicNumber;
} data;

bool ledStatus = LOW;
unsigned int rollingCode = 0;
bool previousFireButtonState = LOW;
bool previousQueryButtonState = LOW;
int fireButtonPin = 4;
int queryButtonPin = 9;
int redLedPin = 2;
int greenLedPin = 3;
int inPin = 5;
int outPin = 6;
int len = sizeof(data);
unsigned long previousMagicNumber=0;
const long rollingcodeOffset = 1234;

void setup() {
  Serial.begin(2400);
  mySerial.begin(2400);
  long check = memoryReadLong(4);
  if (check == CHECK){
    rollingCode = memoryReadLong(0);
  } else {
    rollingCode = 0;
    memoryWriteLong(0, 0);
    memoryWriteLong(4, CHECK);    
    Serial.println("code reset");    
  }
  pinMode(fireButtonPin, INPUT_PULLUP);
  pinMode(queryButtonPin, INPUT_PULLUP);
  pinMode(redLedPin, OUTPUT);
  digitalWrite(redLedPin, LOW);
  pinMode(greenLedPin, OUTPUT);
  digitalWrite(greenLedPin, LOW);
}

void loop() {
  if(mySerial.available() > 1){//read data from hc-12
    Serial.println("data received");
    char aux[len];
    int chars = mySerial.readBytes(aux, len);
    if (chars == len){
      memcpy(&data, &aux, len);
      Serial.print("received code ");
      Serial.println(data.code);
      if (data.code == '0'){//off: blink red led
        digitalWrite(redLedPin, HIGH);
        delay(200);      
        digitalWrite(redLedPin, LOW);
        delay(80);
        digitalWrite(redLedPin, HIGH);
        delay(200);      
        digitalWrite(redLedPin, LOW);
        delay(80);
        digitalWrite(redLedPin, HIGH);
        delay(200);      
        digitalWrite(redLedPin, LOW);
      } else if (data.code == '1'){//on: blink green led
        digitalWrite(greenLedPin, HIGH);
        delay(200);      
        digitalWrite(greenLedPin, LOW);
        delay(80);
        digitalWrite(greenLedPin, HIGH);
        delay(200);      
        digitalWrite(greenLedPin, LOW);
        delay(80);
        digitalWrite(greenLedPin, HIGH);
        delay(200);      
        digitalWrite(greenLedPin, LOW);
      }
    }
  }
  bool state = digitalRead(fireButtonPin); //check fire up button state
  if (previousFireButtonState != state){ //react only one time to button press
    if (state == LOW){
      Serial.print("fire button pressed ");
      Serial.println(rollingCode);
      previousFireButtonState = LOW;
      randomSeed(rollingcodeOffset + rollingCode);
      data.magicNumber = random(65536);
      data.code = 'f';
      char aux[len];
      memcpy(&aux,&data,len); //copy data to buffer
      mySerial.write((uint8_t *)&aux,len); //write to hc-12
      memoryWriteLong(0,++rollingCode); //increment rolling code value
    } else {
      previousFireButtonState = HIGH;
    }
  }
  state = digitalRead(queryButtonPin); //check query button state
  if (previousQueryButtonState != state){ //react only one time to button press
    if (state == LOW){
      Serial.print("query button pressed ");
      Serial.println(rollingCode);
      previousQueryButtonState = LOW;
      randomSeed(rollingcodeOffset + rollingCode);
      data.magicNumber = random(65536);
      data.code = 'q';
      char aux[len];
      memcpy(&aux,&data,len); //copy data to buffer
      mySerial.write((uint8_t *)&aux,len); //write to hc-12
      memoryWriteLong(0,++rollingCode); //increment rolling code value
    } else {
      previousQueryButtonState = HIGH;
    }
  delay(100);
  }
}
