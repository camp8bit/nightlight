
/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Nightlight.h>

#include <serial>

bool DEBUG = true;

////////////////////////////////////////////////////////////////////////////////////

// The broadcast address defines the address space
// Everyone will listen on this address
Nightlight nightlight(0x26B8259100LL);

// States for the nightlight state machine
OpenNode openNode;
ControlledNode controlledNode;

// Output device - an LED on pin #2
DigitalOutput led(2);

void setup(void)
{
//  nightlight.enableSerial();

  // Wire together the states (dependency injection in Arduino, oh my)
  openNode.setState_controlled(&controlledNode);
  controlledNode.setState_lostControl(&openNode);

  // Swapping between states triggered from commands on the serial input
  // openNode.onSerialCommandGoto("controller", &controllerState);
  // controllerState.onSerialCommandGoto("node", &openNode);
  
  // Connect the LED to the output
  led.setup();
  controlledNode.setOutput(&led);

  // Starting state
  nightlight.setState(&openNode);

  nightlight.setup();
}

void loop()
{
  nightlight.loop();
}
