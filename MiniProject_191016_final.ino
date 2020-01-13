//Serial
#include <SoftwareSerial.h>
#define LCDPIN D8
SoftwareSerial mySerial(0, LCDPIN);

//Wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
const char* ssid = "iptime";
const char* password = "";

//NTP Server
#include <WiFiUdp.h>
#include <NTPClient.h> 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
//NTPClient timeClient(ntpUDP, "kr.pool.ntp.org", utcOffsetInSeconds);
//const long utcOffsetInSeconds = 3600; 
WiFiClient client;
char daysOfTheWeek[7][12] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
char getday[20];

//ThingSpeak
#include <ThingSpeak.h>
//unsigned long ChannelID = 879928; //JJS
//const char* WriteAPIKey = "2EK2OMJ9WQ7EU41F";
unsigned long ChannelID = 879749; // JCD
const char* WriteAPIKey = "B7B03JQPVGMZ1YGT";

#include <Wire.h>

//Sensor BH1750(I2C)
#include <BH1750FVI.h>
BH1750FVI BH_1750(BH1750FVI::k_DevModeContLowRes);
uint16_t BH_1750lux = 0;

//Sensor BMP180(I2C)
#include <BMP180I2C.h>
#define I2C_addr 0x77
BMP180I2C BMP_180(I2C_addr);
int BMP_180tem = 0;
int BMP_180pre = 0;

//Sensor DHT11
#include <DHT.h>
#define DHTPIN D5
#define DHTTYPE DHT11
DHT DHT_11(DHTPIN, DHTTYPE);
uint8_t DHT_11tem, DHT_11hum;

//Sensor MQ135(analog)
#define MQ_135PIN A0
int MQ_135gas = 0;


// Pin_Num D0  D1  D2  D3  D4  D5  D6  D7  D8  A0
//         DHT SCL SDA                     LCD MQ
                                    

void setup() {
  Serial.begin(115200);
  
  Wire.begin();
  BH_1750.begin();
  BMP_180.begin();
  DHT_11.begin();
  BMP_180.resetToDefaults();
  BMP_180.setSamplingMode(BMP180MI::MODE_UHR);
  
  timeClient.begin();
  timeClient.setTimeOffset(32400);
  delay(10);
  initWiFi();
  ThingSpeak.begin(client);
  initPrint();
  while(!timeClient.update()) {
      timeClient.forceUpdate();
  }
  pinMode(D6, OUTPUT);  // GAS ALARM
  pinMode(D7, OUTPUT);  // MOOD LAMP
}

void loop() {
  ReadData();
  PrintTimeDHT();

  ThingSpeak.writeField(ChannelID, 1, BH_1750lux, WriteAPIKey);
  delay(15000); // Sending Data is needed more than 15sec
  PrintTimeDHT();
  CHECK();
  ThingSpeak.writeField(ChannelID, 2, BMP_180tem, WriteAPIKey);
  delay(15000);
  PrintTimeDHT();
  CHECK();
  ThingSpeak.writeField(ChannelID, 3, BMP_180pre, WriteAPIKey);
  delay(15000);
  PrintTimeDHT();
  CHECK();
  ThingSpeak.writeField(ChannelID, 4, DHT_11tem, WriteAPIKey);
  delay(15000);
  PrintTimeDHT();
  CHECK();
  ThingSpeak.writeField(ChannelID, 5, DHT_11hum, WriteAPIKey);
  delay(15000);
  PrintTimeDHT();
  CHECK();
  ThingSpeak.writeField(ChannelID, 6, MQ_135gas, WriteAPIKey);
  delay(15000);
  PrintTimeDHT();
  CHECK();
}

void initWiFi(){
  Serial.println();
  Serial.println();
  Serial.println("Connectiong to ssid ...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }
  Serial.println();
  Serial.println("Connected WiFi");
  Serial.println(WiFi.localIP());
  Serial.println();
}


void ReadData(){
  while(!timeClient.update()) {// ensure that we get a valid date and time
    timeClient.forceUpdate();
  }
    
  BH_1750lux = BH_1750.GetLightIntensity();
  Serial.println(BH_1750lux);
    
  BMP_180.measureTemperature();
  do {
    delay(100);
  } while(!BMP_180.hasValue());
  BMP_180tem = BMP_180.getTemperature();
  Serial.println(BMP_180tem);
  BMP_180.measurePressure();
  do {
    delay(100);
  } while(!BMP_180.hasValue());
  BMP_180pre = BMP_180.getPressure();
  Serial.println(BMP_180pre);

  delay(1000);
  DHT_11tem = DHT_11.readTemperature();
  Serial.println(DHT_11tem);
  DHT_11hum = DHT_11.readHumidity();
  Serial.println(DHT_11hum);
  
  MQ_135gas = analogRead(A0);
  Serial.println(MQ_135gas);
  Serial.println("===========================");

}

void CHECK() {
  MQ_135gas = analogRead(A0);
  if(MQ_135gas >= 240) {
    mySerial.print("$CLEAR \r\n");
    mySerial.print("$GO 1 1\r\n");
    mySerial.print("$PRINT !!!!CAUTION!!!!! \r\n");                    
    mySerial.print("$GO 2 1\r\n");
    mySerial.print("$PRINT !!CHECK THE AIR! \r\n");
    digitalWrite(D6, HIGH);
  } else digitalWrite(D6, LOW);

  BH_1750lux = BH_1750.GetLightIntensity();
  if(BH_1750lux <= 50) {
    digitalWrite(D7, HIGH);
  } else digitalWrite(D7, LOW);
}

void initPrint(){
  mySerial.begin(9600);
  mySerial.print("$CLEAR\r\n");
  mySerial.print("$GO 1 1\r\n");
  mySerial.print("$PRINT JUNG CHANGDO\r\n");
  mySerial.print("$GO 2 1\r\n");
  mySerial.print("$PRINT JI JAESAM\r\n");
  delay(1500);
  mySerial.print("$CLEAR\r\n");
  mySerial.print("$GO 1 1\r\n");
  mySerial.print("$PRINT Start!!\r\n");
  mySerial.print("$GO 2 1\r\n");
  mySerial.print("$PRINT Mini Project\r\n");
}

void PrintTimeDHT(){
  String formattedDate;
  String dayStamp;
  String timeStamp;
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T"); 
  dayStamp = formattedDate.substring(5, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-4);
  char date[20];
  char daytime[20];
  mySerial.print("$CLEAR\r\n");
  mySerial.print("$GO 1 7\r\n");
  sprintf(daytime,"$PRINT %s %s\r\n",timeStamp.c_str(),daysOfTheWeek[timeClient.getDay()]);
  mySerial.printf("%s",daytime);
  mySerial.print("$GO 1 1\r\n");
  sprintf(date,"$PRINT %s \r\n",dayStamp.c_str());
  mySerial.printf("%s", date); 

  char temp[20];
  char humi[20];
  sprintf(temp,"$PRINT T:%d^C\r\n", DHT_11tem);    // num value => str value
  sprintf(humi,"$PRINT H:%d\r\n", DHT_11hum);
  mySerial.print("$GO 2 2\r\n");
  mySerial.printf("%s \r\n",temp);                     // point!!
  mySerial.print("$GO 2 10\r\n");
  mySerial.printf("%s \r\n",humi);
  mySerial.print("$PRINT % \r\n");
}
