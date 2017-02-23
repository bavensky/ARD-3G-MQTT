#include "TEE_UC20.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include "gnss.h"
#include <MemoryFree.h>

GNSS gps;
INTERNET net;
UCxMQTT mqtt;

//SIM AIS  internet
#define APN "internet"
#define USER ""
#define PASS ""

// MQTT init
#define MQTT_SERVER      "gb.netpie.io"
#define MQTT_PORT        "1883"
#define MQTT_ID          "gRYY1BDQDwDbAhU6"
#define MQTT_USER        "5yixIOa4YKKqo71"
#define MQTT_PASSWORD    "Dvb1hYXuWmrRyYfO8w5Vu8PbxKM="
#define MQTT_WILL_TOPIC    0
#define MQTT_WILL_QOS      0
#define MQTT_WILL_RETAIN   0
#define MQTT_WILL_MESSAGE  0

AltSoftSerial mySerial;

#define RX_buffer_size 100
volatile uint8_t RX_buffer[RX_buffer_size] = {0};
volatile uint16_t  RX_pointer = 0;
volatile uint16_t  Decode_pointer = 0;
unsigned long prev = 0;

volatile char GNSS_data[75] = "";
String gps_data = "";
String gps_lat = "";
String gps_lon = "";
String gps_alt = "";

float _binID, _volume, _pitch, _roll, _batt;
float _temp, _humid, _lat, _lon, _alt;
float _lidStatus, _flameStatus, _soundStatus;
float  _light, _carbon, _methane;
float _press;
byte count = 0;

//////////////////////////////UART////////////////////////////////
void serialEvent() {
  while (Serial.available()) {
    RX_buffer[RX_pointer] = Serial.read();
    RX_pointer++;
    if (RX_pointer >= RX_buffer_size)RX_pointer = 0;
  }
}

void Decode(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[3] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  int16_t data_buffer = 0;
  for (int8_t i = 0; i < 3 ; i++)
  {
    tmp_header[i] = RX_buffer[tmp_pointer];
    tmp_pointer++;
    if (tmp_pointer >= RX_buffer_size)tmp_pointer = 0;
  }

  if ((tmp_header[0] & tmp_header[1] & tmp_header[2]) == header)
  {
    tmp_data_pointer[0] = (uint8_t)RX_buffer[tmp_pointer + 0];
    tmp_data_pointer[1] = (uint8_t)RX_buffer[tmp_pointer + 1];
    tmp_data_pointer[2] = (uint8_t)RX_buffer[tmp_pointer + 2];

    if ((uint8_t)tmp_data_pointer[0] + (uint8_t)tmp_data_pointer[1] == (uint8_t)tmp_data_pointer[2])
    {
      uint16_t tmp = (uint16_t)tmp_data_pointer[0] << 8 | (uint16_t)tmp_data_pointer[1];
      * value = (float)(tmp) * 0.01f;
    }
  }
}

void Decode_press(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[3] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  int16_t data_buffer = 0;
  for (int8_t i = 0; i < 3 ; i++)
  {
    tmp_header[i] = RX_buffer[tmp_pointer];
    tmp_pointer++;
    if (tmp_pointer >= RX_buffer_size)tmp_pointer = 0;
  }

  if ((tmp_header[0] & tmp_header[1] & tmp_header[2]) == header)
  {
    tmp_data_pointer[0] = (uint8_t)RX_buffer[tmp_pointer + 0];
    tmp_data_pointer[1] = (uint8_t)RX_buffer[tmp_pointer + 1];
    tmp_data_pointer[2] = (uint8_t)RX_buffer[tmp_pointer + 2];

    if ((uint8_t)tmp_data_pointer[0] + (uint8_t)tmp_data_pointer[1] == (uint8_t)tmp_data_pointer[2])
    {
      uint16_t tmp = (uint16_t)tmp_data_pointer[0] << 8 | (uint16_t)tmp_data_pointer[1];
      * value = (float)(tmp);
    }
  }
}

//////////////////////////////mainSETUP////////////////////////////////
void setup()  {
  Serial.begin(9600);
  Serial.print(F("freeRam = "));
  Serial.println(freeMemory());
  pinMode(6, OUTPUT);
  digitalWrite(6, 0);
  /////////////////////////////3G//////////////////////////////////
  gsm.begin(&mySerial, 9600);
  Serial.println(F("Init..."));
  gsm.PowerOn();
  while (gsm.WaitReady()) {}
  net.DisConnect();
  net.Configure(APN, USER, PASS);
  net.Connect();
  //////////////////////////////GPS//////////////////////////////
  Serial.println(F("Start GPS"));
  gps.Start();
  Serial.println(F("Enable NMEA"));
  gps.EnableNMEA();

  String gps_s = gps.GetNMEA("GGA");
  Serial.println(gps.GetNMEA("GGA"));
  while (gps_s.substring(0, 8) == "$GPGGA,," || gps_s.substring(0, 8) == "Please W" || gps_s.substring(0, 8) == "$GPGGA,1") {
    gps_s = gps.GetNMEA("GGA");
    digitalWrite(6, 1);
    delay(3000);
    digitalWrite(6, 0);
  }
  Serial.print(F("Done... "));
  Serial.println(gps.GetNMEA("GGA"));

  gps_data = gps.GetNMEA("GGA");
  if (gps_data.substring(0, 6) == "$GPGGA") {
    gps_data.toCharArray(GNSS_data, 75);

    String gps_cal_s = gps_data.substring(18, 27);
    float gps_cal_f = gps_cal_s.toFloat();
    gps_cal_f /= 60.0f;
    uint32_t gps_cal_l = gps_cal_f * 10000000;

    gps_lat += GNSS_data[16] ;
    gps_lat += GNSS_data[17] ;
    gps_lat += ".";
    gps_lat += gps_cal_l;
    gps_lat += GNSS_data[28] ;

    gps_cal_s = gps_data.substring(33, 42);
    gps_cal_f = gps_cal_s.toFloat();
    gps_cal_f /= 60.0f;
    gps_cal_l = gps_cal_f * 10000000;

    gps_lon += GNSS_data[30] ;
    gps_lon += GNSS_data[31] ;
    gps_lon += GNSS_data[32] ;
    gps_lon += ".";
    gps_lon += gps_cal_l;
    gps_lon += GNSS_data[43] ;

    gps_alt += GNSS_data[54];
    gps_alt += GNSS_data[55];
    gps_alt += GNSS_data[56];
    gps_alt += GNSS_data[57];
    gps_alt += GNSS_data[58];

    Serial.print(gps_lat);
    Serial.print(F("  "));
    Serial.print(gps_lon);
    Serial.print(F("  "));
    Serial.println(gps_alt);
    delay(1000);
  }
  //////////////////////////////MQTT////////////////////////////////
  do  {
    //    Serial.println(F("Connect Server"));
    //    Serial.println(F("wait connect"));
    if (mqtt.DisconnectMQTTServer())
    {
      mqtt.ConnectMQTTServer(MQTT_SERVER, MQTT_PORT);
    }
    delay(500);
    Serial.println(mqtt.ConnectState());
  }
  while (!mqtt.ConnectState());
  Serial.println(F("Server Connected"));
  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  Serial.println(mqtt.ConnectReturnCode(ret));
}
//////////////////////////////mainLOOP////////////////////////////////
void loop() {
  digitalWrite(6, 1);
  delay(100);
  digitalWrite(6, 0);
  delay(100);

  unsigned long current = millis();
  if (current - prev > 1000) {
    prev = current;
    Serial.println(F("Decode..."));
    while (Decode_pointer != RX_pointer) {
      Decode(&_binID, 0xff);
      Decode(&_volume, 0xfe);
      Decode(&_lidStatus, 0xfd);
      Decode(&_temp, 0xfc);
      Decode(&_humid, 0xfb);
      Decode(&_flameStatus, 0xfa);
      Decode(&_soundStatus, 0xf9);
      Decode_press(&_carbon, 0xf8);
      Decode_press(&_methane, 0xf7);
      Decode_press(&_light, 0xf6);
      Decode(&_pitch, 0xf5);
      Decode(&_roll, 0xf4);
      Decode_press(&_press, 0xf3);
      Decode(&_batt, 0xf2);
      Decode_pointer++;
      if (Decode_pointer >= RX_buffer_size)Decode_pointer = 0;
    }
    //    Serial.print(F("data = "));  Serial.print(_binID);
    //    Serial.print(F(" "));  Serial.print(_volume);
    //    Serial.print(F(" "));  Serial.print(_lidStatus);
    //    Serial.print(F(" "));  Serial.print(_temp);
    //    Serial.print(F(" "));  Serial.print(_humid);
    //    Serial.print(F(" "));  Serial.print(_flameStatus);
    //    Serial.print(F(" "));  Serial.print(_soundStatus);
    //    Serial.print(F(" "));  Serial.print(_carbon);
    //    Serial.print(F(" "));  Serial.print(_methane);
    //    Serial.print(F(" "));  Serial.print(_light);
    //    Serial.print(F(" "));  Serial.print(_pitch);
    //    Serial.print(F(" "));  Serial.print(_roll);
    //    Serial.print(F(" "));  Serial.print(_press);
    //    Serial.print(F(" "));  Serial.println(_batt);
  }

  if (_binID != 0 && _volume != 0 && _lidStatus != 0 && _temp != 0 && _humid != 0
      && _flameStatus != 0 && _soundStatus != 0 && _carbon != 0 && _methane != 0
      &&  _light != 0 && _pitch != 0 && _roll != 0 && _press != 0  && _batt != 0 )
  {
    String data_s1 = String(_volume) + "," + String(_lidStatus) + "," + String(_temp) + ","
                     + String(_humid) + "," + String(_flameStatus) + "," + String(_soundStatus) + "," +
                     String(_carbon) + "," + String(_methane) + "," + String(_light);
    String data_s2 = String(_pitch) + "," + String(_roll) + "," + String(_press) + "," + String(_batt);
    String data_s3 = gps_lat + ","  + gps_lon + "," + gps_alt;

    Serial.println(F("MQTT..."));

    mqtt.Publish("/SmartTrash/gearname/binID/id", String(_binID), false);
    mqtt.Publish("/SmartTrash/gearname/binID/data1", data_s1, false);
    mqtt.Publish("/SmartTrash/gearname/binID/data2", data_s2, false);
    mqtt.Publish("/SmartTrash/gearname/binID/data3", data_s3, false);
    delay(2000);
    gsm.PowerOff();
    while (1);
  }
}
//////////////////////////////ENDmainLOOP////////////////////////////////
