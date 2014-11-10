// Arduino types
typedef unsigned char byte;
typedef unsigned long long uint64_t;
typedef unsigned char uint8_t;

#ifndef RF24_h
#define RF24_h

// Test stub for RF24
#define RF24_h
class RF24 {
  public:
    RF24(int, int);
    void begin();
    bool available();
    void setRetries(int, int);
    void setPayloadSize(int);
    void openReadingPipe(int, int);
    void openWritingPipe(int);
    void startListening();
    void stopListening();
    void enableDynamicPayloads();
    void setAutoAck (bool enable);   
    void setAutoAck (uint8_t pipe, bool enable);   
    void setDataRate(int);
    void setPALevel(int);

    uint8_t getDynamicPayloadSize();
    bool write(byte *, int);
    bool startWrite(byte *, int);
    bool read(byte *, int);
    
  private:
};


class SerialClass {
  public:
    void begin(int, int);
    bool available();
    int readBytesUntil(byte, char *, int);
    void write(const char *);
    void write(unsigned long);
    void println(const char *);
    void println(int);
    void print(const char *);
    void print(int);
};

extern SerialClass Serial;

void pinMode(int, int);

void digitalWrite(int, bool);
int analogRead(int);
void delay(int);

void randomSeed(int);
int random(int);

int millis();

const int OUTPUT = 1;
const int SERIAL_8N1 = 0;

const int RF24_2MBPS = 1;
const int RF24_PA_HIGH = 1;

#endif

