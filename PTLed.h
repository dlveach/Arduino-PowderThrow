#ifndef PTLed_h
#define PTLed_h

#include <Adafruit_MCP23X17.h>

#define FLASH_PERIOD 250  

class PTLed
{
  public:
    //Constructors
    PTLed();

    //functions
    void init(Adafruit_MCP23X17 mcp, int _pin);
    void setOn();
    void setOff();
    void setFlash();
    void update();
    
  private:
    //vars
    Adafruit_MCP23X17 _mcp;
    bool _initialized;
    int _pin;
    bool _state;
    bool _flashing;
    unsigned long _last_flash;
    bool _needs_update;
    
    //functions
};

#endif //PTLed_h
