typedef unsigned char byte;
typedef unsigned long long uint64_t;

#include <RF24.h>

#ifndef Nightlight_h
#define Nightlight_h

const byte CONTROLLER_MAX_NODES = 8;  // Maximum number of nodes that can be controlled
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
class Map;


const byte MAP_MAX_ITEMS = 5;

const byte STATE_STACK_SIZE = 5;

/**
 * Represents a simple map with 5 elements; mapping keys to (void *) values
 */
class Map {
  public:
    Map();
    bool add(char *key, void *value);
    void *get(char *key);

  private:
    byte _numItems;
    char *_keys[MAP_MAX_ITEMS];
    void *_values[MAP_MAX_ITEMS];
};

class Nightlight {
  public:
    Nightlight(uint64_t broadcast);
    void setup();
    void loop();
    void sendMessage(byte address, byte type, byte *data, byte dataLength);
    void enableSerial();

    void pushState(NightlightState *state);
    void changeState(NightlightState *from, NightlightState *to);
    void removeState(NightlightState *state);
    
  private:
    uint64_t _broadcast; // The broadcast address, last 2 bytes must be 00
    byte _myAddressOffset; // The offset, 0-255, of the personal address
    RF24 _radio;
    NightlightState *_states[STATE_STACK_SIZE];
    byte _numStates;

    void _handleRadioInput();
    void _handleSerialInput();
};

/**
 * Represents a single state of your nighlight app
 */
class NightlightState {
  public:
    virtual void start(Nightlight *me);
    void finish(Nightlight *me);

    // Event handlers
    virtual void onTimeout(Nightlight *me);
    virtual void onFinished(Nightlight *me);
    virtual bool receiveMessage(Nightlight *me, byte address, byte type, byte *data, byte dataLength);
    virtual bool receiveSerial(Nightlight *me, char *line);

    // Configuration
    void onSerialCommandGoto(char *command, NightlightState *dest);
    void setTimeout(unsigned long millis);
    void notifyFinished(NightlightState *notify);

    unsigned long _timeout;

  private:
    Map _serialCommands;
    NightlightState *_notifyFinished;
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
 * An open node, waiting for a controller
 */
class OpenNode : public NightlightState { 
  public:
    void start(Nightlight *me);
    void onTimeout(Nightlight *me);
    bool receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength);

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
    void onFinished(Nightlight *me);
    bool receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength);
    
    void setCommand(NightlightState *command);
    void setState_lostControl(NightlightState *dest);
    
  private:
    uint64_t _controller;
    NightlightState *_state_lostControl;
    NightlightState *_command;
};

/**
 * An open node, waiting for a controller
 */
class ControllerState : public NightlightState { 
  public:
    void start(Nightlight *me);
    bool receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength);
    bool receiveSerial(Nightlight *me, char *line);

  private:
    byte _numControlling;
    byte _controlling[CONTROLLER_MAX_NODES];
    byte _visible[CONTROLLER_MAX_NODES];
};

/**
 * A blinking light
 */
class BlinkyLight : public NightlightState {
  void start(Nightlight *me);
  void onTimeout(Nightlight *me);

  private:
    bool _on;
    unsigned long _die;
};

/////////

void output(const char* asd);
void output(long unsigned int asd);
void outputln(const char* asd);
void outputln(long unsigned int asd);
void outputBytes(byte *data, byte len);
#endif