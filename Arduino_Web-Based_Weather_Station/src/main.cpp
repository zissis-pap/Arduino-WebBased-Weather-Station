//==================== ARDUINO WebBased Weather Station =============================
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h> // Use ARDUINO Ethernet Shield library
#include "DHT.h" // Use DHT11 sensor library
#include <virtuabotixRTC.h> // Use DS1302 library

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
const byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 22);
// Set HTTP port. (port 80 is default for HTTP):
EthernetServer server(80);

//Define RTC pins: (CLK, DAT, RST)
virtuabotixRTC myRTC(6, 7, 8); 

// Define DHT Sensor
#define DHTPIN 3     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// Variables
String Days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"}; // Store Day names
String Months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
uint8_t Hours[24] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float TempHist[24] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float HumHist[24] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float TempDHist[7] {0,0,0,0,0,0,0};
float HumDHist[7] {0,0,0,0,0,0,0};
float Temperature, Humidity;

// Server Functions
void SendWebpage();

// Time Functions
void UpdateTime();

// Conditions Functions
void MeasureCond();
void ConditionCalculations();

// Settings Functions
void SetIP();

void setup() {
  Serial.begin(9600); // Enable Serial communication
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Ethernet.begin(mac, ip);
  server.begin();
  // Print the IP on which to connect the client.
  Serial.print(F("Server is at "));
  Serial.println(Ethernet.localIP());
  Serial.println();
  // Preset the date and time according to:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  //myRTC.setDS1302Time(00, 20, 19, 06, 04, 04, 2020); // Time is already preloaded to the module
  dht.begin();
}

void loop() {
  UpdateTime();
  MeasureCond();
  ConditionCalculations();
  SendWebpage();
}

void UpdateTime() {
  static unsigned long check {0};
  if (check <= millis() - 1000) {
    myRTC.updateTime();
    check = millis();
  }
}

void MeasureCond() {
  static unsigned long check {0};
  if (check <= millis() - 2000) {
    Temperature = dht.readTemperature();
    Humidity = dht.readHumidity();
    check = millis();
  }

}

void ConditionCalculations() {
  static unsigned long check {0};
  static uint8_t minutesIndex = myRTC.minutes;
  static uint8_t hoursIndex = myRTC.hours;
  static uint8_t dayIndex = myRTC.dayofweek;
  static unsigned int tempTemp {0};
  static unsigned int tempHum {0};
  static uint8_t CalcCount{0};
  static uint8_t HoursCount{0};
  static uint8_t index{0};
  if (check <= millis() - 2000) {
    if (minutesIndex == myRTC.minutes - 5) {
      tempTemp += Temperature;
      tempHum += Humidity;
      CalcCount++;
      minutesIndex = myRTC.minutes;
    }
    if (myRTC.hours != hoursIndex) {
      TempHist[index] = tempTemp/CalcCount;
      HumHist[index] = tempHum/CalcCount;
      Hours[index] = hoursIndex;
      tempHum = 0;
      tempTemp = 0;
      HoursCount++;
      hoursIndex = myRTC.hours;
      minutesIndex = myRTC.minutes;
      CalcCount = 0;
      if (dayIndex != myRTC.dayofweek) {
        uint16_t indexTemp;
        uint16_t indexHum;
        for (uint8_t i = index; i = index - HoursCount; i--) {
          indexTemp += TempHist[i];
          indexHum += HumHist[i];
        }
        TempDHist[dayIndex-1] = indexTemp/HoursCount;
        HumDHist[dayIndex-1] = indexHum/HoursCount;
        HoursCount = 0;
      }
      index++;
      if (index == 24) {
        for (uint8_t i = 0; i <= 22; i++) {
          TempHist[i] = TempHist[i+1];
          HumHist[i] = HumHist[i+1];
          Hours[i] = Hours[i+1];
          index = 23;
        }
      }
      if (myRTC.hours == 0) {

      }
    }
  }
}

void SendWebpage() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println(F("new client"));
    // An http request ends with a blank line.
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // The connection will be closed after completion of the response
          client.println(F("Refresh: 30"));  // Refresh the page automatically every 30 sec
          client.println();
          
          client.println(F("<!DOCTYPE HTML>"));
          client.println(F("<html><head><title>Arduino WebBased Weather Station</title></html>\
          <style>@charset 'UTF-8';@import url(https://fonts.googleapis.com/css?family=Open+Sans:300,400,700);\
          body {background-color: black; font-family: 'Open Sans', sans-serif; line-height: 1em; Color: #cccccc;}\
          table {font-family: arial, sans-serif;border-collapse: collapse;width: 100%;}th {border: 1px solid #336699;\
          background-color: black;color:#0040ff;text-align: center;padding: 8px;}td {border: 1px solid #336699;\
          background-color: black;color:#0080ff;text-align: center;padding: 8px;}tr {background-color: black;}\
          </style><h1 align='center'>Arduino Web-Based Weather Station</h1><hr>\
          <h3 align='center'>WELCOME!</h3></head><body><br><table style='width:60%' align='center'>\
          <tr><th colspan='4'>DATE, TIME AND ROOM CONDITIONS</th></tr><tr><td>&#128197; Date</td><td>&#128336; Time</td>\
          <td>&#x1F321; Temperature</td><td>&#x2614; Humidity</td></tr><tr><td><font color='00ff00'>"));
          client.print(Days[myRTC.dayofweek-1]);
          client.println(F(", the "));
          client.print(myRTC.dayofmonth);
          if (myRTC.dayofmonth == 1) client.println(F("st"));
          else if (myRTC.dayofmonth == 2) client.println(F("nd"));
          else if (myRTC.dayofmonth == 3) client.println(F("rd"));
          else client.println(F("th"));
          client.println(F(" of "));
          client.println(Months[myRTC.month - 1]);
          client.println(F(" "));
          client.println(myRTC.year);
          client.println(F("</font></td><td><font color='00ff00'>"));
          client.println(myRTC.hours);
          client.println(":");
          if (myRTC.minutes <= 9) client.print(0);
          client.println(myRTC.minutes);
          client.println(F("</font></td><td><font color='00ff00'>"));
          client.print(Temperature);
          client.println(F("&#8451"));
          client.println(F("</font></td><td><font color='00ff00'>"));
          client.print(Humidity);
          client.println(F("%"));
          client.println(F("</font></td></tr></table><hr><h3 align='center'>HISTORY</h3>\
          <table style='width:100%' align='center'><tr>\
          <th colspan='25'>AVERAGE CONDITIONS FOR THE LAST 24 HOURS</th></tr><tr><font color='00ff00'>\
          <td>&#x1F559 HOURS:</td>"));
          for (uint8_t i = 0; i <= 23; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(Hours[i]);
            client.println(F(":00</font></td>"));
          }
          client.println(F("</font></tr><tr><font color='00ff00'><td>&#x1F321; TEMPERATURE: (&#8451)</td>"));
          for (uint8_t i = 0; i <= 23; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(TempHist[i]);
            client.println(F("</font></td>"));
          }
          client.println(F("</font></tr><tr><font color='00ff00'><td>&#x2614; HUMIDITY: (%)</td>"));
          for (uint8_t i = 0; i <= 23; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(HumHist[i]);
            client.println(F("</font></td>"));
          }
          client.print(F("</font></tr></table><br><table style='width:60%' align='center'><tr>\
          <th colspan='8'>AVERAGE CONDITIONS FOR THE LAST 7 DAYS</th></tr><tr><font color='00ff00'>\
          <td>&#x1F4C5; DAY:</td>"));
          for (uint8_t i = 0; i <= 6; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(Days[i]);
            client.println(F("</font></td>"));
          }
          client.println(F("</font></tr><tr><font color='00ff00'><td>&#x1F321;TEMPERATURE:</td>"));
          for (uint8_t i = 0; i <= 6; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(TempDHist[i]);
            client.println(F("&#8451</font></td>"));
          }
          client.println(F("</font></tr><tr><font color='00ff00'><td>&#x2614; HUMIDITY:</td>"));
          for (uint8_t i = 0; i <= 6; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(HumDHist[i]);
            client.println(F("%</font></td>"));
          }
          client.print(F("</font></tr></table><hr><h3 align='center'>ABOUT THE PROJECT</h3></body></html>"));
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true; // you're starting a new line
        }
        else if (c != '\r') { // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    delay(1); // Give the web browser time to receive the data.
    client.stop(); // Close the connection:
  }
}

void SetIP() {
  boolean Set = false;
  boolean Cont = false;
  while (!Cont) {
    uint8_t buffer[12];
    Serial.println(F("Please enter the desired IP address according to the following format: XXX.XXX.XXX.XXX"));
    while(!Set) {
      uint8_t i{0};
      // put your main code here, to run repeatedly:
      while (Serial.available() > 0) {
        uint8_t inChar = Serial.read();
        if (isDigit(inChar)) {
          // convert the incoming byte to a char and add it to the string:
          buffer[i] = inChar - 48;
          i++;
          if (i==11) Set = true;
        }
        delay(5);
      }
    }
    uint8_t ip1{0}, ip2{0}, ip3{0}, ip4{0}, x{100};
    for (uint8_t j = 0; j <=2; j++) {
      ip1+=buffer[j]*x;
      ip2+=buffer[j+3]*x;
      ip3+=buffer[j+6]*x;
      ip4+=buffer[j+9]*x;
      x=x/10;
    }
    IPAddress ip(ip1, ip2, ip3, ip4);
    Serial.print(F("Your selected ip is: "));
    Serial.println(ip);
    Serial.print(F("Is that OK? Please press y or n:"));
    Set = false;
    while(!Set) {
      while (Serial.available())  {
        char c = Serial.read();  //gets one byte from serial buffer
        if (c == 'y') {
          Set = true;
          Cont = true;
          Ethernet.begin(mac, ip);
          server.begin();
        }
        if (c == 'n') {
          Set = true;
        }
      }
    }
    Set = false;
  }
}