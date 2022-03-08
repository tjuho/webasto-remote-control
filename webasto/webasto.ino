//Webasto part of the project
//uses HC-12 to send and receive between webasto and remote control part of the project
//autor Juho Taipale

#include <SoftwareSerial.h>
#include <EEPROM.h>

#define CHECK 12

SoftwareSerial mySerial(2, 3); // rx and tx pins

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
//define the parckaged data structure
struct __attribute__ ((packed)) data_packet {
  char code;
  unsigned long magicNumber;
} data;
bool webastoState = false;
unsigned int rollingCode = 0;
int fireupPin = 4;
int webastoStatusPin = 5;
int temperaturePin = 6;
int len = sizeof(data);
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
  pinMode(fireupPin, OUTPUT);
  digitalWrite(fireupPin, LOW);
  rollingCode = memoryReadLong(0);
  pinMode(webastoStatusPin, INPUT);
}

void loop() {

  if(mySerial.available() > 1){//read data from hc-12
    char aux[len];
    int chars = mySerial.readBytes(aux, len);
    if (chars == len){
      memcpy(&data, &aux, len);
      for (int i=0; i<128; i++){ //check up to 128 next possible rolling code values
        randomSeed(rollingcodeOffset+rollingCode+i);
        if (random(65536) == data.magicNumber){ //rolling code check
          rollingCode += i;
          memoryWriteLong(0, rollingCode);
          Serial.print("correct code received");
          Serial.println(rollingCode);
          Serial.print("code ");
          Serial.println(data.code);
          if (data.code == 'f'){ //fire up webasto with simulated key press of 500ms
            digitalWrite(fireupPin, HIGH);
            Serial.println("fire up webasto");
            delay(500);
            digitalWrite(fireupPin, LOW);
            webastoState = !webastoState;
          } 
          if (data.code == 'q' || data.code == 'f'){ //get webasto current state back to remote
            int wstatus = digitalRead(webastoStatusPin);
            if (wstatus)
              data.code = '1';
            else
              data.code = '0';
            memcpy(&aux,&data,len);
            mySerial.write((uint8_t *)&aux,len);   
          }
        }
      }
    }
  }
  delay(200);
}
