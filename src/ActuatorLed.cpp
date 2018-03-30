#include "ActuatorLed.h"

#include "log.h"
#include "Arduino.h"

ActuatorLed::ActuatorLed(const JsonObject& config){
  pin = config["pin"];
  pinMode(pin, OUTPUT);

  digitalWrite(pin, (config["default"] == 1) ? HIGH : LOW);
}

void ActuatorLed::activate(JsonObject& config){
  logger.print("Led Activating; pin: "); logger.println(pin);

  digitalWrite(pin, (config["state"] == 1) ? HIGH : LOW);
}
