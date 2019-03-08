/*
    To use: set RF24_SPI_SPEED = 250000 in RF24_config.h
*/
#include <SPI.h>
#include <SD.h>
#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <EEPROM.h>
//#include "DigitalIO.h"

RF24 radio(7, 8);                     //Sets the CE and CS pins
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

struct dataPacket                     //Stucture of data sent over RF
{
  float temperature_t;                //Measured temperature
  uint8_t NodeID_t;                   //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth_t;                //Depth of the sensor on this node
};


const int chipSelect = 10;
File file;
String dataString = "";

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
  }
  else {
    Serial.println("card initialized.");
  }


  mesh.setNodeID(0);                  //Node 0 is the master node

  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
  Serial.println(F("Connected"));

}

void loop() {
  mesh.update();      //Call mesh.update to keep the network updated

  mesh.DHCP();        //In addition, keep the 'DHCP service' running on the master node so addresses will
  //be assigned to the sensor nodes
  if (network.available())     // Check for incoming data from the sensors
  {
    Serial.println("HELLO");
    RF24NetworkHeader header;
    network.peek(header);

    dataPacket received;       //Data received from slave node
    Serial.println("GOT MAIL");
    switch (header.type)       //Framework for changing behavior based on type of data
    {
      case 'M':
        network.read(header, &received, sizeof(received));
        Serial.println("WOW NETWORK");
        // make a string for assembling the data to log:
        // read temperature sensors and append to the string:
        dataString += String(received.NodeID_t);
        dataString += ", ";
        dataString += String(received.temperature_t);
        dataString += ", ";
        dataString += String(received.nodeDepth_t);
        //Serial.println(dataString);
        File dataFile = SD.open("datalog.txt", FILE_WRITE);

        // if the file is available, write to it:
        if (dataFile) {
          dataFile.println(dataString);
          dataFile.close();
          // print to the serial port too:
          Serial.println(dataString);
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening datalog.txt");
        }
        dataString = "0";
        break;

      default:
        network.read(header, 0, 0);
        Serial.println(header.type);
        Serial.println("Unknown header type");
        break;
    }
  }
}
