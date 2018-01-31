#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define START_FLAG 128
#define BROADCAST 127
#define REPORT 126
#define GET_COUNT 64
#define SET_COLOR 32
#define GET_STATES 16
#define SET_ACTIVE 4
#define RESET_ACTIVE 2
#define END_FLAG 0

const int len = 64;
unsigned char data[len];
int pnt =0;
bool stringComplete = false;

int ledPin = 0;
int sensorPin = 4;

int prevState = 0;
int address = -1;

SoftwareSerial serial(2, 3); // RX, TX
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, ledPin, NEO_GRB + NEO_KHZ800);

uint32_t activeColor = strip.Color(0xff,0xff,0xff);
byte pCol[3] = {255,0,0};


void setEmptyColor(){
  strip.setPixelColor(0, strip.Color((pCol[0])/10,(pCol[1])/10,(pCol[2])/10));
}

void setOccColor(){
  strip.setPixelColor(0, strip.Color(pCol[0],pCol[1],pCol[2]));
}

void sendPacket(char addr, char cmd, byte data = 128, byte data2 = 128, byte data3 = 128, byte data4 = 128){
  int tot = (START_FLAG | addr) + cmd;
  serial.write(START_FLAG | addr);
  serial.write(cmd);
  
  //digitalWrite(13,!digitalRead(13));
  if(data < 128) tot+=data, serial.write(data);
  if(data2 < 128) tot+=data2, serial.write(data2);
  if(data3 < 128) tot+=data3, serial.write(data3);
  if(data4 < 128) tot+=data4, serial.write(data4);
  
  serial.write(tot & 0b01111111);
  serial.write(START_FLAG + END_FLAG);
}

bool errCheck(unsigned char * input){
  int tot = 0;
  int len = 2;
  if(input[1] == GET_COUNT) len = 3;
  else if(input[1] == SET_COLOR) len = 5;
  else if(input[1] == GET_STATES) len = (input[2] / 7) + 4;
  for(int i = 0; i < len; i++) {
    tot += input[i];
  }
  return ((tot & 0b01111111) == input[len]);
}

void setup()
{
  // set the data rate for the SoftwareSerial port
  serial.begin(19200);

  pinMode(sensorPin,INPUT);
  strip.begin();
  setEmptyColor();
  strip.show();
}

void setActive() {
  strip.setPixelColor(0,activeColor);
  strip.show();
}

void resetActive() {
  if(digitalRead(sensorPin)) setEmptyColor();
  else setOccColor();
  strip.show();
}

bool commandStarted = false;

void parseSerial(unsigned char * input, int pntr){
    int addr = input[0] & 0b01111111;
    if(addr == REPORT){
      for(int i=0; i<pntr; i++){
        serial.write(input[i]);
      }
      serial.write(START_FLAG+END_FLAG);
    } else if(errCheck(input)){
      if(addr == address || addr == 127){
        unsigned char cmd = input[1];
        switch(cmd){
          case (GET_COUNT):
            address = input[2] + 1; 
            sendPacket(input[0],cmd, address);
            break;
          case (SET_ACTIVE):
            setActive();
            sendPacket(input[0], cmd);
            break;
          case (RESET_ACTIVE):
            resetActive();
            sendPacket(input[0], cmd);
            break;
          case (SET_COLOR):{
              pCol[0] = input[2] * 2;
              pCol[1] = input[3] * 2;
              pCol[2] = input[4] * 2;
              if(digitalRead(sensorPin)) setEmptyColor();
              else setOccColor();
              sendPacket(input[0], cmd,input[2], input[3], input[4]);
          }
            break;
          case (GET_STATES):{
              int numBoards = input[2];
              int tot = (START_FLAG | addr) + GET_STATES + numBoards;
              serial.write(START_FLAG | addr);
              serial.write(GET_STATES);
              serial.write(numBoards);

              for(int j = 0; j  < (numBoards / 7) + 1; j++){
                char state = input[3 + j];
                if(j == (address - 1) / 7){
                  bitWrite(state, (address - 1) % 7, prevState);
                }
            
                tot+= state;
                serial.write(state);
              }
            
              serial.write(tot & 0b01111111);
              serial.write(START_FLAG + END_FLAG);
          }
            break;
          default:
            break;
        }
      } else {
        for(int i=0; i<pntr; i++){
          serial.write(input[i]);
        }
        serial.write(START_FLAG+END_FLAG);
      }
    } else {
      //sendPacket(REPORT,input[1],input[2],pntr);
      int tot = (START_FLAG | REPORT);
      serial.write(START_FLAG | REPORT);
      serial.print("Cell ");
      serial.print(address);
      serial.print(" had an error: ");
      for(int i=0; i<pntr; i++){
        int dataToWrite = input[i] & 0b01111111;
        serial.print(dataToWrite);
        serial.print(" ");
      }
      serial.write(START_FLAG + END_FLAG);
    }
}

bool packStarted = false;

void loop() // run over and over
{
  if(digitalRead(sensorPin) != prevState){
    if(!prevState) setEmptyColor();
    else setOccColor();
    strip.show();
    prevState = !prevState;
  }

  
  while (serial.available()) {
    int inChar = serial.read();
    if(inChar > START_FLAG + END_FLAG){
      packStarted = true;
      pnt = 1;
      data[0] = inChar;
    } else if (inChar == START_FLAG + END_FLAG){
      if(pnt>0 && packStarted) parseSerial(data,pnt);
      packStarted = false;
      pnt = 0;
    } else if(pnt < len && packStarted) data[pnt++] = inChar;
    else if(pnt >= len) pnt = 0;
  }
}

