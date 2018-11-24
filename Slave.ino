/*
The intended purpose of this code is to allow any slave sensor node to take sensor readings and relay them back to
the master node via a mesh network while spending as much time as possible in a low power state.
*/

#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>

RF24 radio(7, 8);                     //Sets the CE and CS pins
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

const int nodeID = 2;                      //Unique ID of node. In future should be set dynamically during setup
const int nodeDepth = 30;             //Depth of sensor
uint32_t timer = 0;                   //Used for time keeping

struct dataPacket                     //Stucture of data sent over RF
{
  uint16_t temperature;                  //Measured temperature
  uint8_t uniqueNodeID;                   //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth;                      //Depth of the sensor on this node
};

void setup() 
{

  Serial.begin(115200);               //For debugging - final product unlikely to use this

  network.setup_watchdog(9);          //Sets up watchdog timer, 9 denotes a time period of 8 seconds, the longest possible (I'm pretty certain this means the greatest power saving)
  ADCSRA &= ~(1 << ADEN);
  power_adc_disable();                //Disables the ADC, should save significant power

  mesh.setNodeID(nodeID);             //Sets the nodeID manually
  
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
                                                                            //data rate set as low as possible to increase antenna sensitivity
}

void loop() 
{
  mesh.update();                      //Keeps mesh updated - testing required to see how this should be used when bringing node in and out of sleep

    network.sleepNode(1,0);           //Puts node in a low power state, can be woken by an interrupt

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

uint32_t takeTemperature()     //Measures temperature from a sensor - currently simulates behavior
{
  return(random(50));
}
