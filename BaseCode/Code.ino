#include <Arduino.h>
#include "geeWhiz.h"

// ================== Pins ==================
int MOT_PIN = A0;   // motor angle sensor
int BAL_PIN = A1;   // ball position sensor
int count = 0;

// ================== Setup ==================
void setup() {

  analogReadResolution(14);
  pinMode(A5, OUTPUT);   // A5 can be used to measure cycle time using an oscilloscope by connecting the scope to the Arduino Box Motor Leads
  Serial.begin(115200);
  delay(300);

  geeWhizBegin();                 
  set_control_interval_ms(100); // 100 ms loop
  setMotorVoltage(0.0f);

  Serial.println("geeWhiz Started");
}

// ================== Loop ==================
void loop() {
  
}

// ================== Control ISR ==================
void interval_control_code(void) {
  // ---- For Serial Plotter Scaling
  int maxy = 17000;
  int miny = 0;
  // ---- Read sensors ----
  int motor = analogRead(MOT_PIN);
  int ball  = analogRead(BAL_PIN);

  // Exercise B Code
  // if (count < 250) {
  //     setMotorVoltage(0.0f);
  // }
  // else if (count < 350) {
  //     setMotorVoltage(2.0f);
  // }
  // else if (count < 500) {
  //     setMotorVoltage(0.0f);
  // }
  // else {
  //     setMotorVoltage(-2.0f);
  // }

  // count = count + 1;  

  //Exercise C
  float theta = -0.000362771f * motor + 1.398480f;

  // Exercise D Code
  // Voltage ramp: +0.02 V every 5 ticks (0.5 s). Watch the Serial Monitor and
  // note the voltage at which theta starts to change. Set DIR to -1 and
  // re-upload to sweep the other direction.
  const float DIR = -1.0f;                       // +1 or -1
  float MotorVoltage = DIR * 0.002f * (count / 5);
  if (MotorVoltage > 2.0f || MotorVoltage < -2.0f) MotorVoltage = 0.0f;   // stop the ramp at 4 V
  setMotorVoltage(MotorVoltage);

  count = count + 1;

  digitalWrite(A5,HIGH);   // A5 can be used to measure cycle time using an oscilloscope by connecting the scope to the Arduino Box Motor Leads
  Serial.print(maxy);
    Serial.print(",");
  Serial.print(miny);
    Serial.print(",");
  Serial.print(ball);
  Serial.print(",");
  Serial.print(motor);
  Serial.print(",");
  Serial.print(theta, 4);
  Serial.print(",");
  Serial.println(MotorVoltage, 2);
  digitalWrite(A5,LOW);   // A5 can be used to measure cycle time using an oscilloscope by connecting the scope to the Arduino Box Motor Leads
 
}