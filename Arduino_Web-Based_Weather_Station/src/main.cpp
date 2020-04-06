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
  //myRTC.setDS1302Time(00, 53, 11, 01, 06, 04, 2020); // Time is already preloaded to the module
  dht.begin();
}

void loop() {
  UpdateTime();
  MeasureCond();
  ConditionCalculations();
  SendWebpage();
}

void UpdateTime() { // Update time every second
  static unsigned long check {0};
  if (check <= millis() - 1000) {
    myRTC.updateTime();
    check = millis();
  }
}

void MeasureCond() { // Measure temperature and humidity every 2 seconds
  static unsigned long check {0};
  if (check <= millis() - 2000) {
    Temperature = dht.readTemperature();
    Humidity = dht.readHumidity();
    check = millis();
  }
}

void ConditionCalculations() { // 
  static unsigned long check {0};
  if (check <= millis() - 2000) {
    static float tempTemp {0}; // Buffer to add temperature every 5 minutes
    static float tempHum {0}; // Buffer to humidity every 5 minutes
    static uint8_t minutesIndex = myRTC.minutes; // Stores last minute
    static uint8_t hoursIndex = myRTC.hours; // Stores last hour
    static uint8_t dayIndex = myRTC.dayofweek; // Stores last day
    static uint8_t CalcCount{0}; // Measure number of calculations per hour
    static uint8_t index{0}; // Hourly matrix index
    static uint8_t HoursCount{0}; // Measure number of calculations per day
    if (minutesIndex == myRTC.minutes - 5) { // If 5 minutes have passed since last calculation
      tempTemp += Temperature; // Add current temperature to the temporary buffer
      tempHum += Humidity; // Add current humidity to the temporary buffer
      CalcCount++; // Add one to the calculation counter
      minutesIndex = myRTC.minutes; // Reset the minutes counter
    }
    if (myRTC.hours != hoursIndex) { // If one hour passed
      TempHist[index] = tempTemp/CalcCount; // Calculate average temperature
      HumHist[index] = tempHum/CalcCount; // Calculate average humidity
      Hours[index] = myRTC.hours; // Store the hour when the calculations were made
      tempTemp = 0; // Reset temperature buffer
      tempHum = 0; // Reset humidity buffer
      CalcCount = 0; // Reset measurements counter
      HoursCount++; // Add one to the hours counter
      minutesIndex = myRTC.minutes; // Reset the minutes counter
      hoursIndex = myRTC.hours; // Reset the hours counter
      index++; // Increase array index 
      if (dayIndex != myRTC.dayofweek) { // Check if day changed as well
        float indexTemp {0};
        float indexHum {0};
        for (uint8_t i = (index - HoursCount); i <= index - 1; i++) {
          indexTemp += TempHist[i];
          indexHum += HumHist[i];
        }
        TempDHist[dayIndex-1] = indexTemp/HoursCount; // Calculate average daily temperature
        HumDHist[dayIndex-1] = indexHum/HoursCount; // Calculate average daily humidity
        HoursCount = 0; // Reset Hours counter
      }
      if (index == 24) { // if index passed array limits
        for (uint8_t i = 0; i <= 22; i++) { // shift data left
          TempHist[i] = TempHist[i+1];
          HumHist[i] = HumHist[i+1];
          Hours[i] = Hours[i+1];
          index = 23;
        }
      }
    }
    check = millis();
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
          client.println(F("<html><head><title>Arduino WebBased Weather Station</title>\
          <style>@charset 'UTF-8';@import url(https://fonts.googleapis.com/css?family=Open+Sans:300,400,700);\
          body {background-color: black; font-family: 'Open Sans', sans-serif; line-height: 1em; text-align: center;\
          Color: #cccccc;}table {font-family: arial, sans-serif;border-collapse: collapse;width: 100%;\
          margin-left:auto;margin-right:auto;}th {border: 1px solid #336699;\
          color:#0040ff;text-align: center;padding: 8px;}td {border: 1px solid #336699;\
          color:#0080ff;text-align: center;padding: 8px;}td.td2 {border: 1px solid #336699;color:#0080ff;text-align:\
          left;padding: 8px;}</style><h1>Arduino Web-Based Weather Station</h1><hr>\
          <h3>WELCOME!</h3><p>Project is hosted on github. Please visit my\ 
          <a href='https://github.com/zissis-pap'>page</a> for more!</p><hr></head><body><br><table style='width:60%'>\
          <tr><th colspan='4'>DATE, TIME AND ROOM CONDITIONS</th></tr><tr><td>Date &#128197</td><td>Time &#128336</td>\
          <td>Temperature &#x1F321</td><td>Humidity &#x2614</td></tr><tr><td><font color='00ff00'>"));
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
          client.println(F("</font></td></tr></table><hr><h3>HISTORY</h3>\
          <table style='width:100%'><tr>\
          <th colspan='25'>AVERAGE CONDITIONS FOR THE LAST 24 HOURS</th></tr><tr>\
          <td class='td2'>HOURS:</td>"));
          for (uint8_t i = 0; i <= 23; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(Hours[i]);
            client.println(F(":00</font></td>"));
          }
          client.println(F("</tr><tr><td class='td2'>TEMPERATURE (&#8451):</td>"));
          for (uint8_t i = 0; i <= 23; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(TempHist[i]);
            client.println(F("</font></td>"));
          }
          client.println(F("</tr><tr><td class='td2'>HUMIDITY (%):</td>"));
          for (uint8_t i = 0; i <= 23; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(HumHist[i]);
            client.println(F("</font></td>"));
          }
          client.print(F("</tr></table><br><table style='width:60%'><tr>\
          <th colspan='8'>AVERAGE CONDITIONS FOR THE LAST 7 DAYS</th></tr><tr>\
          <td class='td2'>DAY:</td>"));
          for (uint8_t i = 0; i <= 6; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(Days[i]);
            client.println(F("</font></td>"));
          }
          client.println(F("</tr><tr><td class='td2'>TEMPERATURE:</td>"));
          for (uint8_t i = 0; i <= 6; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(TempDHist[i]);
            client.println(F("&#8451</font></td>"));
          }
          client.println(F("</tr><tr><td class='td2'>HUMIDITY:</td>"));
          for (uint8_t i = 0; i <= 6; i++) {
            client.println(F("<td><font color='#3d0099'>"));
            client.print(HumDHist[i]);
            client.println(F("%</font></td>"));
          }
          client.print(F("</tr></table><hr><h3>ABOUT THE PROJECT</h3><p align='right'>\
          Author: Zissis Papadopoulos @2020</p></body></html>"));
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