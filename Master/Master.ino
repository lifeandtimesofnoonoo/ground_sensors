/*
    The intended purpose of this code is to allow the master node to receive data from any node in the network and record it
    
    ***************************************************************
    ***************************************************************
    TO USE:
    Set RF24_SPI_SPEED = 250000 in RF24_config.h
    ***************************************************************
    ***************************************************************
*/
#include <SPI.h>
#include <SD.h>
#include <RF24.h>
#include "RF24Network.h"
#include "RF24Mesh.h"

#define gs_serial_debug               //Uncomment to enable serial debugging

RF24 radio(7, 8);                     //Sets the CE and CS pins for the radio
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

uint8_t radioChannel = 74;            //Sets radio channel used by network

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
  #ifdef gs_serial_debug
  Serial.begin(115200);
  Serial.print("Initializing SD card...");
  #endif

  if (!SD.begin(chipSelect))          //See if the card is present and can be initialized:
  {
    #ifdef gs_serial_debug
    Serial.println("Card failed, or not present");
    #endif
  }
  else 
  {
    #ifdef gs_serial_debug
    Serial.println("card initialized.");
    #endif
  }

  mesh.setNodeID(0);                  //Node 0 is the master node
  mesh.begin(radioChannel, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
                                                                    //data rate set as low as possible to increase antenna sensitivity
  #ifdef gs_serial_debug                                                                  
  Serial.print(F("Began the mesh on channel: "));
  Serial.println(radioChannel);
  #endif
}

void loop()
{
  mesh.update();      //Call mesh.update to keep the network updated
  mesh.DHCP();        //In addition, keep the 'DHCP service' running on the master node so addresses will
                      //be assigned to the sensor nodes
  if (network.available())     // Check for incoming data from the sensors
  {
    #ifdef gs_serial_debug
    Serial.println("network.available == true");
    #endif
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
          #ifdef gs_serial_debug
          Serial.println(dataString);                   //Print to the serial port too
          #endif
        }
        else
        {
          #ifdef gs_serial_debug
          Serial.println("error opening datalog.txt");
          #endif
        }
        break;

      default:
        network.read(header, 0, 0);
        #ifdef gs_serial_debug
        Serial.println(header.type);
        Serial.println("Unknown header type");
        #endif
        break;
    }
  }
}
