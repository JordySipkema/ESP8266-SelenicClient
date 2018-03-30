#pragma once

#include "Actuator.h"

class ActuatorLed : public Actuator
{
public:
  ActuatorLed(const JsonObject& config);
  void activate(JsonObject& config);
};
