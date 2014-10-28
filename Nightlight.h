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
    virtual void receiveSerial(Nightlight *me, char *line);
};

/**
 * Represents a node that has a friend of some kind, loaded into _friendAddress.
 * Used, for example, to represent state with a controller.
 */
class NightlightStateWithFriend : public NightlightState {
  public:
    void setFriend(uint64_t friendAddress) {
      _friendAddress = friendAddress;
    }

  protected:
    uint64_t _friendAddress; 
};

/**
 * Represents an output/display device that it just "on" or "off".
 * Can be strobed, etc, by the caller
 */
class DigitalOutput {
    public:
        DigitalOutput(byte pin);
        DigitalOutput(byte pin, bool inverted);

        void setup();
        void on();
        void off();

    protected:
        byte _pin;
        bool _inverted;
};

/**
 * An open node, waiting for a controller
 */
class OpenNode : public NightlightState { 
  public:
    void start(Nightlight *me);
    void onTimeout(Nightlight *me);
    void receiveMessage(Nightlight *me, uint64_t sender, byte type);
//    void receiveSerial(Nightlight *me, char *line);

    void setState_controlled(NightlightStateWithFriend *dest);

  private:
    NightlightStateWithFriend *_state_controlled;

};  

/**
 * An open node, waiting for a controller
 */
class ControlledNode : public NightlightStateWithFriend { 
  public:
    void start(Nightlight *me);
    void onTimeout(Nightlight *me);
    void receiveMessage(Nightlight *me, uint64_t sender, byte type);
    
    void setOutput(DigitalOutput *output);
    void setState_lostControl(NightlightState *dest);
    
  private:
    uint64_t _controller;
    NightlightState *_state_lostControl;
    DigitalOutput *_output;
};

/////////

void output(const char* asd);
void output(long unsigned int asd);
void outputln(const char* asd);
void outputln(long unsigned int asd);

#endif