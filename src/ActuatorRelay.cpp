#include "ActuatorRelay.h"

#include "log.h"
#include "Arduino.h"

ActuatorRelay::ActuatorRelay(const JsonObject& config){
  pin = config["pin"];
  pinMode(pin, OUTPUT);

  digitalWrite(pin, (config["default"] == 1) ? HIGH : LOW);
}

void ActuatorRelay::activate(JsonObject& config){
  logger.print("Relay Activating; pin: "); logger.println(pin);

  digitalWrite(pin, (config["state"] == 1) ? HIGH : LOW);
}
