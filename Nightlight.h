typedef unsigned char byte;
typedef unsigned long long uint64_t;

#include <RF24.h>

#ifndef Nightlight_h
#define Nightlight_h

// Configuration constants

const byte MAP_MAX_ITEMS = 5;    // Maximum number of items in a Map class
const byte CONTROLLER_MAX_NODES = 8;  // Maximum number of nodes that can be controlled
const byte STATE_STACK_SIZE = 5; // Maximum number of concurrently-running states
const int FRAME_LENGTH = 25;     // Frame length in msec


// Message types

// Presence notification
const byte MSG_HELLO = 0x01; // Send this every X secondas
const byte MSG_APPEAR = 0x02; // Generate this when new nodes appear (e.g. from FriendList)
const byte MSG_DISAPPEAR = 0x03; // Generate this when nodes disappear (e.g. from FriendList)

// Negotation remote control of devices
const byte MSG_CONTROL_REQUEST = 0x08; // Request control, can be broadcast or unicast
const byte MSG_CONTROL_START = 0x09; // Positive response to MSG_CONTROL_REQUEST to start remote-control
const byte MSG_CONTROL_STOP = 0x0A; // Cancel a previous remote-control session

// Sending commands to remote-controlled devices
const byte MSG_COMMAND_SEND = 0x10; // Send a command to a remote-controlled device
const byte MSG_COMMAND_START = 0x11; // Successfully received a remote-control command, and activity started
const byte MSG_COMMAND_END = 0x11; // Activity finished (e.g. animation completed)

// Events
const byte MSG_EVENT = 0x18;

// Operational control (from serial)
const byte MSG_CHANGE_MODE = 0x20;

const byte FRIENDLIST_MAX_NODES = 16;

class Nightlight;
class NightlightState;
class Map;

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
    void sendMessage(int address, byte type, byte *data, byte dataLength);
    void enableSerial();

    void pushState(NightlightState *state);
    void changeState(NightlightState *from, NightlightState *to);
    void removeState(NightlightState *state);

    byte _myAddressOffset; // The offset, 0-255, of the personal address
    
  private:
    uint64_t _broadcast; // The broadcast address, last 2 bytes must be 00
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


/*
sender: 1-255 = radio devices
      : 0     = unknown
      : -1    = serial
*/
    virtual bool receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength);

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
    bool receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength);

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
    bool receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength);
    
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
    bool receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength);

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


/**
 * An agent that keeps state of who is here, sending MSG_APPEAR and MSG_DISAPPEAR messages.
 */
class FriendList : public NightlightState { 
  public:
    void start(Nightlight *me);
    void onTimeout(Nightlight *me);
    bool receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength);

  private:
    byte _numFriends;
    byte _friends[FRIENDLIST_MAX_NODES];
    unsigned long _timeout[FRIENDLIST_MAX_NODES];
};


/////////

void output(const char* asd);
void output(long unsigned int asd);
void outputln(const char* asd);
void outputln(long unsigned int asd);
void outputBytes(byte *data, byte len);
#endif


byte hexPair(char *ascii);
byte hexChar(char ascii);