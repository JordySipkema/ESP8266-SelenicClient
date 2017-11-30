#include "ActuatorRelay.h"

#include "log.h"
#include "Arduino.h"

ActuatorRelay::ActuatorRelay(const JsonObject& config){
  pin = config["pin"];
  pinMode(pin, OUTPUT);
}

void ActuatorRelay::activate(JsonObject& o){
  logger.print("Relay Activating; pin: "); logger.println(pin);
  if(o["state"] == 1)
    digitalWrite(pin, HIGH);
  else
    digitalWrite(pin, LOW);
}
