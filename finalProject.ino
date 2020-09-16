#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "Arduino.h"
#include <SoftwareSerial.h>    //Allows us to use two GPIO pins for a second UART

//all pins connected to 1kohm resistors and LED's
#define redPin 2
#define greenPin 4
#define bluePin 6


SoftwareSerial espSerial(10,11);  //Create software UART to talk to the ESP8266

String IO_USERNAME = "RDMartin";   //paste your Adafruit IO username here
String IO_KEY = "aio_SufS30hFo1frJUX5VQN21ypCC1Oq"; //paste your Adafruit IO key here
//String WIFI_SSID = "UD Devices";  //Only need to change if using other network, eduroam won't work with ESP8266
//String WIFI_PASS = "";    //Blank for open network
String WIFI_SSID =  "PYWVD";    //Change to your Wi-Fi network SSID (Service Set Identifier)
String WIFI_PASS =  "TFDPZCDPH3DN2GYG"; //Change to your Wi-Fi password
float num = 0.0;      //if num == 1: red is dominant, num == 2: green is dominant, num == 3: blue is dominant
int counter = 0;  //Counter to send the value to esp

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
uint16_t red, green, blue, clr;
void setup() {

  //esp setup
  Serial.begin(9600);
  espSerial.begin(9600);    // set up software UART to ESP8266 @ 9600 baud rate
  Serial.println("setting up");
  String resp = espData("get_macaddr",2000,false);  //get MAC address of 8266
  resp = espData("wifi_ssid="+WIFI_SSID,2000,false);  //send Wi-Fi SSID to connect to
  resp = espData("wifi_pass="+WIFI_PASS,2000,false);  //send password for Wi-Fi network
  resp = espData("io_user="+IO_USERNAME,2000,false);  //send Adafruit IO info
  resp = espData("io_key="+IO_KEY,2000,false);
  resp = espData("setup_io",15000,false);     //setup the IoT connection
  if(resp.indexOf("connected") < 0) {
    Serial.println("\nAdafruit IO Connection Failed");
    while(1);
  }
  resp = espData("setup_feed=1,CPEG-ELEG298",2000,false); //start the data feed
  Serial.println("------ Setup Complete ----------");

  //color sensor setup
  if (tcs.begin()) {
    Serial.println("Starting!");
  } else {
    Serial.println("Sensor Connection Failed");
    while (1); // Doesn't commence
  }

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}


//The loop gets the RGB values and finds which color is dominant.
//Defaults to red being dominant if too dark/light
void loop() {
  tcs.getRawData(&red, &green, &blue, &clr);  //Detects color from color sensor
  if(red >= green){
    if(red >= blue){
      Serial.println("red is dominant");
      analogWrite(redPin,200);
      analogWrite(greenPin,0);
      analogWrite(bluePin,0);
      num = 1.0;
    }
    else{
      Serial.println("blue is dominant");
      analogWrite(bluePin,200);
      analogWrite(greenPin,0);
      analogWrite(redPin,0);
      num = 3.0;
    }
  }
  else if(green >= blue){
    Serial.println("green is dominant");
      analogWrite(greenPin,200);
      analogWrite(bluePin,0);
      analogWrite(redPin,0);
      num = 2.0;
  }
  else{
    Serial.println("blue is dominant");
    analogWrite(bluePin,200);
    analogWrite(greenPin,0);
    analogWrite(redPin,0);
    num = 3.0;
  }
  delay(100);//Returns dominant color 10 times per second
  counter=counter+1;
  if(counter >= 50){  //Sends data to adafruit.io
    counter=0;
    Serial.print("Num is: ");
    Serial.println(num);
    String resp = espData("send_data=1,"+String(num),2000,false); //send feed to cloud
  }
}

String espData(String command, const int timeout, boolean debug) {
  String response = "";
  espSerial.println(command); //send data to ESP8266 using serial UART
  long int time = millis();
  while ( (time + timeout) > millis()) {  //wait the timeout period sent with the command
    while (espSerial.available()) { //look for response from ESP8266
      char c = espSerial.read();
      response += c;
      Serial.print(c);  //print response on serial monitor
    }
  }
  if (debug) {
    Serial.println("Resp: "+response);
  }
  response.trim();
  return response;
}
