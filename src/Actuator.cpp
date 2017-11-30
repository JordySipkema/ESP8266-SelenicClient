#include "Actuator.h"
#include <Arduino.h>

#include "ActuatorRelay.h"
#include "Log.h"


Actuator* Actuator::build(JsonObject& info)
{
  int type = info["type"];
  Actuator* a = nullptr;

  switch(type)
  {
    case ACT_RELAY_TYPE:
      a = new ActuatorRelay((JsonObject&) info["config"]);
      break;
  }

  if(a)
    a->id = info["id"];

  return a;
}
