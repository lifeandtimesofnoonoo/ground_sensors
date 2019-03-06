# Ground sensors
3rd year group project at UoN - designing sensors to be placed in the ground.

There is a single master device that can comminucate with up to 254 slave devices. ATmega328P chips, using the Arduino bootloader, are used as the MCUs on all devices.
nRF24L01+ radios are used to communicate between all nodes.
The slaves in particular are designed to use as little battery power as possible, intending to spend months in the ground running on battery power.

## Required libraries:
[RF24](http://tmrh20.github.io/RF24/index.html)

[RF24 Network](http://tmrh20.github.io/RF24Network/index.html)

[RF24 Mesh](https://tmrh20.github.io/RF24Mesh/index.html)

All by [tmrh20](https://github.com/TMRh20) - These provide the basic funcionality of the radios as well as the backbone of the communications network.
