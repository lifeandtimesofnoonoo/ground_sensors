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
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

RF24 radio(7, 8);                     //Sets the CE and CS pins
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

int nodeID = EEPROM.read(0);          //Unique ID of node. Taken from EEPROM, default is 255, available ID gets allocated by master
const int nodeDepth = 30;             //Depth of sensor, some way of setting this automatically based on length of sensor?
uint32_t timer = 0;                   //Used for time keeping
bool mainFunction = false;

struct dataPacket                     //Stucture of data sent over RF
{
  float temperature;                  //Measured temperature
  uint8_t uniqueNodeID;               //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth;                  //Depth of the sensor on this node
};

void setup()
{
  Serial.begin(115200);               //For debugging - final product unlikely to use serial

  sensors.begin();

  Serial.println(F("Sensor test:"));
  Serial.println(takeTemperature());

  // network.setup_watchdog(9);          //Sets up watchdog timer, 9 denotes a time period of 8 seconds, the longest possible (I'm pretty certain this means the greatest power saving)

  //SETUP WATCHDOG TIMER
  WDTCSR = (24);          //change enable and WDE - also resets
  WDTCSR = (33);          //prescalers only - get rid of the WDE and WDCE bit
  WDTCSR |= (1 << 6);     //enable interrupt mode

  //Disable ADC - don't forget to flip back after waking up if using ADC in your application ADCSRA |= (1 << 7);
  ADCSRA &= ~(1 << 7);

  //ENABLE SLEEP - this enables the sleep mode
  SMCR |= (1 << 2);     //power down mode
  SMCR |= 1;            //enable sleep

  mesh.setNodeID(nodeID);             //Sets the nodeID manually

  Serial.println(F("I am a slave!"));

  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
  //data rate set as low as possible to increase antenna sensitivity
  Serial.println(F("Connected to the mesh"));

  mesh.write(&nodeID, 'C', sizeof(nodeID));   //Needs some form of error checking
//  while (mesh.write(&nodeID, 'C', sizeof(nodeID)) == false)   //No useful data sent, header used to request node ID
//  {
//    if (!mesh.checkConnection())                        //Refresh the network address
//    {
//      Serial.println("Renewing Address");
//      mesh.renewAddress();                              //Re-establishes connection to mesh
//    }
//    else
//    {
//      Serial.println("Send fail, Network test OK");
//    }
//  }

//  Serial.println("Waiting to be allocated nodeID");
//  delay(1);
//  RF24NetworkHeader header;
//
//
//  while (network.read(header, &nodeID, sizeof(nodeID)) == false)
//  {
//  }
//
//  delay(1);
//  Serial.println("My new ID is:");
//  Serial.println(nodeID);
//  delay(1);
//
//  mesh.releaseAddress();
//  mesh.setNodeID(nodeID);
//  mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);

}

void loop()
{
  mesh.update();                      //Keeps mesh updated - testing required to see how this should be used when bringing node in and out of sleep

  if (mainFunction == true)
  {

    goToSleep(1);

    struct dataPacket toBeSent = {takeTemperature(), nodeID, nodeDepth};

    Serial.println("Sending data...");

    if (!mesh.write(&toBeSent, 'D', sizeof(toBeSent)))  //If write fails, check conectivity
    {
      if (!mesh.checkConnection())                        //Refresh the network address
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

  if(network.available())       //Framework for receiving messages
  {
    RF24NetworkHeader header;
    network.peek(header);
    switch(header.type)
    {
      case 'C':                 //As with the master, messages are split into types specified in their headers
      network.read(header, &nodeID, sizeof(nodeID));
      mesh.releaseAddress();
      mesh.setNodeID(nodeID);               //Testing required to figure out what needs to happen to connect a node with a new node ID
      //mesh.begin(MESH_DEFAULT_CHANNEL, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);
      mesh.renewAddress();
      delay(1);
      Serial.println("My new ID is:");
      Serial.println(nodeID);
      delay(1);
      mainFunction = true;    //Slave told to start taking and sending measurements after receiving new ID
      break;

      default:
      network.read(header,0,0);
      break;
    }
  }
  
}

void goToSleep(int cycles)
{
  Serial.println("Sleeping...");
  Serial.println("");
  delay(2);
  radio.powerDown();      //Powers down radio module - testing may be required as to how this affects mesh functionality - should also be able to reduce power consumption without completely powering down?

  for (cycles; cycles >= 0; cycles--)
  {
    //BOD DISABLE - this must be called right before the __asm__ sleep instruction
    MCUCR |= (3 << 5);                          //set both BODS and BODSE at the same time
    MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);     //then set the BODS bit and clear the BODSE bit at the same time
    __asm__  __volatile__("sleep");             //in line assembler to go to sleep
  }

  radio.powerUp();        //Restarts radio module
  Serial.println("I'm awake!");
}

float takeTemperature()     //Measures temperature from a sensor
{
  sensors.requestTemperatures();      //Send the command to get temperatures
  return (sensors.getTempCByIndex(0));
}
