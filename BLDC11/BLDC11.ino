#include <Encoder.h>
#define psclr true

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
}



int i = 0;
int dir = 1;
int pos = 0;
int stage = 0;

byte mode = 0; //Start in off.

boolean recievingPacket = false;
int packetLength = 0;
int packetIndex = 0;
int inBuffer[256]; //define as maximum packet length.
void loop() {
  if(Serial.available() > 0)
  {
    byte b = Serial.read(); //Unsigned byte
    //Serial.write(5);
    
    if(!recievingPacket && b == 0) //Not recieving packet & new packet command recieved
    {
        recievingPacket = true;
        packetIndex = 0;
        packetLength = 0;
    }
    else if(recievingPacket && packetIndex == 0) //Catch second byte, first recorded byte, which is message command.
    {
        packetLength = getCommandLength(b); //Write packet length
        inBuffer[packetIndex] = b;
        packetIndex++;
        if(packetLength == 1) //If 1 byte command, finish after first command byte.
        {
          finishPacket();
        }
    }
    else if(recievingPacket && packetIndex < packetLength - 1) //If recieving and in range, just keep adding the bytes.
    {
        inBuffer[packetIndex] = b;
        packetIndex++;
    }
    else if(recievingPacket)//Last expected byte in, Packet is finished, analyze.
    {
        inBuffer[packetIndex] = b;
        finishPacket();
    }
    //Otherwise, not part of packet. Just let it go.
  }

  switch(mode)
  {
    case 0:
    break;
    case 1:
    break;
    case 2:
    break;
    case 3:
    break;
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
  switch(inBuffer[0]) //Switch based on command byte.
  {
    case 1: //ACK
      cmdBuffer[0] = 1;
      sendByteArray(cmdBuffer, 1);
      break;
    case 3://Add shorts
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
      break;
    case 4: //setMode
      mode = inBuffer[1];     
  }
  
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

int getCommandLength(uint8_t command)
{
  switch(command)
  {
    case 1 : return 1;
    case 3 : return 5;
  }
}

int increment(int dir)
{
  wavePos[0] = wrapIntRealitive(wavePos[0] + dir, 0, arrLength-1);
  wavePos[1] = wrapInt(wavePos[1] + dir, 0, arrLength-1);
  wavePos[2] = wrapInt(wavePos[2] + dir, 0, arrLength-1);
  return wavePos[0];
}

int wrapIntRealitive(int x, int low, int high)
{
  
  if(x<low){stage = wrapInt(stage -1, 0, cyclesPerRot - 1); return high;}
  else if(x>high){stage = wrapInt(stage + 1, 0, cyclesPerRot - 1); return low;}
  else{return x;}
}

int wrapInt(int x, int low, int high)
{
  if(x<low){return high;}
  else if(x>high){return low;}
  else{return x;}
}

int sign(double x)
{
  return abs(x)/x;
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
  const int readingsNumber = 20;
  const int duration = 20;
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
