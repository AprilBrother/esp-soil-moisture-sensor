#include <ESP8266WiFi.h>
#include <Wire.h>

String API_KEY ="<YOUR-API-KEY>";
const char* MY_SSID = "<YOUR-SSID>"; 
const char* MY_PWD = "<YOUR-PASSWORD>";

const int PIN_CLK = D5;
const int PIN_SOIL = A0; 
const int PIN_LED = D7;
const char* SERVER = "api.thingspeak.com";
const int TMP_ADDR = 0x48;

int sent = 0;

#define SLEEP_TIME 1200 * 1000 * 1000

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  
  // device address is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));            // sends instruction byte
  Wire.write(0x60);             // sends potentiometer value byte
  Wire.endTransmission();     // stop transmitting
  
  analogWriteFreq(40000);
  analogWrite(PIN_CLK, 400);
  delay(500);
  connectWifi();
}

void loop() {
  digitalWrite(PIN_LED, LOW);
  delay(100);
  digitalWrite(PIN_LED, HIGH);
  float hum=0,temp=0,soil_hum=0;
  Serial.println("Requesting Temperature...");

  // Begin transmission
  Wire.beginTransmission(TMP_ADDR);
  // Select Data Registers
  Wire.write(0X00);
  // End Transmission
  Wire.endTransmission();
  
  delay(500);
  
  // Request 2 bytes , Msb first
  Wire.requestFrom(TMP_ADDR, 2 );
  // Read temperature as Celsius (the default)
  while(Wire.available()) {  
    int msb = Wire.read();
    int lsb = Wire.read();
    Wire.endTransmission();

    int rawtmp = msb << 8 |lsb;
    int value = rawtmp >> 4;
    temp = value * 0.0625 ;
  }

  delay(1000);
  soil_hum = readSoilVal(8);
  Serial.print("Soil_Humidity:");
  Serial.println(soil_hum);
  Serial.print("Temperature:");
  Serial.println(temp);
  delay(5000);
  
  sendData(temp, soil_hum);
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
    ValBuf[i] = analogRead(PIN_SOIL);
    Serial.println(ValBuf[i]);
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


void sendData(float temp, float soil_hum) {  
   WiFiClient client;
  
   if (client.connect(SERVER, 80)) { // use ip 184.106.153.149 or api.thingspeak.com
       Serial.println("WiFi Client connected ");
       
       String postStr = API_KEY;
       postStr += "&field1=0&field2=";
       postStr += String(temp);
       postStr += "&field3=";
       postStr += String(soil_hum);
       postStr += "\r\n\r\n";
       
       client.print("POST /update HTTP/1.1\n");
       client.print("Host: api.thingspeak.com\n");
       client.print("Connection: close\n");
       client.print("X-THINGSPEAKAPIKEY: " + API_KEY + "\n");
       client.print("Content-Type: application/x-www-form-urlencoded\n");
       client.print("Content-Length: ");
       client.print(postStr.length());
       client.print("\n\n");
       client.print(postStr);
       delay(1000);
   }
   
   sent++;
   client.stop();
}
