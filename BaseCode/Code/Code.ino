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
  set_control_interval_ms(1); // 100 ms loop, changed to a faster sampling time for part E
  setMotorVoltage(0.0f);
  delay(1000);

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
  const float V_STIC_POS = 0.34f;
  const float V_STIC_NEG = 0.37f;
  float Kp = -10; // value is changed throughout 


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
  // const float DIR = -1.0f;                       // +1 or -1
  // float MotorVoltage = DIR * 0.002f * (count / 5);
  // if (MotorVoltage > 2.0f || MotorVoltage < -2.0f) MotorVoltage = 0.0f;   // stop the ramp at 4 V
  // setMotorVoltage(MotorVoltage);

  // count = count + 1;


  //Exercise E Code

  //Implement square wave reference signal 
  float theta_ref; 

  if ((millis()/1000) % 2 ==0){
    theta_ref = -0.1f; 
  }
  else {
    theta_ref = 0.1f;
  }

  float error = theta_ref - theta;

  float controlVoltage = Kp * error;

  // Add stiction compensation 

  float motorVoltage; 

  if (controlVoltage > 0.0f) {
    motorVoltage = controlVoltage + V_STIC_POS;
  }
  else if (controlVoltage < 0.0f){
    motorVoltage = controlVoltage - V_STIC_NEG;
  }
  else {
    motorVoltage = 0.0f;
  }


  setMotorVoltage(motorVoltage);



  

  // digitalWrite(A5,HIGH);   // A5 can be used to measure cycle time using an oscilloscope by connecting the scope to the Arduino Box Motor Leads
  // Serial.print(maxy);
  //   Serial.print(",");
  // Serial.print(miny);
  //   Serial.print(",");
  // Serial.print(ball);
  // Serial.print(",");
  // Serial.print(motor);
  // Serial.print(",");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(theta_ref,4);
  Serial.print(",");
  Serial.print(theta, 4);
  Serial.print(",");
  Serial.print(controlVoltage);
  Serial.print(",");
  Serial.println(motorVoltage, 2);
  digitalWrite(A5,LOW);   // A5 can be used to measure cycle time using an oscilloscope by connecting the scope to the Arduino Box Motor Leads
 
}