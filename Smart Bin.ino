#include "WiFi.h"
#include <HTTPClient.h>
#include "time.h"
#include <HardwareSerial.h>
#include <TinyGPS++.h>


#include <ESP32Servo.h>

#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

TinyGPSPlus gps;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;  //19800;
const int daylightOffset_sec = 3600;
float volts;
float human_distance;
float vol2;
String distance, distance2, cond, Gps_LT;

const int IRpin = 34;
const int IRpin2 = 35;

const int motorcheck = 14;

Servo servoMain; // initiallize the first servo
Servo servoTwo; // initiallize the second Servo


int trigpin = 12; // setting the trig pin to pin 12
int echopin = 13; // setting the echo pin to pin 13

int sudistance; // creating the distance variable for the Ultrasonic sensor
float duration;
float cm;
int angle;
bool is_opened = false;


const char* ssid = "Wifi Nmae";
const char* password = "Wifi Password";

String GOOGLE_SCRIPT_ID = "AKfycbzPTJ-E-RcVCQbwqdr0tpOUJdlNX8UqfrGCry30usmtYdPjhGEtkh_JIdlKu_EvKPt6";  //"AKfycbx_PxkSe37CGVOeNZaI5ebxprMfyFMDa320O6plZ_RkQjEETTHy8DFhIegPz9anbXwc";


void setup() {

  Serial.begin(115200);

  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(IRpin, INPUT_PULLUP);
 
  servoMain.attach(23);  // servo on digital pin 10
  servoTwo.attach(22);  // servo on digital pin 10




  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  pinMode(IRpin2, INPUT_PULLUP);

  // setting the pins of the proximity sensor for on and off
  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);

  // closing the bin
  servoMain.write(0);
  servoTwo.write(160);
}
void loop() {

  // calling the function to update the distance with the current distance
  get_dist();

  // if (sudistance < 30){
  //   // if the distance is less than 
  //   angle = 0;
  //   while ( angle <= 50) {
      
  //     get_dist();
  //     if (sudistance < 30 &&  angle == 49){
  //     servoMain.write(50);
  //     servoTwo.write(110);
  //     delay(3000);
  //     continue;
  //   }
      
  //     servoMain.write(angle);       // Turn Servo back to center position
  //     servoTwo.write(160 - angle);  // Turn Servo back to center position
  //     ++angle;
  //     delay(25);
      

      
  //   }
  //    Serial.println(sudistance);
  //   is_opened = true;
  //   delay(3000);

  // } else {
  //   //  move 50 , 110
  //   // Serial.println(is_opened);
  //   if (is_opened == true ){
  //     int i = 50; 
  //     while (i >=0) {
  //       servoMain.write(i);
  //       servoTwo.write(160-i);
  //       delay(45);
  //       --i;
  //     }
  //     is_opened = false;
  //   }
  //   else{

  //     servoMain.write(0);
  //     servoTwo.write(160);
  //   }
    
  //   }
     sensor();
  send_update();
  Update_Cond();
  //motor();
  Gpsdata();
  //Display_GPS();

  vol2 = analogRead(IRpin2) * (0.0048828125);
  human_distance = 65 * pow(vol2, -1.10);
  distance2 = (distance2, (human_distance > 0 && human_distance <= 5) ? "HALF_FILLED" : "GOOD");
  //Serial.println(distance2);
  delay(10);



 
}

void sensor() {

  volts = analogRead(IRpin) * 0.0048828125;
  float save = 65 * pow(volts, -1.10);
  distance = (distance, (save > 0 && save <= 5) ? "BIN_FILLED" : "OKAY");
  // Serial.println(distance);
  delay(10);
  
}
void Update_Cond() {

  if (((distance == "BIN_FILLED") && (distance2 == "HALF_FILLED")) || (distance == "BIN_FILLED")) {
    cond = "BIN_FULLY_FILLED";
  } else if ((distance2 == "HALF_FILLED") && (distance != "BIN_FILLED")) {
    cond = "BIN_HALF_FILLED";
  } else {
    cond = "Empty";  // "Check Sensor"
  }
}
void send_update() {
  if (WiFi.status() == WL_CONNECTED) {
    static bool flag = false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    String asString(timeStringBuff);
    asString.replace(" ", "-");

    String param;
    param = "latitude=" + cond;
    param += "&longitude=" + String(asString);
    param += "&lat=" + Gps_LT;
    param += "&long=" + String("AC:67:B2:11:58:8C");


    Serial.println(param);
    write_to_google_sheet(param);
  }
}


void write_to_google_sheet(String params) {

  HTTPClient http;
  String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;
  if ((cond == "BIN_FULLY_FILLED") || (cond == "BIN_HALF_FILLED")) {
    for (int i = 0; i < 2; ++i) {
      http.begin(url.c_str());
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      int httpCode = http.GET();
      Serial.print("HTTP Status Code: ");
      Serial.println(httpCode);

      String payload;
      if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload: " + payload);
      }

      http.end();
    }
  }
}

void Gpsdata() {
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (neogps.available()) {
      if (gps.encode(neogps.read())) {
        newData = true;
      }
    }
  }

  //If newData is true
  if (newData == true) {
    newData = false;
    Serial.println(gps.satellites.value());
    Display_GPS();
  }
}
void Display_GPS() {
  if (gps.location.isValid() == 1) {

    Gps_LT = (String(gps.location.lng(), 6) + "," + (String(gps.location.lat(), 6)));
    Serial.println(Gps_LT);

  } else {

    Serial.println("No Data");
  }
}

void get_dist() {
  digitalWrite(trigpin, LOW);


  delay(2);

  digitalWrite(trigpin, HIGH);

  delayMicroseconds(10);

  digitalWrite(trigpin, LOW);

  duration = pulseIn(echopin, HIGH);

  sudistance = (duration / 58.82);

}
