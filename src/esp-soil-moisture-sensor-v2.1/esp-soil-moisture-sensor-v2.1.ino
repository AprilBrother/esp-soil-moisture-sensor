#include <ESP8266WiFi.h>
#include <Wire.h>

String API_KEY ="<YOUR-API-KEY>";
const char* MY_SSID = "<YOUR-SSID>"; 
const char* MY_PWD = "<YOUR-PASSWORD>";

const int PIN_CLK   = D5;
const int PIN_SOIL  = A0; 
const int PIN_LED   = D7;
const int PIN_SWITCH = D8;

// I2C address for temperature sensor
const int TMP_ADDR  = 0x48;

int sent = 0;

#define SLEEP_TIME 1200 * 1000 * 1000


float readTemperature() {
  float temp;
  
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
    temp = value * 0.0625;

    return temp;
  }
  
  return 0;
}

// Get soil sensor value
float readSoilSensor() {
  float tmp = 0;
  float total = 0;
  float rawVal = 0;
  int sampleCount = 3;

  for(int i = 0; i < sampleCount; i++){
    rawVal = analogRead(PIN_SOIL);
    total+= rawVal;
    Serial.println(rawVal);
  }

  tmp = total / sampleCount;
  return tmp;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);

  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_SWITCH, HIGH);

  // device address is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));            // sends instruction byte
  Wire.write(0x60);             // sends potentiometer value byte
  Wire.endTransmission();     // stop transmitting
  
  analogWriteFreq(60000);
  analogWrite(PIN_CLK, 20);
  delay(500);
  connectWifi();
}

void loop() {
  digitalWrite(PIN_LED, LOW);
  delay(100);
  digitalWrite(PIN_LED, HIGH);
  
  float temp = 0, soil_hum = 0;
  Serial.println("Requesting Temperature...");
  temp = readTemperature();
  
  delay(100);
  soil_hum = readSoilSensor();
  Serial.print("Soil Humidity:");
  Serial.println(soil_hum);
  Serial.print("Temperature:");
  Serial.println(temp);
  delay(500);
  
  sendData(temp, soil_hum);
  ESP.deepSleep(SLEEP_TIME, WAKE_RF_DEFAULT);
}

void connectWifi() {
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

void sendData(float temp, float soil_hum) {  
   WiFiClient client;
  
   if (client.connect("api.thingspeak.com", 80)) { // use ip 184.106.153.149 or api.thingspeak.com
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
