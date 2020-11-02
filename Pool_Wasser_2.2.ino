
#include "WiFi.h"
#include "WiFiUdp.h"
#include "Arduino.h"
#include "Wire.h"                
#include "heltec.h"              

/*********  EDIT here ***********************/

String Version = "Pool_Wasser_2.3";

// WLAN
const char * ssid = "Meine WLAN SSID";
const char * pwd = "PSK";

// IP address to send UDP data to.
const char * udpAddress = "172.27.1.9";
const int udpPort = 701;

// ml Chlor pro Impuls
String Chlorml = "3";

// ml PH- pro Impuls
String PHml = "2";

/*****************PRO Edit **************************/
#define address_RTD 102            //102 default I2C ID number for EZO RTD Circuit.
#define address_PH 99              //99 default I2C ID number for EZO RTD Circuit.
#define address_ORP 98             //98 default I2C ID number for EZO RTD Circuit.
#define address_CHLOR 103          //103 Chlor Pumpe
#define address_PHM 104            //104 PH- Pumpe

// Ports Eingang
#define Eingang_Flow 12
//const int BUTTON_Flow = 12;

// Ports für I2C Kommunikation
#define SDA 21
#define SCL 22

/***************************************************/

// int button_flow_ist;

// buffers for receiving and sending data
char packetBuffer[50];
String UDP_data;

char packetBuffer_empfang[50];


//create UDP instance
WiFiUDP udp;

byte code = 0;                   //used to hold the I2C response code.
char I2C_data[20];               //we make a 20 byte character array to hold incoming data from the RTD circuit.
byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the RTD Circuit.
byte i = 0;                      //counter used for I2C_data array.
int time_ = 800;                 //used to change the delay needed depending on the command sent to the EZO Class RTD Circuit.
// float RTD_float;

uint8_t temp_uint8 ;


void setup()                    //hardware initialization.
{
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  delay(500);

  Serial.begin(9600);           //enable serial port.
  Serial.println("Systemstart");

  //Display Starten

  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, Version);
  Heltec.display -> drawString(0, 10, "Connecting to WLAN");
  Heltec.display -> display();


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

  //Ausgabe Display
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, Version);
  Heltec.display -> drawString(0, 10, "WLAN Online:");
  Heltec.display -> drawString(60, 10, WiFi.localIP().toString());
  Heltec.display -> display();

  //This initializes udp and transfer buffer
  udp.begin(udpPort);

  Wire1.begin(SDA, SCL);                 //enable I2C port.

  pinMode(Eingang_Flow, INPUT_PULLUP);
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
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 0, Version);
  Heltec.display -> drawString(0, 10, "Online:");
  Heltec.display -> drawString(60, 10, WiFi.localIP().toString());
  int z = 0;

  while (z < 60) {
    checkUDP();
    messen (address_RTD);
    
 /***************   FLOW Schalter Anfang ****************/  
    if (digitalRead(Eingang_Flow) == LOW) {
      // AN
      sendUDP("Flow:1");
      Serial.print("Flow:1");
    } else {
      //Aus
      sendUDP("Flow:0");
      Serial.println ("Flow:0");
    }

 /*************** FLOW Schalter Ende ********************/   
   
    z = z + 1;
    Serial.println (z);

    delay (2000) ;
  }
  delay (1500);
  
  Serial.print("Temp Comp:");
  Serial.println(temp_uint8);
  
  messen_ph (temp_uint8);
  delay(800);
  messen (address_ORP);
  delay(800);
  Heltec.display -> display();

}

void checkUDP()
{
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    udp.read(packetBuffer_empfang, 50);
    Serial.print("ReceiveUDP: ");
    Serial.println(packetBuffer_empfang);
  }

  if (!strcmp(packetBuffer_empfang, "c:"))
  {
    Serial.println("Chlor Läuft");
   pumpen(1, Chlorml);
  }
  if (!strcmp(packetBuffer_empfang, "p-"))
  {
    Serial.println("PH- Läuft");
   pumpen(2, PHml); // Fix mit Variabler
   // pumpen(2, packetBuffer_empfang[2]); // Empfang per UDP, nur 1-9 möglich, !!! Noch Fehler in der Umwandlung String to Char !!!
    
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

void pumpen (int pumpe, String v_ml)
{
  String pumpebefehl1 = v_ml;
  String pumpebefehl2 = "RT,";
  String pumpebefehl3 = pumpebefehl2 + pumpebefehl1  ;
  uint8_t pumpebefehl = atoi (pumpebefehl3.c_str());
  int address;

  switch (pumpe) {
    case 1: // Chloren
      address = address_CHLOR;
      break;
    case 2: // PH Minus
      address = address_PHM;
      break;
  }

  Wire1.beginTransmission(address);                                              //call the circuit by its ID number.
  Wire1.write(pumpebefehl);
  Wire1.endTransmission();                                                       //end the I2C data transmission.

  delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
  Wire1.requestFrom(address, 20, 1);                                           //call the circuit and request 20 bytes (this may be more than we need)
  code = Wire1.read();                                                         //the first byte is the response code, we read this separately.
  Serial.println(pumpebefehl);
  Serial.println(code);
}


void messen (int address)
{


  Wire1.beginTransmission(address);                                              //call the circuit by its ID number.
  Wire1.write("r");
  Wire1.endTransmission();                                                       //end the I2C data transmission.

  delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
  Wire1.requestFrom(address, 20, 1);                                           //call the circuit and request 20 bytes (this may be more than we need)
  code = Wire1.read();                                                         //the first byte is the response code, we read this separately.
  //Serial.println(code);
  while (Wire1.available()) {            //are there bytes to receive.
    in_char = Wire1.read();              //receive a byte.
    I2C_data[i] = in_char;              //load this byte into our array.
    i += 1;                             //incur the counter for the array element.
    if (in_char == 0) {                 //if we see that we have been sent a null command.
      i = 0;                            //reset the counter i to 0.
      break;                            //exit the while loop.
    }
  }



  if (code == 1) {
    switch (address) {                        //switch case based on what the response code is.
      case 98:

        UDP_data = "orp:";
        Heltec.display -> drawString(0, 20, "Redox:");
        Heltec.display -> drawString(35, 20, I2C_data);
        break;


      case 102:
        UDP_data = "temp:";

        Heltec.display -> drawString(0, 40, "Temp:");
        Heltec.display -> drawString(35, 40, I2C_data );
        String test = String (I2C_data);
        String test2 = "rt,";
        String test3 = test2 + test;
        temp_uint8 = atoi (test3.c_str());
        Serial.print("Temp Comp=");
        Serial.println(temp_uint8);
        break;

    }
    sendUDP(UDP_data + I2C_data);
    Serial.print(UDP_data);
    Serial.println(I2C_data);
  }
  else
  {
    Serial.println("Fehler");
  }


}

/*_____________________________________________________________________________________________________*/
void messen_ph (uint8_t tempcomp)
{
  Wire1.beginTransmission(address_PH);                                              //call the circuit by its ID number.
  Wire1.write(tempcomp);
  Wire1.endTransmission();
  delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.

  Wire1.requestFrom(address_PH, 20, 1);                                           //call the circuit and request 20 bytes (this may be more than we need)
  code = Wire1.read();                                                         //the first byte is the response code, we read this separately.

  while (Wire1.available()) {            //are there bytes to receive.
    in_char = Wire1.read();              //receive a byte.
    I2C_data[i] = in_char;              //load this byte into our array.
    i += 1;                             //incur the counter for the array element.
    if (in_char == 0) {                 //if we see that we have been sent a null command.
      i = 0;                            //reset the counter i to 0.
      break;                            //exit the while loop.
    }
  }

  if (code == 1) {
    UDP_data = "ph:";
    Heltec.display -> drawString(0, 30, "PH:");
    Heltec.display -> drawString(35, 30,  I2C_data );
    sendUDP(UDP_data + I2C_data);
    Serial.print(UDP_data);
    Serial.println(I2C_data);
  }
  else
  {
    Serial.print("PH Fehler");
  }
}

/*void schreiben (int address, uint8_t befehl)
  {


  Wire1.beginTransmission(address);                                              //call the circuit by its ID number.
  Wire1.write(befehl);
  Wire1.endTransmission();                                                       //end the I2C data transmission.
  delay(time_);                                                               //wait the correct amount of time for the circuit to complete its instruction.
  Wire1.requestFrom(address, 20, 1);                                           //call the circuit and request 20 bytes (this may be more than we need)
  code = Wire1.read();                                                         //the first byte is the response code, 255=no Data
  Serial.println(code);

  }*/
