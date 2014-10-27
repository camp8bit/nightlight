#include "RF24.h"

#ifndef Nightlight_h
#define Nightlight_h

// Message types

// Presence notification
const byte MSG_HELLO = 1; // Send this 

// Negotation remote control of devices
const byte MSG_CONTROL_REQUEST = 8; // Request control, can be broadcast or unicast
const byte MSG_CONTROL_START = 9; // Positive response to MSG_CONTROL_REQUEST to start remote-control
const byte MSG_CONTROL_STOP = 10; // Cancel a previous remote-control session

// Sending commands to remote-controlled devices
const byte MSG_COMMAND_SEND = 16; // Send a command to a remote-controlled device
const byte MSG_COMMAND_START = 17; // Successfully received a remote-control command, and activity started
const byte MSG_COMMAND_END = 18; // Activity finished (e.g. animation completed)

// Events
const byte MSG_EVENT = 24;

class Nightlight;
class NightlightState;

class Nightlight {
  public:
    Nightlight(uint64_t broadcast);
    void setup();
    void loop();
    void sendMessage(uint64_t address, byte type);
    void broadcastMessage(byte type);
    void setState(NightlightState *state);
    void setTimeout(unsigned long millis);
    
  private:
    uint64_t _broadcast;
    uint64_t _myAddress;
    RF24 _radio;
    NightlightState *_state;
    unsigned long _timeout;

};

/**
 * Represents a single state of your nighlight app
 */
class NightlightState {
  public:
    virtual void start(Nightlight *me);
    virtual void onTimeout(Nightlight *me);
    virtual void receiveMessage(Nightlight *me, uint64_t address, byte type);
    virtual void receiveMessage(Nightlight *me, char *line);
};


/////////

void output(const char* asd);
void output(long unsigned int asd);
void outputln(const char* asd);
void outputln(long unsigned int asd);

#endif