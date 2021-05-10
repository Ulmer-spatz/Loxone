
#include "WiFi.h"
#include "WiFiUdp.h"
#include "Arduino.h"

/*********  EDIT here ***********************/

String Version = "Garage 0.5";

// WLAN
const char * ssid = "WLAN Name";
const char * pwd = "WLAN Kennwort";

// IP address Loxone Server and UDP Port.
const char * udpAddress = "172.27.1.9";
const int udpPort = 702;


// Ports Eingang
#define Eingang_Auf 4
#define Eingang_Zu 5

// Ports Ausgang
#define Ausgang_Auf 18
#define Ausgang_Zu 19

/***************************************************/

// buffers for receiving and sending data
char packetBuffer[50];
String UDP_data;
char packetBuffer_empfang[50];

//create UDP instance
WiFiUDP udp;


void setup()                    //hardware initialization.
{

  Serial.begin(9600);           //enable serial port.
  Serial.println("Systemstart");

  //Connect to the WiFi network
  WiFi.begin(ssid, pwd);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  //This initializes udp and transfer buffer
  udp.begin(udpPort);


  pinMode(Eingang_Auf, INPUT_PULLUP);
  pinMode(Eingang_Zu, INPUT_PULLUP);
  pinMode(Ausgang_Auf, OUTPUT);
  pinMode(Ausgang_Zu, OUTPUT);

digitalWrite(Ausgang_Auf, HIGH);
digitalWrite(Ausgang_Zu, HIGH);

}


void loop() {                                                                     //the main loop.
  if (WiFi.status() != 3) {
    WiFi.begin(ssid, pwd);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      delay(500);
    }

  }


  /***************   Endlagenerkennung ****************/
  if (digitalRead(Eingang_Auf) == LOW) {
    
    sendUDP("Auf:0");


  } else {
    
    sendUDP("Auf:1");

  }


  if (digitalRead(Eingang_Zu) == LOW) {
    
    sendUDP("Zu:0");


  } else {
    
    sendUDP("Zu:1");
  }
/*********************************************/
    delay (1000);
  
/************ UDP Befehle verarbeiten ****************/

    int packetSize = udp.parsePacket();
    //Serial.print("packetBuffer_empfang");
    if (packetSize)
    {
      udp.read(packetBuffer_empfang, 50);
      //Serial.print(packetBuffer_empfang);

    }

    if (!strcmp(packetBuffer_empfang, "Auf"))
    {

      Serial.print("Auf Befehl erhalten");
      digitalWrite(Ausgang_Auf, LOW);
      delay (1000);
      digitalWrite(Ausgang_Auf, HIGH);
      
    }

    if (!strcmp(packetBuffer_empfang, "Zu"))
    {

      Serial.print("Zu Befehl erhalten");
      digitalWrite(Ausgang_Zu, LOW);
      delay (1000);
      digitalWrite(Ausgang_Zu, HIGH);
    }
    packetBuffer_empfang[0] = 0;
    packetBuffer_empfang[1] = 0;
    packetBuffer_empfang[2] = 0;
    packetBuffer_empfang[3] = 0;
  }

  void sendUDP(String text)
  {
    udp.beginPacket(udpAddress, udpPort);
    udp.print(text);
    udp.endPacket();
  }
