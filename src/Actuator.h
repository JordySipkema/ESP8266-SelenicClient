#pragma once

#include <ArduinoJson.h>

#define ACT_RELAY_TYPE 5
#define ACT_LED_TYPE 1

class Actuator
{
protected:
  int pin;
public:
  int id;
  static Actuator* build(JsonObject& info);

  virtual void activate(JsonObject& config);
  virtual void update() {};
};
