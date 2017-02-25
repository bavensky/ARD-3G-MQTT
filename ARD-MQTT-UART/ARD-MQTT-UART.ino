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

#define _binID 1234
#define SS_pin  A5
#define _volume_OK            1<<0
#define _lidStatus_OK         1<<1
#define _temp_OK              1<<2
#define _humid_OK             1<<3
#define _flameStatus_OK       1<<4
#define _soundStatus_OK       1<<5
#define _carbon_OK            1<<6
#define _methane_OK           1<<7
#define _light_OK             1<<8
#define _pitch_OK             1<<9
#define _roll_OK              1<<10
#define _press_OK             1<<11
uint16_t is_data_OK = 0;

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

float _volume, _pitch, _roll;
float _temp, _humid, _lat, _lon, _alt;
float _lidStatus, _flameStatus, _soundStatus;
float  _light, _carbon, _methane;
float _press;
byte _batt = 0;
byte count = 0;
//////////////////////////////UART////////////////////////////////
void serialEvent() {
  while (Serial.available()) {
    RX_buffer[RX_pointer] = Serial.read();
    RX_pointer++;
    if (RX_pointer >= RX_buffer_size)RX_pointer = 0;
  }
}

uint8_t Decode(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[3] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  int16_t data_buffer = 0;
  uint8_t OK = 0;
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
      OK = 1;
    }
  }
  return OK;
}

uint8_t Decode_press(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[3] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  int16_t data_buffer = 0;
  uint8_t OK = 0;
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
      OK = 1;
    }
  }
  return OK;
}
//////////////////////////////calVoltage////////////////////////////////
float calculate_initial_capacity_percentage(float voltage)
{
  float capacity = 0;
  if (voltage > 4.2) {
    capacity = 100;
  } else if (voltage < 3.2 ) {
    capacity = 0;
  } else if (voltage > 4 ) {
    capacity = 80 * voltage - 236;
  } else if (voltage > 3.67 ) {
    capacity = 212.53 * voltage - 765.29;
  } else {
    capacity = 29.787234 * voltage - 95.319149 ;
  }
  return capacity;
}
//////////////////////////////mainSETUP////////////////////////////////
void setup()  {
  Serial.begin(9600);
  Serial.print(F("freeRam = "));
  Serial.println(freeMemory());
  pinMode(SS_pin, OUTPUT);
  /////////////////////////////3G//////////////////////////////////
  gsm.begin(&mySerial, 9600);
  Serial.println(F("Init..."));
  gsm.PowerOn();
  while (gsm.WaitReady()) {}
  Serial.println(millis() / 1000);
  //////////////////////////////GPS//////////////////////////////
  Serial.println(F("Start GPS"));
  gps.Start();
  Serial.println(F("Enable NMEA"));
  gps.EnableNMEA();

  gps_data = gps.GetNMEA("GGA");
  Serial.println(gps.GetNMEA("GGA"));

  while (gps_data.substring(0, 8) == "$GPGGA,," || gps_data.substring(0, 8) == "Please W" || count == 10) {// || gps_data.substring(0, 8) == "$GPGGA,1") {
    gps_data = gps.GetNMEA("GGA");
    Serial.println(gps_data);
    delay(3000);
    count += 1;
  }

  Serial.print(F("Done... "));
  //  Serial.println(gps.GetNMEA("GGA"));

  //  gps_data = gps.GetNMEA("GGA");
  if (gps_data.substring(0, 6) == "$GPGGA" && count != 30) {
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
    Serial.println(F("Stop GPS"));
    gps.Stop();
    gps.DisableNMEA();
    Serial.println(millis() / 1000);
  }
  //////////////////////////////MQTT////////////////////////////////
  Serial.println(F("MQTT Connected"));
  net.DisConnect();
  net.Configure(APN, USER, PASS);
  net.Connect();
  do  {
    Serial.println(F("Connect Server"));
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
  Serial.println(millis() / 1000);
  // mqtt first sent
  mqtt.Publish("/SmartTrash/gearname/binID", String(_binID), false);
  String data_s4 = gps_lat + ","  + gps_lon + "," + gps_alt;
  mqtt.Publish("/SmartTrash/gearname/binID/gps", data_s4, false);
}
//////////////////////////////mainLOOP////////////////////////////////
void loop() {
  unsigned long current = millis();
  if (current - prev > 1000) {
    prev = current;
    Serial.println(F("Decode..."));
    while (Decode_pointer != RX_pointer) {
      if (Decode(&_volume, 0xff)) is_data_OK |= _volume_OK;
      if (Decode(&_lidStatus, 0xfe)) is_data_OK |= _lidStatus_OK;
      if (Decode(&_temp, 0xfd)) is_data_OK |= _temp_OK;
      if (Decode(&_humid, 0xfc)) is_data_OK |= _humid_OK;
      if (Decode(&_flameStatus, 0xfb)) is_data_OK |= _flameStatus_OK;
      if (Decode(&_soundStatus, 0xfa)) is_data_OK |= _soundStatus_OK;
      if (Decode_press(&_carbon, 0xf9)) is_data_OK |= _carbon_OK;
      if (Decode_press(&_methane, 0xf8)) is_data_OK |= _methane_OK;
      if (Decode_press(&_light, 0xf7)) is_data_OK |= _light_OK;
      if (Decode(&_pitch, 0xf6)) is_data_OK |= _pitch_OK;
      if (Decode(&_roll, 0xf5)) is_data_OK |= _roll_OK;
      if (Decode_press(&_press, 0xf4)) is_data_OK |= _press_OK;

      Decode_pointer++;
      if (Decode_pointer >= RX_buffer_size)Decode_pointer = 0;
    }
    //    Serial.print(F("data = ")); Serial.print(_volume);
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
    //    Serial.print(F(" "));  Serial.println(_press);
  }

  if (is_data_OK == 0x0fff) {
    _batt = calculate_initial_capacity_percentage((float)analogRead(A0) * 0.00488758553f);
    Serial.print(F("MQTT..."));
    String data_s1 = String(_volume) + "," + String(_lidStatus) + "," + String(_temp) + ","
                     + String(_humid) + "," + String(_flameStatus);
    String data_s2 = String(_pitch) + "," + String(_roll) + "," + String(_press) + "," + String(_batt);
    String data_s3 = String(_soundStatus) + "," + String(_carbon) + "," + String(_methane) + "," + String(_light);

    // mqtt data sent
    mqtt.Publish("/SmartTrash/gearname/binID/data1", data_s1, false);
    //    delay(1000);
    mqtt.Publish("/SmartTrash/gearname/binID/data2", data_s2, false);
    //    delay(1000);
    mqtt.Publish("/SmartTrash/gearname/binID/data3", data_s3, false);
    delay(1000);
    Serial.println(F("Done"));
    digitalWrite(SS_pin, HIGH); // tell STM32 Mission Complete
    Serial.println(millis() / 1000);
    gsm.PowerOff();
    while (1);
  }
}
//////////////////////////////ENDmainLOOP////////////////////////////////
