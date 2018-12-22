#include <Encoder.h>
#define psclr false

#define motor false

//Phase output pins
#define A 8
#define B 9
#define C 10 
#define ISense A0
#define VSense A1
const int VMultiplier = 2;
const int IMultiplier = 1;
const int arrLengths = 2400;

const int arrLength = 300;
const int cyclesPerRot = 7;

int wavePos[3] = {0, (arrLength/3), (2*arrLength/3)}; //Init motor positions in waveform.
int waveForm[arrLength];
int modForm[arrLength];

int phasePwm[3] = {0,0,0}; //Keeps track of phase PWMs.

double angle[arrLength];
Encoder enc(2,3);

void setup() {
  if(psclr)
  {
    //Prescaler
    int myEraser = 7;
    int myPrescaler = 1;
    TCCR2B &= ~myEraser;
    TCCR2B |= myPrescaler;
    TCCR4B &= ~myEraser;
    TCCR4B |= myPrescaler;    
  }
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
}



int i = 0;
int dir = 1;
int pos = 0;
int stage = 0;

byte mode = 0;
boolean recievingPacket = false;
int packetLength = 0;
int packetIndex = 0;
volatile int inBuffer[256]; //define as maximum packet length.
void loop() {
  //PACKET HANDLING BELOW
  if(Serial.available() > 0){
    byte b = Serial.read(); //Unsigned byte
    //Serial.write(5);
    
    if(!recievingPacket && b == 0){ //Not recieving packet & new packet command recieved
        recievingPacket = true;
        packetIndex = 0;
        packetLength = 0;
    }
    else if(recievingPacket && packetIndex == 0){ //Catch second byte, first recorded byte, which is message command.
        packetLength = getCommandLength(b); //Write packet length
        inBuffer[packetIndex] = b;
        packetIndex++;
        if(packetLength == 1){ //If 1 byte command, finish after first command byte.
          finishPacket();
        }
    }
    else if(recievingPacket && packetIndex < packetLength - 1){ //If recieving and in range, just keep adding the bytes.
        inBuffer[packetIndex] = b;
        packetIndex++;
    }
    else if(recievingPacket){//Last expected byte in, Packet is finished, analyze.
        inBuffer[packetIndex] = b;
        finishPacket();
    }
    //Otherwise, not part of packet. Just let it go.
  }
  
  
}

void setMode(byte newMode)
{
  switch(newMode)
  {
    case 0: //Motor stopped mode
      analogWrite(A, 0);
      analogWrite(B, 0);
      analogWrite(C, 0);
      mode = 0;
      break;
    case 1: //Remote Phase Control Mode
      mode = 1;
      break;
  }
}

void setPhase(byte phase, byte pwm)
{
  
  if(mode == 1) //Make sure in correct mode.
  {
    
    if(phase == 1){
      analogWrite(A, (int)pwm);
    } else if(phase == 2){
      analogWrite(B, (int)pwm);
    }else if(phase == 3){
      analogWrite(C, (int)pwm);
    }
  }
  
}

void finishPacket()
{
  parseCommands(); //Parse commands,
  packetIndex = 0;
  recievingPacket = false;
}

void parseCommands()
{

  byte cmdBuffer[256];
  //if(inBuffer[0] == 4){digitalWrite(13, HIGH);}
  byte b = inBuffer[0];
  
  if(b == 1){ //Switch statement wasn't working, so if-else ladder it is.
    sendAck();
    
  } else if(b == 7){ //Setphase
    setPhase(inBuffer[1], inBuffer[2]);
    sendAck();
    
  } else if(b == 3){
    long a = 0;
    long b = 0;
    a |= inBuffer[1] << 8; //Upper part of int.
    a |= inBuffer[2]; //Lower part of int.
    b |= inBuffer[3] << 8;
    b |= inBuffer[4];
    long ret = a+b;
    cmdBuffer[0] = 3;
    cmdBuffer[1] = (uint8_t)(ret >> 24); //Shift down top byte, cast to byte.
    cmdBuffer[2] = (uint8_t)(ret >> 16);
    cmdBuffer[3] = (uint8_t)(ret >> 8);
    cmdBuffer[4] = (uint8_t)ret;
    sendByteArray(cmdBuffer, 5);
    
  } else if(b == 4){
    setMode(inBuffer[1]);
    sendAck();
    
  } else if(b == 5){
    
  }

}

void sendAck()
{
  byte outBuffer[] = {1};
  sendByteArray(outBuffer, 1);
}

void sendByteArray(byte toSend[], int commandLength)
{
  byte outBuffer[commandLength + 1];
  outBuffer[0] = 0;
  for(int i = 0; i < commandLength; i++)
  {
      outBuffer[i+1] = toSend[i]; //Append cmd to outbuffer.
  }
  Serial.write(outBuffer, commandLength + 1);
}

int getCommandLength(byte command)
{
  switch(command)
  {
    case 1 : return 1; //ACK
    case 7 : return 3; //SetPhase
    case 3 : return 5; //Addshorts
    case 4 : return 2; //Setmode
    case 5 : return 1; //GetTotalVals
    case 6 : return 1;
  }
}

double readAmps() //Averages out I readings over 100ms
{
  const int readingsNumber = 20;
  const int duration = 20;
  long sum = 0;
  for(int i = 0; i < readingsNumber; i++)
  {
    sum += analogRead(ISense);
    delay(duration/readingsNumber);
  }
  int avg = (int)(sum/((long)readingsNumber));
  double amps = ((double)(avg - 512) / 512.0) * 5.0;
  return amps;
}

double readVolts()
{
  const int readingsNumber = 20; //Amount of readings
  const int duration = 20; //Time to gather readings.
  long sum = 0;
  
  for(int i = 0; i < readingsNumber; i++)
  {
    sum += analogRead(VSense);
    delay(duration/readingsNumber);
  }
  
  int avg = (int)(sum/((long)readingsNumber));
  double volts = 5.0 * ((double)avg)/523.0;
  return volts;
}
