#include "RF24.h"

SerialClass Serial;

RF24::RF24(int, int) {}

void RF24::begin() {};
   
bool RF24::available() { return false; };

void RF24::setRetries(int, int) {};

void RF24::setPayloadSize(int) {};

void RF24::enableDynamicPayloads() {};

void RF24::setAutoAck (bool enable) {};

void RF24::setAutoAck (uint8_t pipe, bool enable) {};

uint8_t RF24::getDynamicPayloadSize() { return 1; };

void RF24::openReadingPipe(int, int) {};

void RF24::openWritingPipe(int) {};

void RF24::startListening() {};

void RF24::stopListening() {};

bool RF24::write(byte *, int) { return true; };
bool RF24::startWrite(byte *, int) { return true; };

bool RF24::read(byte *, int) { return false; };
    
void RF24::setDataRate(int) { };
void RF24::setPALevel(int) { };

void SerialClass::begin(int, int) {}
bool SerialClass::available() { return false; }

int SerialClass::readBytesUntil(byte, char *, int) { return 0; }

void SerialClass::write(const char *) {}

void SerialClass::write(unsigned long) {}

void SerialClass::println(const char *) {}
void SerialClass::println(int) {}
void SerialClass::print(const char *) {}
void SerialClass::print(int) {}

void pinMode(int, int) {
}

void digitalWrite(int, bool) {
}
void delay(int) {}

int analogRead(int) {
	return 0;
}
void randomSeed(int){
}
int random(int) {
  return 0;
}

int millis() {
  return 0;
}
