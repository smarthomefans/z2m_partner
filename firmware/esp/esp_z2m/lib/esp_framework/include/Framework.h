
// Framework.h

#ifndef _FRAMEWORK_h
#define _FRAMEWORK_h

#include "Arduino.h"

class Framework
{
    static void callback(char *topic, byte *payload, unsigned int length);
    static void connectedCallback();

public:
    Framework() { Framework(115200); };
    Framework(unsigned long baud) { one(baud); };
    void one(unsigned long baud);
    void setup();
    void loop();
};

#endif