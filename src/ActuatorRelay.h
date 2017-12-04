#pragma once

#include "Actuator.h"

class ActuatorRelay : public Actuator
{
public:
  ActuatorRelay(const JsonObject& config);
  void activate(JsonObject& config);
};
