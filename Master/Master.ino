/*
    To use: set RF24_SPI_SPEED = 250000 in RF24_config.h
*/
#include <SPI.h>
#include <SD.h>
#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <EEPROM.h>

RF24 radio(7, 8);                     //Sets the CE and CS pins for the radio
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

struct dataPacket                     //Stucture of data sent over RF
{
  float temperature_t;                //Measured temperature
  uint8_t NodeID_t;                   //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth_t;                //Depth of the sensor on this node
};

const int chipSelect = 10;            //Sets the CS pin for the sd card reader
File file;
String dataString = "";

void setup()
{
  Serial.begin(115200);
  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect))          //See if the card is present and can be initialized:
  {
    Serial.println("Card failed, or not present");
  }
  else {
    Serial.println("card initialized.");
  }

  mesh.setNodeID(0);                  //Node 0 is the master node
  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
                                                                            //data rate set as low as possible to increase antenna sensitivity
  Serial.println(F("Began the mesh"));
}

void loop()
{
  mesh.update();      //Call mesh.update to keep the network updated
  mesh.DHCP();        //In addition, keep the 'DHCP service' running on the master node so addresses will
                      //be assigned to the sensor nodes
  if (network.available())     // Check for incoming data from the sensors
  {
    Serial.println("You've got mail!");
    RF24NetworkHeader header;  //Initialises a header to determine message content
    network.peek(header);      //Check content of next message
    dataPacket received;       //Data received from slave node
    switch (header.type)       //Framework for changing behavior based on type of data
    {
      case 'M':
        network.read(header, &received, sizeof(received));
                                                        
        dataString = "";                                 //Make a string for assembling the data to log              
        dataString += String(received.NodeID_t);
        dataString += ", ";
        dataString += String(received.temperature_t);
        dataString += ", ";
        dataString += String(received.nodeDepth_t);
        File dataFile = SD.open("datalog.txt", FILE_WRITE);   //Open file, prepare to write to it
     
        if (dataFile)                                   //If the file is available, write to it
        {
          dataFile.println(dataString);
          dataFile.close();                             //Is it required to close the file after use if we're only ever writing to one file?       
          Serial.println(dataString);                   //Print to the serial port too
        }
        else 
        {
          Serial.println("error opening datalog.txt");
        }                              
        break;

      default:
        network.read(header, 0, 0);
        Serial.println(header.type);
        Serial.println("Unknown header type");
        break;
    }
  }
}
