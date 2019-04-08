/*
    The intended purpose of this code is to allow any slave sensor node to take sensor readings and relay them back to
    the master node via a mesh network while spending as much time as possible in a low power state.

    ***************************************************************
    ***************************************************************
    TO USE:
    Uncomment #define ENABLE_SLEEP_MODE in RF24Network_config.h
    ***************************************************************
    ***************************************************************
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

//#define gs_serial_debug               //Uncomment to enable serial debugging

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

RF24 radio(7, 8);                     //Sets the CE and CS pins
RF24Network network(radio);           //Begins network using the nRF24L01+ radio
RF24Mesh mesh(radio, network);        //Defines network as a mesh

const int nodeID = 1;                 //Unique ID of node. In future should be set dynamically during setup
const int nodeDepth = 60;             //Depth of sensor
uint32_t timer = 0;                   //Used for time keeping

struct dataPacket                     //Stucture of data sent over RF
{
  float temperature;                  //Measured temperature
  uint8_t uniqueNodeID;               //Unique ID of node - used to know where data was recorded
  uint8_t nodeDepth;                  //Depth of the sensor on this node
};

void setup() 
{
  #ifdef gs_serial_debug
  Serial.begin(115200);               //For debugging - final product unlikely to use serial
  #endif

  sensors.begin();
  
  #ifdef gs_serial_debug
  Serial.println(F("Sensor test:"));
  Serial.println(takeTemperature());
  #endif

  // network.setup_watchdog(9);          //Sets up watchdog timer, 9 denotes a time period of 8 seconds, the longest possible (I'm pretty certain this means the greatest power saving)
  
  //SETUP WATCHDOG TIMER
  WDTCSR = (24);          //change enable and WDE - also resets
  WDTCSR = (33);          //prescalers only - get rid of the WDE and WDCE bit
  WDTCSR |= (1<<6);       //enable interrupt mode

  //Disable ADC - don't forget to flip back after waking up if using ADC in your application ADCSRA |= (1 << 7);
  ADCSRA &= ~(1 << 7);
  
  //ENABLE SLEEP - this enables the sleep mode
  SMCR |= (1 << 2);     //power down mode
  SMCR |= 1;            //enable sleep

  mesh.setNodeID(nodeID);             //Sets the nodeID manually

  #ifdef gs_serial_debug
  Serial.println(F("I am a slave!"));
  #endif
  
  mesh.begin(74, RF24_250KBPS, MESH_RENEWAL_TIMEOUT);     //Required for connecting to the mesh
                                                                            //data rate set as low as possible to increase antenna sensitivity
  #ifdef gs_serial_debug
  Serial.println(F("Connected to the mesh"));
  #endif
}

void loop() 
{
  mesh.update();                      //Keeps mesh updated - testing required to see how this should be used when bringing node in and out of sleep

    
   // network.sleepNode(1,0);           //Puts node in a low power state, can be woken by an interrupt - part of RF24Network, doesn't seem to be working
    goToSleep(1);
    
    struct dataPacket toBeSent = {takeTemperature(), nodeID, nodeDepth};

    #ifdef gs_serial_debug
    Serial.println("Sending data...");
    #endif

    if(!mesh.write(&toBeSent, 'M', sizeof(toBeSent)))   //If write fails, check conectivity
    {
      if(!mesh.checkConnection())                         //Refresh the network address 
      {
        #ifdef gs_serial_debug
        Serial.println("Renewing Address");
        #endif
        
        mesh.renewAddress();                              //Re-establishes connection to mesh
      } 
      else 
      {
        #ifdef gs_serial_debug
        Serial.println("Send fail, Network test OK");
        #endif
      }
    }
}

void goToSleep(int cycles)      //Cycles are periods of 8 seconds
{
  #ifdef gs_serial_debug
  Serial.println("Sleeping...");
  Serial.println("");
  delay(5);
  #endif
  
  radio.powerDown();      //Powers down radio module - testing may be required as to how this affects mesh functionality - should also be able to reduce power consumption without completely powering down?

  for (cycles; cycles >= 0; cycles--)          
  {
    //BOD DISABLE - this must be called right before the __asm__ sleep instruction
    MCUCR |= (3 << 5);                          //set both BODS and BODSE at the same time
    MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);     //then set the BODS bit and clear the BODSE bit at the same time
    __asm__  __volatile__("sleep");             //in line assembler to go to sleep
  }

  radio.powerUp();        //Restarts radio module

  #ifdef gs_serial_debug
  Serial.println("I'm awake!");
  #endif
}

float takeTemperature()     //Measures temperature from a sensor
{
  sensors.requestTemperatures();      //Send the command to get temperatures
  return(sensors.getTempCByIndex(0));
}
