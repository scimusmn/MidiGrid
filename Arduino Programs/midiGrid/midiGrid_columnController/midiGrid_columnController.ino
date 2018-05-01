#include <AltSoftSerial.h>

#define ROW_COLOR 48
#define ERR_CODE 126
#define SUCCESS 127
#define REPORT 126
const int BROADCAST = 127;
const int COMMAND = 128;
const int SEND_COUNT = 64;
const int GET_STATES = 16;
const int SET_COLOR = 32;
const int SET_DEFAULT_COLOR = 96;
const int SET_COLUMN_ACTIVE = 96;
const int RESET_COLUMN = 32;
const int END_FLAG = 0;
const int COMPLETE = 1;
const int SET_ACTIVE = 4;
const int RESET_ACTIVE = 2;

class serialBuffer {
public:

  int length;
  unsigned char  *input;
  int pntr;
  bool stringComplete;
  bool packStarted;
  unsigned char endByte;
  void (*endCB)(unsigned char  *, int);

  serialBuffer(unsigned char endChar){
    length = 128;
    input = new unsigned char[length];
    pntr = 0;
    stringComplete= packStarted = false;
    endByte = endChar;
  }

  void setCallback(void (*CB)(unsigned char  *,int)){
    endCB = CB;
  }

  void push(unsigned char inChar){
    //if (next == endByte) stringComplete = true;
    //else input[pntr++] = next;

    if(inChar > endByte){
      packStarted = true;
      pntr = 1;
      input[0] = inChar;
    } else if (inChar == endByte){
      if(pntr>0 && packStarted) endCB(input,pntr);
      packStarted = false;
      pntr = 0;
    } else if(pntr < length && packStarted) input[pntr++] = inChar;
    else if(pntr >= length) pntr = 0;

//    if (stringComplete&&pntr){
//      endCB(input,pntr);
//      stringComplete = false;
//      pntr = 0;
//      
//    }
  }
};

int address = -1;

int numBoards = 0;
int * boardStates;

unsigned long countTimer = 0;
unsigned long colorTimer = 0;

unsigned long beginTimer = 0;

unsigned long activeTimer = 0;
unsigned long resetTimer = 0;

bool writingActive = false;
bool writingReset = false;
bool writingColor = false;
bool countRequested = false;
bool statesRequested = false;

int writing = 0;

bool defSet = false;
bool firstRead = true;

int loopColor = 0;

bool scheduleActive = false;
bool scheduleReset = false;
bool scheduleRead = false;
bool scheduleCount = false;
bool scheduleColor = false;

bool active = false;

AltSoftSerial serial; //8 RX, 9 TX
//SoftwareSerial serial(11, 10); // RX, TX

void sendCellPacket(char addr, char cmd, byte data = 128, byte data2 = 128, byte data3 = 128, byte data4 = 128){
  int tot = (COMMAND | addr) + cmd;
  serial.write(COMMAND + END_FLAG);
  serial.write(COMMAND | addr);
  serial.write(cmd);
  
  //digitalWrite(13,!digitalRead(13));
  if(data < 128) tot+=data, serial.write(data);
  if(data2 < 128) tot+=data2, serial.write(data2);
  if(data3 < 128) tot+=data3, serial.write(data3);
  if(data4 < 128) tot+=data4, serial.write(data4);
  
  serial.write(tot & 0b01111111);
  serial.write(COMMAND + END_FLAG);
}

bool cellErrCheck(unsigned char * input){
  int tot = 0;
  int len = 2;
  if(input[1] == SEND_COUNT) len = 3;
  else if(input[1] == SET_COLOR) len = 5;
  else if(input[1] == GET_STATES) len = (input[2] / 7) + 4;
  for(int i = 0; i < len; i++) {
    tot += input[i];
  }
  return ((tot & 0b01111111) == input[len]);
}


void setSensorColor(int which, int r, int g, int b){
  sendCellPacket(which, SET_COLOR, r, g, b);
}

void setDefaultColor(int which, int r, int g, int b){
  //writingColor = true;
  Serial.write(COMMAND | REPORT);
  Serial.print("Try setting color on ");
  Serial.print(which);
  Serial.write(COMMAND + END_FLAG);
  sendCellPacket(which, SET_COLOR, r, g, b);
}

void requestSensorStates(){
  if(numBoards>0){
    //statesRequested = true;
    int tot = (COMMAND | BROADCAST) + GET_STATES + numBoards;
    serial.write(COMMAND | BROADCAST);
    serial.write(GET_STATES);
    serial.write(numBoards);
    
    for(int i=0; i < (numBoards / 7) + 1; i++){
      serial.write((byte)0x00);
    }
  
    serial.write(tot & 0b01111111);
    serial.write(COMMAND + END_FLAG);
  }
}

void requestCount(){
  //countRequested = true;
  sendCellPacket(BROADCAST, SEND_COUNT, (byte)0x00);
}

void setActive(){
  //writingActive = true;
  activeTimer = millis();
  sendCellPacket(BROADCAST, SET_ACTIVE);
  
}

void checkActiveWrite(){
  if(writingActive && activeTimer + 20 < millis()){
    //setActive();
    //Serial.println("Didn't set active in time");
  }
}

void resetActive(){
  //writingReset = true;
  resetTimer = millis();
  sendCellPacket(BROADCAST, RESET_ACTIVE);
}

void checkResetWrite(){
  if(writingReset && resetTimer + 20 < millis()){
    resetActive();
    //Serial.println("Didn't reset active in time");
  }
}

void sendStates(){
  int tot = (COMMAND | address) + GET_STATES + numBoards;
  Serial.write(COMMAND | address);
  Serial.write(GET_STATES);
  Serial.write(numBoards);
  
  for(int j = 0; j  < (numBoards / 7) + 1; j++){
    int states = 0;
    for( int k =0; k < 7 && k + j*7 < numBoards; k++){
      bitWrite(states, k, boardStates[j * 7 + k]);
    }

    tot+= states;
    Serial.write(states);
  }

  Serial.write(tot & 0b01111111);
  Serial.write(COMMAND + END_FLAG);
}

void cellSerialHandle(unsigned char * input, int pntr){
  int addr = input[0] & 0b01111111;
  if(addr == REPORT){
    Serial.write(COMMAND | REPORT);
    Serial.print("Column ");
    Serial.print(address);
    Serial.print(", ");
    for(int i=1; i<pntr; i++){
      Serial.write(input[i]);
    }
    Serial.write(COMMAND + END_FLAG);
  } else if(cellErrCheck(input)){
    unsigned char cmd = input[1];
    switch(cmd){
      case (SEND_COUNT):{
          numBoards = input[2];
          boardStates = new int[numBoards];

          Serial.write(COMMAND | REPORT);
          Serial.print("Number of boards in column ");
          Serial.print(address);
          Serial.print(":");
          Serial.print(numBoards);
          Serial.write(COMMAND + END_FLAG);
        }
        break;
      case (SET_ACTIVE):
        //writingActive = false;
        digitalWrite(13,HIGH);
        active = true;
        break;
      case (RESET_ACTIVE):
        //writingReset = false;
        digitalWrite(13,LOW);
        active = false;
        break;
      case (SET_COLOR):{
        Serial.write(COMMAND | REPORT);
        Serial.print("Set color on ");
        Serial.print(addr);
        Serial.write(COMMAND + END_FLAG);
        //writingColor = false;
      }
        break;
      case (GET_STATES):{
          int curBoard = 0;
          bool changed = false;
          for( int j = 0; j < (numBoards / 7) + 1; j++){
            int nex = input[3 + j];
            
            do {
              int curVal = bitRead(nex,curBoard % 7);
              if(boardStates[curBoard] != curVal){
                boardStates[curBoard] = curVal;
                //////////
                changed = true;
                digitalWrite(13,curVal);
                 /////////
              }               
              curBoard++;
            } while(curBoard % 7 && curBoard <= numBoards);
          }

          if((changed || firstRead) && address>0) {
            firstRead = false;
            sendStates();
          }
      }
        break;
      default:
        break;
    }
  } else {
    Serial.write(COMMAND | REPORT);
    Serial.print("Column ");
    Serial.print(address);
    Serial.print(" had an error:");
    for(int i=0; i<pntr; i++){
      Serial.print(input[i]);
      Serial.print(" ");
    }
    Serial.write(COMMAND + END_FLAG);
  }
  writing = 0;
}

void sendPacket(char addr, char cmd, byte data = 128, byte data2 = 128, byte data3 = 128, byte data4 = 128){
  int tot = (COMMAND | addr) + cmd;
  Serial.write(COMMAND | addr);
  Serial.write(cmd);
  
  //digitalWrite(13,!digitalRead(13));
  if(data < 128) tot+=data, Serial.write(data);
  if(data2 < 128) tot+=data2, Serial.write(data2);
  if(data3 < 128) tot+=data3, Serial.write(data3);
  if(data4 < 128) tot+=data4, Serial.write(data4);
  
  Serial.write(tot & 0b01111111);
  Serial.write(COMMAND + END_FLAG);
}

bool errCheck(unsigned char * input){
  int tot = 0;
  int len = (input[1] == SEND_COUNT)?3:(input[1] == ROW_COLOR)?6:2;
  for(int i = 0; i < len; i++) {
    tot += input[i];
  }
  return ((tot & 0b01111111) == input[len]);
}

int whichCell = 0;
unsigned char r = 0;
unsigned char g = 0;
unsigned char b = 0;

void setColor(){
  setDefaultColor(whichCell,r,g,b);
}

void columnSerialHandle(unsigned char * input, int pntr){
  int addr = input[0] & 0b01111111;
  if(addr == address || addr == 127){
    if(errCheck(input)){
      unsigned char cmd = input[1];
      switch(cmd){
        case (SEND_COUNT):
          address = input[2] + 1;
          beginTimer = millis();
          sendPacket(input[0],cmd, address);
          //requestCount();
          scheduleCount = true;
          break;
        case (SET_COLUMN_ACTIVE):
          //setActive();
          scheduleActive = true;
          sendPacket(input[0], cmd);
          break;
        case (RESET_COLUMN):
          //resetActive();
          scheduleReset = true;
          sendPacket(input[0], cmd);
          break;
        case (ROW_COLOR):{
            whichCell = input[2];
            r = input[3];
            g = input[4];
            b = input[5];
            //setDefaultColor(which,r,g,b);
            scheduleColor = true;
            sendPacket(input[0], cmd, whichCell, r, g, b);
            return;
        }
          break;
        default:
          break;
      }
    } else {
      sendPacket(REPORT,input[1],input[2],pntr);
    }
  } else {
    for(int i=0; i<pntr; i++){
      Serial.write(input[i]);
    }
    Serial.write(COMMAND+END_FLAG);
  }
}

serialBuffer cellSerial(COMMAND + END_FLAG);
serialBuffer columnSerial(COMMAND + END_FLAG);

void setup()
{
  // set the data rate for the SoftwareSerial port
  serial.begin(19200);
  Serial.begin(115200);

  
  pinMode(13,OUTPUT);
  //digitalWrite(13,LOW);
  

  cellSerial.setCallback(cellSerialHandle);
  columnSerial.setCallback(columnSerialHandle);
  
}

unsigned long writeTimer = 0;

bool* schedules[5] = {&scheduleCount, &scheduleColor, &scheduleReset, &scheduleActive, &scheduleRead};
void (* callbacks [5])() = {requestCount, setColor, resetActive,setActive, requestSensorStates};

void loop() {

  if(countTimer < millis() && address > 0  && beginTimer + 5000 < millis()){
    countTimer = millis() + 500;
    //requestSensorStates();
    scheduleRead = true;
  }

  if(writeTimer < millis() && writing){
    if(writing != 5 && writing != 3){
      Serial.write(COMMAND | REPORT);
      Serial.print("Error: Didn't receive verification on  ");
      Serial.print(writing);
      Serial.write(COMMAND + END_FLAG);
    }
    
    if(writing == 1) scheduleCount = true;
    else if(writing == 2) scheduleColor = true;
    //else if(writing == 3) scheduleReset = true;
    writing = 0;
  }

  //checkResetWrite();
  //checkActiveWrite();
  

  if(!writing){
    for(int i=0; i<5; i++){
      if(*(schedules[i])){
        callbacks[i]();
        *(schedules[i]) = false;
        writing = i+1;
        writeTimer = millis() + 75;
        break;
      }
    }
  }


  
  while (serial.available()) cellSerial.push(serial.read());
}

void serialEvent() {
  while(Serial.available()) columnSerial.push(Serial.read());
}

