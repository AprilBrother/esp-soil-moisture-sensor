#include <ESP8266WiFi.h>
#include "DHT.h"

String apiKey ="<YOUR-API-KEY>";
const char* MY_SSID = "<YOUR-SSID>"; 
const char* MY_PWD = "<YOUR-PASSWORD>";

const int pin_clk = 5;
const int pin_soil = A0; 
const int pin_led = 12;
const int pin_read = 2;
const char* server = "api.thingspeak.com";

DHT dht(pin_read, DHT11);

int sent = 0;

#define SLEEP_TIME 1200 * 1000 * 1000

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(pin_clk, OUTPUT);
  pinMode(pin_soil, INPUT);
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_led, LOW);
  pinMode(pin_read, INPUT);
  
  analogWriteFreq(40000);
  analogWrite(pin_clk, 400);
  delay(500);
  connectWifi();
}

void loop() {
  //char buffer[10];
  digitalWrite(pin_led, HIGH);
  delay(500);
  digitalWrite(pin_led, LOW);
  float hum=0,temp=0,soil_hum=0;
  Serial.println("Requesting Temperature and Humidity...");
  

  hum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temp = dht.readTemperature();

  delay(1000);
  soil_hum = readSoilVal(8);
  Serial.print("Soil_Humidity:");
  Serial.println(soil_hum);
  delay(1000);
  
  sendData(hum, temp,soil_hum);
  ESP.deepSleep(SLEEP_TIME, WAKE_RF_DEFAULT);
}

void connectWifi()
{
  Serial.print("Connecting to " + *MY_SSID);
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Connected");
  Serial.println("");  
}//end connect

// Get average value
float readSoilVal(int n) {
  float valtemp = 0;
  float ValBuf[n];
  int i = 0;
  int j = 0;

  for(i=0;i<n;i++){
    ValBuf[i] = analogRead(pin_soil);  
  }

  for(i=0;i<n-1;i++){
    for(j=i+1;j<n;j++){
      if(ValBuf[i]>ValBuf[j]){
        valtemp = ValBuf[i];
        ValBuf[i] = ValBuf[j];
        ValBuf[j] = valtemp;
      }
    }
  }

  valtemp = 0;
  for(i=1;i<n-1;i++){
    valtemp+=ValBuf[i];
  }
  valtemp/=(n-2);
  return valtemp;
}


void sendData(float hum, float temp,float soil_hum) {  
   WiFiClient client;
  
   if (client.connect(server, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
       Serial.println("WiFi Client connected ");
       
       String postStr = apiKey;
       postStr += "&field1=";
       postStr += String(hum);
       postStr += "&field2=";
       postStr += String(temp);
       postStr += "&field3=";
       postStr += String(soil_hum);
       postStr += "\r\n\r\n";
       
       client.print("POST /update HTTP/1.1\n");
       client.print("Host: api.thingspeak.com\n");
       client.print("Connection: close\n");
       client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
       client.print("Content-Type: application/x-www-form-urlencoded\n");
       client.print("Content-Length: ");
       client.print(postStr.length());
       client.print("\n\n");
       client.print(postStr);
       delay(1000);
   
   }//end if
   sent++;
   client.stop();
}//end send
