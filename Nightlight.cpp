#include "RF24.h"
#include "Nightlight.h"

Nightlight::Nightlight(uint64_t broadcast) : _radio(9,10)
{
  _broadcast = broadcast;
  _myAddress = broadcast + random(256);
}

void Nightlight::setup()
{
  
  _radio.begin();

  // optionally, increase the delay between retries & # of retries
  _radio.setRetries(15,15);

  // optionally, reduce the payload size. seems to improve reliability
  _radio.setPayloadSize(8);

  // Listen on the broadcast port
  _radio.openReadingPipe(1, _broadcast);
  _radio.openReadingPipe(2, _myAddress);

  _radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  //  radio.printDetails();  
}

void Nightlight::loop()
{
  bool done = false;

  // Check for radio messages
  if ( _radio.available() ) {
    // Dump the payloads until we've gotten everything
    byte messageType;
    while (!done) {
      // Fetch the payload, and see if this was the last one.
      done = _radio.read( &messageType, sizeof(byte) );
      _state->receiveMessage(this, 0, messageType);

      output("Got payload");
      outputln(messageType);
    }
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

/**
 * Send a message to a specific address
 */
void Nightlight::sendMessage(uint64_t address, byte type)
{
  outputln("Sending message");

  _radio.stopListening();
  _radio.openWritingPipe(address);

  if(!_radio.write( &type, sizeof(byte) )) {
    // error 
  }

  _radio.startListening();
}

/**
 * Broadcast a message - send it on the broadcast address
 */
void Nightlight::broadcastMessage(byte type)
{
  outputln("Broadcasting message");
  sendMessage(_broadcast, type);
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
void NightlightState::receiveMessage(Nightlight *me, uint64_t address, byte type) {
}
void NightlightState::receiveSerial(Nightlight *me, char *line) {
}

///////////////////////////////////////////////////////

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