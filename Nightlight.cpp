#include <RF24.h>
#include "Nightlight.h"

Nightlight::Nightlight(uint64_t broadcast) : _radio(9,10)
{
  _broadcast = broadcast;
}

void Nightlight::setup()
{
  
  randomSeed(analogRead(3));
  _myAddressOffset = random(256);
  Serial.print("My address is ");
  Serial.println(_myAddressOffset);

  _radio.begin();

  // optionally, reduce the payload size. seems to improve reliability
  _radio.enableDynamicPayloads();
  _radio.setPayloadSize(16);
  _radio.setRetries(15,15);

  // Listen on the broadcast port
  _radio.openReadingPipe(0, _broadcast);
  _radio.openReadingPipe(1, _broadcast + (uint64_t)_myAddressOffset);
  _radio.setAutoAck(0, false);

  _radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  //  radio.printDetails();  
}

void Nightlight::enableSerial() {
  Serial.begin(57600, SERIAL_8N1);
  outputln("\nNightlight - serial communication activated");
  output("Listening on ");
  Serial.print((long unsigned int)_broadcast);
  output("-");
  Serial.println(_myAddressOffset);
}


void Nightlight::loop()
{

  // Check for radio messages
  if ( _radio.available() ) {
    _handleRadioInput();
  }

  // Check for serial messages
  if ( Serial.available() ) {
    char c[80];
    byte numChars;
    numChars = Serial.readBytesUntil('\n', c, 80);
    c[numChars] = 0;
    _state->receiveSerial(this, c);
  }
  
  // Check for timeout
  if(_timeout && _timeout <= millis()) {
    _timeout = 0;
    _state->onTimeout(this);
  }
}

void Nightlight::_handleRadioInput() {
  bool done = false;
  byte message[32];

  // Fetch the payload, and see if this was the last one.
  byte messageSize = _radio.getDynamicPayloadSize();

  done = _radio.read(message, 32);
  _state->receiveMessage(this, message[1], message[0], message+2, messageSize-2);

}



/**
 * Send a message to an address
 * @param address 0 for broadcast, 1-255 for a specific recipient
 */
void Nightlight::sendMessage(byte address, byte type, byte *data, byte dataLength)
{

  byte packet[32];

  output("Sending message to ");
  Serial.print(address);
  output(". My address: ");
  Serial.print(_myAddressOffset);
  output("Type: ");
  Serial.print(type);
  output(", data: ");
  outputBytes(data, dataLength);
  Serial.write('\n');

  _radio.stopListening();
  _radio.openWritingPipe(_broadcast + address);

  // Build packet
  packet[0] = type;
  packet[1] = _myAddressOffset;
  for(int i=0;i<dataLength;i++) {
    packet[i+2] = data[i];
  }

  // Non-blocking, no error checking
  _radio.write(packet, dataLength + 2 );

  _radio.startListening();
}

void Nightlight::setTimeout(unsigned long timeout)
{
  _timeout = millis() + timeout;
}

void Nightlight::setState(NightlightState *state)
{
  _state = state;
  _state->start(this);
}

///////////////////////////////////////////////////////

void NightlightState::start(Nightlight *me) {
}
void NightlightState::onTimeout(Nightlight *me) {
}
void NightlightState::receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength) {
}

void NightlightState::receiveSerial(Nightlight *me, char *line) {
  // Look up a built-in command
  NightlightState *dest = (NightlightState *)_serialCommands.get(line);
  if((int)dest != 0) {
    Serial.println("Switching state");
    me->setState(dest);

  } else {
    Serial.print("Unknown command '");
    Serial.print(line);
    Serial.println("'");
  }
}

/**
 * Add a serial command that will switch to another state
 */
void NightlightState::onSerialCommandGoto(char *command, NightlightState *dest) {
  _serialCommands.add(command, dest);
}

///////////////////////////////////////////////////////

DigitalOutput::DigitalOutput(byte pin) {
  _pin = pin;
  _inverted = false;
}
DigitalOutput::DigitalOutput(byte pin, bool inverted) {
  _pin = pin;
  _inverted = inverted;
}

void DigitalOutput::setup() {
  pinMode(_pin, OUTPUT);
}
void DigitalOutput::on() {
  digitalWrite(_pin, !_inverted);
}
void DigitalOutput::off() {
  digitalWrite(_pin, _inverted);
}

////////////////////////////////////////////////////////////////////////////////////


void OpenNode::setState_controlled(NightlightStateWithFriend *dest) {
  _state_controlled = dest;
}

void OpenNode::start(Nightlight *me) {
  onTimeout(me);
}

void OpenNode::onTimeout(Nightlight *me) {
  me->sendMessage(0, MSG_HELLO, (byte *)"OpenNode", 8);
  me->setTimeout(5000);
}
  
void OpenNode::receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength)
{
  if(type == MSG_CONTROL_REQUEST) {
    _state_controlled->setFriend(sender);
    me->setState(_state_controlled);
  }

  output("message received from #");
  Serial.print(sender);
  output(": type ");
  Serial.print(type);
  output("; data: ");
  outputBytes(data, dataLength);
  Serial.write('\n');
}

////////////////////////////////////////////////////////////////////////////////////

void ControlledNode::setOutput(DigitalOutput *output) {
  _output = output;
}
void ControlledNode::setState_lostControl(NightlightState *dest) {
  _state_lostControl = dest;
}
void ControlledNode::start(Nightlight *me) {
  me->sendMessage(_friendAddress, MSG_CONTROL_START, 0, 0);
}
void ControlledNode::onTimeout(Nightlight *me) {
  _output->off();
  me->sendMessage(_friendAddress, MSG_COMMAND_END, 0, 0);
}

void ControlledNode::receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength)
{
  if(type == MSG_COMMAND_SEND) {
    if(sender == _friendAddress) {
      me->sendMessage(_friendAddress, MSG_COMMAND_START, 0, 0);
      _output->on();
      me->setTimeout(1000);
    }
  }    
}

////////////////////////////////////////////////////////////////////////////////////

void ControllerState::start(Nightlight *me) {
  _controlling = 0;
}

void ControllerState::receiveSerial(Nightlight *me, char *line)
{
  if(line[0] == 'B') {
    if(_controlling > 0) {
      Serial.print("Sending a beat to ");
      Serial.println(_controlling);
    } else {
      Serial.print("No nodes currently under control");
    }
  } 
}

void ControllerState::receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength)
{
  if(type == MSG_HELLO) {
    Serial.print("Received MSG_HELLO from ");
    Serial.println(sender);
    me->sendMessage(sender, MSG_CONTROL_REQUEST, 0, 0);
  }    

  if(type == MSG_CONTROL_START) {
    Serial.print("Received MSG_CONTROL_START from ");
    Serial.println(sender);
    _controlling = sender;
  }

  if(type == MSG_COMMAND_START) {
    Serial.print("Received MSG_COMMAND_START from ");
    Serial.println(sender);
  }
  if(type == MSG_COMMAND_END) {
    Serial.print("Received MSG_COMMAND_END from ");
    Serial.println(sender);
  }
}


void output(const char* asd) {
    /*if (DEBUG)*/ Serial.write(asd);    
}

void output(long unsigned int asd) {
    /*if (DEBUG)*/ Serial.write(asd);
}

void outputln(const char* asd) {
    /*if (DEBUG)*/ Serial.println(asd);
}

void outputln(long unsigned int asd) {
    /*if (DEBUG)*/ Serial.write(asd);
}

void outputBytes(byte *data, byte len) {
  byte i;
  for(i=0;i<len;i++) {
    Serial.print(data[i]);
    Serial.print(',');
  }
  Serial.print(" ('");
  for(i=0;i<len;i++) {
    Serial.write(data[i]);
  }
  Serial.print("')");

}

////////////////////////////////////////////////////////////////////////////////////

Map::Map() {
  _numItems = 0;
}

/**
 * Add an item to the map
 * Returns true if it could, false if it's full
 */
bool Map::add(char *key, void *value) {
  if(_numItems < MAP_MAX_ITEMS) {
    _keys[_numItems] = key;
    _values[_numItems] = value;
    _numItems++;
    return true;
  }
  // Sorry, we're full
  return false;
}

/**
 * Return the item corresponding to this key
 */
void *Map::get(char *key) {
  byte i;
  for(i=0; i<_numItems; i++) {
    if(strcmp(key, _keys[i]) == 0) {
      return _values[i];
    }
  }

  // No match
  return (void *)0;
}
