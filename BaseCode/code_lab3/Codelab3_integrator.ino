#include <Arduino.h>
#include "geeWhiz.h"

double previousError = 0.0;
double previousControlVoltage = 0.0;
double previousBallError = 0.0; // *
double previousThetaRef = 0.0; // *
double integralThetaRef = 0.0; // *
const double OUTER_KI = 0.1; // *
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
  // double theta_ref = TEST_ANGLE; // *
  double ball_ref = ((millis() / 15000UL) % 2 == 0) ? 0.10 : 0.25; // *
  double ballError = ball_ref - ballPosition; // *
  // double theta_ref = 0.987577639751553 * previousThetaRef
  //                  - 6.962608695652174 * ballError
  //                  + 6.950434782608695 * previousBallError;
  double leadThetaRef = 0.987577639751553 * previousThetaRef // *
                   - 6.962608695652174 * ballError // *
                   + 6.950434782608695 * previousBallError; // *
  double integralChange = -OUTER_KI * 0.005 * 0.5 * (ballError + previousBallError); // *
  double candidateThetaRef = leadThetaRef + integralThetaRef + integralChange; // *
  if (theta >= -0.8 && theta <= 0.8 && // *
      !(candidateThetaRef > 0.7 && integralChange > 0.0) && // *
      !(candidateThetaRef < -0.7 && integralChange < 0.0)) { // *
    integralThetaRef += integralChange; // *
  } // *
  double theta_ref = leadThetaRef + integralThetaRef; // *
  previousBallError = ballError; // *
  previousThetaRef = leadThetaRef; // *
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
  if (controlVoltage > 0.0) motorVoltage -= 0.00;
  else if (controlVoltage < 0.0) motorVoltage -= 0.522;

  // const double DIR = -1.0;
  // double motorVoltage = DIR * 0.002 * (count / 100);
  // count++;

  // if (motorVoltage > 2.0 || motorVoltage < -2.0) motorVoltage = 0.0;

  if (theta > 0.8 || theta < -0.8) motorVoltage = 0.0; // *

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
  // Serial.println(beamAngle, 6); // *
  Serial.print(beamAngle, 6); // *
  Serial.print(","); // *
  Serial.println(ball_ref, 4); // *
  digitalWrite(A5, LOW);
}
