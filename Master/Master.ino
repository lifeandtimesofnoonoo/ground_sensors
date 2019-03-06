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
  float temperature_t;                //Measured temperature
  uint8_t NodeID_t;                   //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth_t;                //Depth of the sensor on this node
};

byte nodeIDs[256];                     //Keeping track of node ID allocated, when 0 this means unallocated
//Returns 0 for unallocated, the node ID if it is allocated
void setup()
{
  Serial.begin(115200);               //For debugging - final product unlikely to use this

  mesh.setNodeID(0);                  //Node 0 is the master node
  Serial.println(F("I am the master!"));

  nodeIDs[0] = 1;
  nodeIDs[1] = 1;                     //IDs 0 and 1 are reserved - assumed used by the master
  nodeIDs[255] = 255;                 //Since 255 is default, assumed always in use
  for (byte i = 2; i < 255; i++)
  {
    nodeIDs[i] = 0;                   //All node IDs are set 0 prior to use
  }


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
    Serial.println("You've got mail");
    delay(1);
    RF24NetworkHeader header;
    network.peek(header);

    dataPacket received;       //Data received from slave node

    switch (header.type)       //Framework for changing behavior based on type of data
    {
      case 'D':                //Default message type, just data
        network.read(header, &received, sizeof(received));
        Serial.print("The temperature is ");
        Serial.print(received.temperature_t);
        Serial.print(" degrees at node ");
        Serial.print(received.NodeID_t);
        Serial.print(" which has a depth of ");
        Serial.print(received.nodeDepth_t);
        Serial.println(" cm");
        break;

      case 'C':                //New node connecting, requesting node ID

        //Serial.println("I've been asked to allocate");
        byte i = 2;
        byte availableNode;
        do
        {
          availableNode = i;
          i = i + 1;

        } while (nodeIDs[i - 1] == !0);     //Finds the smallest unallocated node ID
        nodeIDs[i - 1] = i - 1;             //Records that the node ID has been allocated

        //byte defaultNode;
        //network.read(header, &defaultNode, sizeof(defaultNode));

        mesh.write(&availableNode, 'C', sizeof(availableNode), 255);          //Reply sent to node 255, this is the default

        //Serial.println("New ID sent");
        //delay(1);

        break;

      default:
        network.read(header, 0, 0);
        Serial.println(header.type);
        Serial.println("Unknown header type");
        break;
    }

  }

}
