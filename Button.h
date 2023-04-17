#ifndef __BUTTON__H
#define __BUTTON__H

#include <Arduino.h>
#include <TelnetStream.h>


class Button {
private:
  int buttonPin;
  int buttonState;
  int lastButtonState;
  int debounceDelay;
  int lastDebounceTime;
public:
  Button(int pin);
  int getState();
  bool wasPressed();
};



#endif