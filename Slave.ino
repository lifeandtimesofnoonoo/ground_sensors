/*
The intended purpose of this code is to allow any slave sensor node to take sensor readings and relay them back to
the master node via a mesh network while spending as much time as possible in a low power state.
*/

#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>

RF24 radio(7, 8);                     //Sets the CE and CS pins
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

#define nodeID 2                      //Unique ID of node. In future should be set dynamically during setup
const int nodeDepth = 30;             //Depth of sensor
uint32_t timer = 0;                   //Used for time keeping

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
  
  mesh.setNodeID(nodeID);             //Sets the nodeID manually
  
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
                                                                            //data rate set as low as possible to increase antenna sensitivity

}

void loop() 
{
  mesh.update();                      //Keeps mesh updated - testing required to see how this should be used when bringing node in and out of sleep

  if(millis() - timer >= 5000)        //Send message every 5 seconds
  {
    timer = millis();
    struct dataPacket toBeSent = {takeTemperature(), nodeID, nodeDepth};

    Serial.println("Sending data...");

    if(!mesh.write(&toBeSent, 'M', sizeof(toBeSent)))   //If write fails, check conectivity
    {
      if(!mesh.checkConnection())                         //Refresh the network address 
      {
        Serial.println("Renewing Address");
        mesh.renewAddress();                              //Re-establishes connection to mesh
      } 
      else 
      {
        Serial.println("Send fail, Network test OK");
      }
    }
  }
}


uint32_t takeTemperature()     //Measures temperature from a sensor - currently simulates behavior
{
  return(random(50));
}
