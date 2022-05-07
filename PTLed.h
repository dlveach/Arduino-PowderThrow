#ifndef PTLed_h
#define PTLed_h

#include <Adafruit_MCP23X17.h>

#define FLASH_RATE 250  //milliseconds  

class PTLed
{
  public:
    //Constructors
    PTLed();

    //functions
    void init(Adafruit_MCP23X17 mcp, int _pin);
    void setOn();
    void setOff();
    void setFlash(int rate=FLASH_RATE);
    void update();
    
  private:
    //vars
    Adafruit_MCP23X17 _mcp;
    bool _initialized;
    int _pin;
    bool _state;
    bool _flashing;
    int _flash_rate;
    unsigned long _last_flash;
    bool _needs_update;
    
    //functions
};

#endif //PTLed_h
