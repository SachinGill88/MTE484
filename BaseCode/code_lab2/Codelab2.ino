#include <Arduino.h>
#include "geeWhiz.h"

double previousError = 0.0;
double previousControlVoltage = 0.0;

void setup() {
  analogReadResolution(14);
  pinMode(A5, OUTPUT);
  Serial.begin(115200);
  delay(300);
  geeWhizBegin();
  setMotorVoltage(0.0f);
  delay(1000);
  Serial.println("geeWhiz Started");
  set_control_interval_ms(5);
}

void loop() {}

void interval_control_code(void) {
  int motor = analogRead(A0);
  double theta = -0.000362771 * motor + 1.398480;
  double theta_ref = ((millis() / 1000UL) % 2 == 0) ? -0.7 : 0.7;
  double theta_ref_sat = constrain(theta_ref, -0.7, 0.7);
  double error = theta_ref_sat - theta;

  double controlVoltage = 0.692327639311257 * previousControlVoltage
                        + 0.032635065920414 * error
                        - 1.462208491947337 * previousError;
  previousError = error;
  previousControlVoltage = controlVoltage;

  double motorVoltage = controlVoltage;
  if (controlVoltage > 0.0) motorVoltage += 0.34;
  else if (controlVoltage < 0.0) motorVoltage -= 0.37;
  setMotorVoltage(motorVoltage);

  Serial.print(millis());
  Serial.print(",");
  Serial.print(theta_ref, 4);
  Serial.print(",");
  Serial.print(theta_ref_sat, 4);
  Serial.print(",");
  Serial.print(theta, 4);
  Serial.print(",");
  Serial.print(controlVoltage);
  Serial.print(",");
  Serial.println(motorVoltage, 2);
  digitalWrite(A5, LOW);
}
