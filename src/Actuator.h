#pragma once

#include <ArduinoJson.h>

#define ACT_RELAY_TYPE 5

class Actuator
{
protected:
  int pin;
public:
  int id;
  static Actuator* build(JsonObject& o);

  virtual void activate(JsonObject& o);
  virtual void update() {};
};
