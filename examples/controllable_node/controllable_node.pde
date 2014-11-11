
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
ControllerState controllerState;

// This state is basically a command
BlinkyLight blinky;


void setup(void)
{
   Serial.begin(57600, SERIAL_8N1);
  
  // Wire together the states (dependency injection in Arduino, oh my)
  openNode.setState_controlled(&controlledNode);
  controlledNode.setState_lostControl(&openNode);

  // Swapping between states triggered from commands on the serial input
  openNode.onSerialCommandGoto("controller", &controllerState);
  controllerState.onSerialCommandGoto("node", &openNode);

  openNode.onSerialCommandGoto("B", &blinky);

  // Connect the LED to the output 
  controlledNode.setCommand(&blinky);

  nightlight.setup();

  nightlight.enableSerial();

  // Starting state
  nightlight.pushState(&openNode);
}

void loop()
{
  nightlight.loop();
}
