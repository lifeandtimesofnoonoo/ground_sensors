/*
The intended purpose of this code is to allow the master node to receive data from any node in the network and record it
*/

#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>

RF24 radio(7, 8);                     //Sets the CE and CS pins
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

struct dataPacket                     //Stucture of data sent over RF
{
  uint16_t temperature;                  //Measured temperature
  //time, in some format
  uint8_t uniqueNodeID;                   //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth;                      //Depth of the sensor on this node
};

void setup()
{
  Serial.begin(115200);               //For debugging - final product unlikely to use this
  
  mesh.setNodeID(0);                  //Node 0 is the master node
  
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
                                                                            //data rate set as low as possible to increase antenna sensitivity

}

void loop()
{
  
  mesh.update();      //Call mesh.update to keep the network updated
  
  
  mesh.DHCP();        //In addition, keep the 'DHCP service' running on the master node so addresses will
                      //be assigned to the sensor nodes


  
  if(network.available())      // Check for incoming data from the sensors
  {
    RF24NetworkHeader header;
    network.peek(header);

    dataPacket received;       //Data received from slave node

    switch(header.type)        //Framework for changing behavior based on type of data
    {
      case 'M':
      network.read(header, &received, sizeof(received));
      Serial.print("The temperature is: ");
      Serial.println(received.temperature);
      Serial.print("At node ");
      Serial.print(received.uniqueNodeID);
      Serial.print(" which has a depth of ");
      Serial.println(received.nodeDepth);
      break;

      default:
      network.read(header,0,0);
      Serial.println(header.type);
      Serial.println("Unknown header type");
      break;
    }

  }

}
