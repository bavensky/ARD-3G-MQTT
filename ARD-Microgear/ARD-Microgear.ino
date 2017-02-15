#include "TEE_UC20.h"
#include "SoftwareSerial.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "tcp.h"
#include <MicroGear.h>

INTERNET net;
TCP tcp;

#define APPID   "SmartTrash"
#define KEY     "EG7CXWfd9T9Caa8"
#define SECRET  "AlPygVXer9pBXV3uw41Km5SI5"
#define ALIAS   "anything"


//SIM TRUE  internet
#define APN "internet"
#define USER ""
#define PASS ""

AltSoftSerial mySerial;

MicroGear *_migrogear = NULL;

void debug(String data)
{
  Serial.println(data);
}
void setup() 
{
  Serial.begin(9600);
  gsm.begin(&mySerial,9600);
  gsm.Event_debug = debug;
  Serial.println(F("UC20"));
  gsm.PowerOn(); 
  while(gsm.WaitReady()){}
 
  Serial.print(F("GetOperator --> "));
  Serial.println(gsm.GetOperator());
  Serial.print(F("SignalQuality --> "));
  Serial.println(gsm.SignalQuality());
  
  Serial.println(F("Disconnect net"));
  net.DisConnect();
  Serial.println(F("Set APN and Password"));
  net.Configure(APN,USER,PASS);
  Serial.println(F("Connect net"));
  net.Connect();
  Serial.println(F("Show My IP"));
  Serial.println(net.GetIP());
  
  MicroGear microgear(gsm);
  _migrogear = &microgear;
  microgear.init(KEY, SECRET, ALIAS);
  microgear.connect(APPID);

}
void loop() 
{
   _migrogear->loop();
}
