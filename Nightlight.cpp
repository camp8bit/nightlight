#include <RF24.h>
#include <string.h>
#include "Nightlight.h"

Nightlight::Nightlight(uint64_t broadcast) : _radio(9,10)
{
  _broadcast = broadcast;
  _numStates = 0;
}

void Nightlight::setup()
{
  
  randomSeed(analogRead(3));
  _myAddressOffset = random(256);
  Serial.print("\n00 My address is ");
  Serial.println(_myAddressOffset);

  _radio.begin();

  // optionally, reduce the payload size. seems to improve reliability
  _radio.setPayloadSize(16);
  _radio.enableDynamicPayloads();
  _radio.setRetries(15,15);

  _radio.setDataRate(RF24_2MBPS);
  _radio.setPALevel(RF24_PA_HIGH);

  // Listen on the broadcast port
  _radio.openReadingPipe(0, _broadcast);
  _radio.openReadingPipe(1, _broadcast + (uint64_t)_myAddressOffset);
  _radio.setAutoAck(0, false);

  _radio.startListening();
}

void Nightlight::enableSerial() {
  Serial.begin(57600, SERIAL_8N1);
  outputln("00 Nightlight - serial communication activated");
  output("00 Listening on ");
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
    _handleSerialInput();
  }
  
  // Check for timeouts
  int i;
  unsigned long m = millis();
  for(i=0; i<_numStates; i++) {
    if(_states[i]->_timeout && _states[i]->_timeout < m) {
      _states[i]->_timeout = 0;
      _states[i]->onTimeout(this);
    }
  }
}

void Nightlight::_handleRadioInput() {
  byte message[32];

  // Fetch the payload, and see if this was the last one.
  byte messageSize = _radio.getDynamicPayloadSize();

  _radio.read(message, 32);

  // Debug message receive
  output("00 message received from #");
  Serial.print(message[1]);
  output(": type ");
  Serial.print(message[0]);
  output("; data: ");
  outputBytes(message+2, messageSize-2);
  Serial.write('\n');

  // Bubble message through states until one receives the inout
  int i;
  for(i=_numStates-1;i>=0;i--) {
    if(_states[i]->receiveMessage(this, message[1], message[0], message+2, messageSize-2)) break;
  }
}

void Nightlight::_handleSerialInput() {
  char c[80];
  byte numChars;

  // Collect a line from the serial device (of up to 80 char)
  numChars = Serial.readBytesUntil('\n', c, 80);
  c[numChars] = 0;

  byte type = hexPair(c);

  // Bubble message through states until one receives the input
  int i;
  for(i=_numStates-1;i>=0;i--) {
    if(_states[i]->receiveMessage(this, -1, type, (byte *)c+3, numChars-3)) break;
  }
}



/**
 * Send a message to an address
 * 
 * Messages can be sent to this node's address, and will be processed internally
 * rather than sent over radio.
 *
 * @param address 0 for broadcast, 1-255 for a specific recipient, -1 for serial
 */
void Nightlight::sendMessage(int address, byte type, byte *data, byte dataLength)
{

  int i;
  byte packet[32];

  // Debug message about sending
  output("00 Sending message to ");
  Serial.print(address);
  output(". My address: ");
  Serial.print(_myAddressOffset);
  output("Type: ");
  Serial.print(type);
  output(", data: ");
  outputBytes(data, dataLength);
  Serial.write('\n');

  // Internal message to send back to other states
  if(address == _myAddressOffset) {
    // Bubble message through states until one receives the inout
    for(i=_numStates-1;i>=0;i--) {
      if(_states[i]->receiveMessage(this, _myAddressOffset, type, data, dataLength)) break;
    }
  }

  // Serial message
  else if(address == -1) {
    Serial.print(type, HEX);
    Serial.print(' ');
    Serial.println((char *)data);

  }

  // Radio message
  else {
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
}

/**
 * Switch from one state to another
 */
void Nightlight::changeState(NightlightState *from, NightlightState *to)
{
  this->removeState(from);
  this->pushState(to);
}

/**
 * Remove a state from the stack (needn't be at the top of the stack)
 */
void Nightlight::removeState(NightlightState *state)
{
  int i;
  // Find the place to remove an item
  for(i=0; i<_numStates; i++) {
    if(_states[i] == state) break;
  }
  // Shift all other items back 1
  for(; i<_numStates-1;i++) {
    _states[i] = _states[i+1];
  }
  _numStates--;
}

/**
 * Push a new state onto the stack
 */
void Nightlight::pushState(NightlightState *state)
{
  // Add the item to the stack
  _states[_numStates] = state;
  _numStates++;

  state->_timeout = 0;
  state->start(this);
}

///////////////////////////////////////////////////////

void NightlightState::setTimeout(unsigned long timeout)
{
  _timeout = millis() + timeout;
}

void NightlightState::finish(Nightlight *me)
{
  me->removeState(this);

  // If another state is watching for the end of this one, notify
  if((long)_notifyFinished > 0) {
    _notifyFinished->onFinished(me);
  }
}

/**
 * Attach another state to be notifed of the finish of this one.
 * state->onFinished(nightlight) will be called.
 */
void NightlightState::notifyFinished(NightlightState *notify) {
  _notifyFinished = notify;
}

void NightlightState::start(Nightlight *me) {
}

void NightlightState::onTimeout(Nightlight *me) {
}
void NightlightState::onFinished(Nightlight *me) {
}

bool NightlightState::receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength) {
  if(type == MSG_CHANGE_MODE && sender == -1) {
    // Look up a built-in command
    NightlightState *dest = (NightlightState *)_serialCommands.get((char *)data);
    if((long)dest != 0) {
      Serial.println("00 Switching state");
      me->pushState(dest);
      return true;

    } else {
      Serial.print("00 Unknown mode '");
      Serial.print((char *)data);
      Serial.println("'");
    }
  }

  return false;
}

/**
 * Add a serial command that will switch to another state
 */
void NightlightState::onSerialCommandGoto(char *command, NightlightState *dest) {
  _serialCommands.add(command, dest);
}

///////////////////////////////////////////////////////

void OpenNode::setState_controlled(NightlightStateWithFriend *dest) {
  _state_controlled = dest;
}

void OpenNode::start(Nightlight *me) {
  onTimeout(me);
}

void OpenNode::onTimeout(Nightlight *me) {
  me->sendMessage(0, MSG_HELLO, (byte *)"OpenNode", 8);
  this->setTimeout(2000);
}
  
bool OpenNode::receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength)
{
  if(type == MSG_CONTROL_REQUEST) {
    _state_controlled->setFriend(sender);
    me->pushState(_state_controlled);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////////

void ControlledNode::setCommand(NightlightState *command) {
  _command = command;
}
void ControlledNode::setState_lostControl(NightlightState *dest) {
  _state_lostControl = dest;
}
void ControlledNode::start(Nightlight *me) {
  me->sendMessage(_friendAddress, MSG_CONTROL_START, 0, 0);
}
void ControlledNode::onFinished(Nightlight *me) {
  me->sendMessage(_friendAddress, MSG_COMMAND_END, 0, 0);
}

bool ControlledNode::receiveMessage(Nightlight *me, int sender, byte type, byte *data, byte dataLength)
{
  if(type == MSG_COMMAND_SEND) {
    if(sender == _friendAddress) {
      me->sendMessage(_friendAddress, MSG_COMMAND_START, 0, 0);
      me->pushState(_command);
      _command->notifyFinished(this);
      return true;
    }
  }

  // Prevent the event from bubbling
  if(type == MSG_CONTROL_REQUEST) return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////////

void ControllerState::start(Nightlight *me) {
  _controlling = 0;
}

bool ControllerState::receiveSerial(Nightlight *me, char *line)
{
  if(line[0] == 'B') {
    if(_controlling > 0) {
      Serial.print("Sending a beat to ");
      Serial.println(_controlling);

      me->sendMessage(_controlling, MSG_COMMAND_SEND, 0, 0);
      
    } else {
      Serial.println("00 No nodes currently under control");
    }
    return true;
  }

  return false;
}

bool ControllerState::receiveMessage(Nightlight *me, byte sender, byte type, byte *data, byte dataLength)
{
  if(type == MSG_HELLO) {
    Serial.print("00 Received MSG_HELLO from ");
    Serial.println(sender);
    me->sendMessage(sender, MSG_CONTROL_REQUEST, 0, 0);
    return true;
  }    

  if(type == MSG_CONTROL_START) {
    Serial.print("00 Received MSG_CONTROL_START from ");
    Serial.println(sender);
    _controlling = sender;
  }

  if(type == MSG_COMMAND_START) {
    Serial.print("00 Received MSG_COMMAND_START from ");
    Serial.println(sender);
  }
  if(type == MSG_COMMAND_END) {
    Serial.print("00 Received MSG_COMMAND_END from ");
    Serial.println(sender);
  }

  return false;
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

void BlinkyLight::start(Nightlight *me) {
  pinMode(2, OUTPUT);

  _on = true;
  _die = millis() + 2000;

  this->onTimeout(me);
}

void BlinkyLight::onTimeout(Nightlight *me) {
  digitalWrite(2, _on);
  _on = !_on;

  if(millis() > _die) {
    digitalWrite(2, false);
    this->finish(me);

  } else { 
   this->setTimeout(100);
  }
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

/**
 * Convert a 2-hex-character ascii-encoded value into a byte, 0-255
 */
byte hexPair(char *ascii) {
  return hexChar(ascii[0]) * 16 + ascii[1];
}

/**
 * Convert a hex-character ascii-encoded value into a byte, 0-15
 */
byte hexChar(char ascii) {
  ascii = toupper(ascii);
  return (ascii >= 'A') ? ascii - 'A' + 10 : ascii - '0';
}