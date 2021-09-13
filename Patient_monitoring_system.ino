#include <ESP8266WiFi.h>
#include "Ubidots.h"
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

#define pulsePin A0


String apiWritekey = "7JZU70LAVYTGDGYU"; // replace with your THINGSPEAK WRITEAPI key here
const char* ssid = "TP-Link"; // your wifi SSID name
const char* password = "ruel12345" ;// wifi pasword

const char* server = "api.thingspeak.com";
float resolution = 3.3 / 1023; // 3.3 is the supply volt  & 1023 is max analog read value
WiFiClient client;

const char* UBIDOTS_TOKEN = "BBFF-ms0r3rPZPLpfB6tmqZvAYQEnFBoJUu";  // Put here your Ubidots TOKEN
const char* WIFI_SSID = "TP-Link";      // Put here your Wi-Fi SSID
const char* WIFI_PASS = "ruel12345";      // Put here your Wi-Fi password
Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);




const int postingInterval = 10 * 1000;

int rate[10];
unsigned long sampleCounter = 0;
unsigned long lastBeatTime = 0;
unsigned long lastTime = 0, N;
int BPM = 0;
int IBI = 0;
int P = 512;
int T = 512;
int thresh = 512;
int amp = 100;
int Signal;
boolean Pulse = false;
boolean firstBeat = true;
boolean secondBeat = true;
boolean QS = false;





void setup() {
  Serial.begin(115200);
  lcd.setCursor(0, 0);
  WiFi.disconnect();
  delay(10);
  WiFi.begin(ssid, password);

  ubidots.wifiConnect(WIFI_SSID, WIFI_PASS);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("NodeMcu connected to wifi...");
  Serial.println(ssid);
  Serial.println();


  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
}

void loop() {



  if (QS == true) {
    //Serial.println("BPM: " + String(BPM));
    QS = false;
  } else if (millis() >= (lastTime + 2)) {
    readPulse();
    lastTime = millis();
  }
}

void readPulse() {

  Signal = analogRead(pulsePin);
  sampleCounter += 2;
  int N = sampleCounter - lastBeatTime;

  detectSetHighLow();

  if (N > 250) {
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3) )
      pulseDetected();
  }

  if (Signal < thresh && Pulse == true) {
    Pulse = false;
    amp = P - T;
    thresh = amp / 2 + T;
    P = thresh;
    T = thresh;
  }

  if (N > 2500) {
    thresh = 512;
    P = 512;
    T = 512;
    lastBeatTime = sampleCounter;
    firstBeat = true;
    secondBeat = true;
  }

}

void detectSetHighLow() {

  if (Signal < thresh && N > (IBI / 5) * 3) {
    if (Signal < T) {
      T = Signal;
    }
  }

  if (Signal > thresh && Signal > P) {
    P = Signal;
  }

}

void pulseDetected() {
  Pulse = true;
  IBI = sampleCounter - lastBeatTime;
  lastBeatTime = sampleCounter;

  if (firstBeat) {
    firstBeat = false;
    return;
  }
  if (secondBeat) {
    secondBeat = false;
    for (int i = 0; i <= 9; i++) {
      rate[i] = IBI;
    }
  }

  word runningTotal = 0;

  for (int i = 0; i <= 8; i++) {
    rate[i] = rate[i + 1];
    runningTotal += rate[i];
  }


  rate[9] = IBI;
  runningTotal += rate[9];
  runningTotal /= 10;
  BPM = 60000 / runningTotal;
  QS = true;






  float temp = (analogRead(A0) * resolution) * 100;
  
  if (client.connect(server, 80))
  {

    String body1 = "field1=";
    body1 += Temp;

    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: ESP8266 (nothans)/1.0");
    client.println("Connection: close");
    client.println("X-THINGSPEAKAPIKEY: " + apiWritekey);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(body1.length()));
    client.println("");
    client.print(body1);

    Serial.print("Temperature: ");
    Serial.print(Temp);


  }

  if (client.connect(server, 80))
  {

    String body2 = "field2=";
    body2 += BPM;

    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: ESP8266 (nothans)/1.0");
    client.println("Connection: close");
    client.println("X-THINGSPEAKAPIKEY: " + apiWritekey);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(body2.length()));
    client.println("");
    client.print(body2);

    Serial.println("     BPM: " + String(BPM));



    Serial.println("\n uploaded to Thingspeak server....");




    lcd.setCursor(0, 0);
    lcd.print("BPM:");
    lcd.print(BPM);
    //delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("Temperature:");
    lcd.print(Temp);
    delay(2000);


  ubidots.add("Pulse", BPM);

  bool bufferSent = false;
  bufferSent = ubidots.send();  // Will send data to a device label that matches the device Id

  if (bufferSent) {
    // Do something if values were sent properly
    Serial.println("Values sent by the device");
  }
    




  }
  client.stop();

  Serial.println("Waiting to upload next reading...");
  Serial.println();
  delay(postingInterval);
  // thingspeak needs minimum 15 sec delay between updates
  // delay(2000);
}
