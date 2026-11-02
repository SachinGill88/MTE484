#include <Arduino.h>
#include "geeWhiz.h"

double previousError = 0.0;
double previousControlVoltage = 0.0;
int count = 0;
const double TEST_ANGLE = -0.7; // Part F: test 

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
  int ball = analogRead(A1);
  double theta = -0.000362771 * motor + 1.398480;
  // double ballPosition = 0.0000249191088158444 * (ball - 15);
  double ballPosition = 0.0000248138957816377 * ball + 0.00148883374689826;
  double beamAngle = 0.0609 * theta;

  // double theta_ref = ((millis() / 1000UL) % 2 == 0) ? -0.7 : 0.7;
  // double theta_ref = 0.0;
  double theta_ref = TEST_ANGLE;
  double theta_ref_sat = constrain(theta_ref, -0.7, 0.7);
  double error = theta_ref_sat - theta;
  double controlVoltage = 0.692327639311257 * previousControlVoltage
                        + 0.032635065920414 * error
                        - 1.462208491947337 * previousError;
  previousError = error;
  previousControlVoltage = controlVoltage;
  double motorVoltage = controlVoltage;
  // if (controlVoltage > 0.0) motorVoltage += 0.34;
  // else if (controlVoltage < 0.0) motorVoltage -= 0.37;
  if (controlVoltage > 0.0) motorVoltage += 0.0;
  else if (controlVoltage < 0.0) motorVoltage -= 0.522;

  // const double DIR = -1.0;
  // double motorVoltage = DIR * 0.002 * (count / 100);
  // count++;

  // if (motorVoltage > 2.0 || motorVoltage < -2.0) motorVoltage = 0.0;

  if (theta > 0.9 || theta < -0.9) motorVoltage = 0.0;

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
  // Serial.println(motorVoltage, 3);
  Serial.print(motorVoltage, 3);
  Serial.print(",");
  // Serial.println(ball);
  Serial.print(ball);
  Serial.print(",");
  Serial.print(ballPosition, 6);
  Serial.print(",");
  Serial.println(beamAngle, 6);
  digitalWrite(A5, LOW);
}
