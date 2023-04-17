// Button.cpp
#include "Button.h"

Button::Button(int pin) {
  buttonPin = pin;
  buttonState = LOW;
  lastButtonState = LOW;
  pinMode(buttonPin, INPUT_PULLUP);
}

int Button::getState() {
  buttonState = digitalRead(buttonPin);
  return buttonState;
}

bool Button::wasPressed() {
  int currentState = getState();
  if (currentState == LOW && lastButtonState == HIGH) {
    lastButtonState = LOW;
    return true;
  } else {
    lastButtonState = currentState;
    return false;
  }
}

