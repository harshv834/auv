// Copyright 2016 AUV-IITK
#include <ros.h>
#include <Arduino.h>
#include <std_msgs/Int32.h>
#include <math.h>

#define pwmPinWest 4
#define pwmPinEast 5
#define directionPinWest1 26
#define directionPinWest2 27
#define directionPinEast1 35
#define directionPinEast2 29

#define pwmPinNorthSway 2
#define pwmPinSouthSway 3
#define directionPinNorthSway1 31
#define directionPinNorthSway2 30
#define directionPinSouthSway1 33
#define directionPinSouthSway2 32

#define pwmPinNorthUp 6
#define pwmPinSouthUp 7
#define directionPinNorthUp1 24
#define directionPinNorthUp2 25
#define directionPinSouthUp1 22
#define directionPinSouthUp2 23

#define analogPinPressureSensor A0

const float c092 = 506.22;
const float s092 = -2.65;
const float c093 = 448.62;
const float s093 = -2.92;
const float c099 = 397.65;  // reference as their graph is at lowest
const float s099 = -2.71;   // reference as their graph is at lowest
const float c113 = 539.85;
const float s113 = -3.38;
const float c117 = 441.32;
const float s117 = -3.03;
const float c122 = 547.39;
const float s122 = -2.93;

int Delay = 1500;
bool isMovingForward = true;
ros::NodeHandle nh;

int btd092(int pwm)
{
  pwm = (c099 + s099 * pwm - c092) / (s092);
  return pwm;
}

int btd093(int pwm)
{
  pwm = (c099 + s099 * pwm - c093) / (s093);
  return pwm;
}

int btd099(int pwm)
{
  return pwm;
}

int btd113(int pwm)
{
  pwm = (c099 + s099 * pwm - c113) / (s113);
  return pwm;
}

int btd117(int pwm)
{
  pwm = (c099 + s099 * pwm - c117) / (s117);
  return pwm;
}

int btd122(int pwm)
{
  pwm = (c099 + s099 * pwm - c122) / (s122);
  return pwm;
}

void thrusterNorthUp(int pwm, int isUpward)
{
  pwm = abs(pwm);
  pwm = btd117(pwm);
  analogWrite(pwmPinNorthUp, 255 - pwm);
  if (isUpward)
  {
    digitalWrite(directionPinNorthUp1, HIGH);
    digitalWrite(directionPinNorthUp2, LOW);
  }
  else
  {
    digitalWrite(directionPinNorthUp1, LOW);
    digitalWrite(directionPinNorthUp2, HIGH);
  }
}

void thrusterSouthUp(int pwm, int isUpward)
{
  pwm = abs(pwm);
  pwm = btd093(pwm);
  analogWrite(pwmPinSouthUp, 255 - pwm);
  if (isUpward)
  {
    digitalWrite(directionPinSouthUp1, HIGH);
    digitalWrite(directionPinSouthUp2, LOW);
  }
  else
  {
    digitalWrite(directionPinSouthUp1, LOW);
    digitalWrite(directionPinSouthUp2, HIGH);
  }
}

void thrusterNorthSway(int pwm, int isRight)
{
  pwm = abs(pwm);
  pwm = btd113(pwm);
  analogWrite(pwmPinNorthSway, 255 - pwm);
  if (isRight)
  {
    digitalWrite(directionPinNorthSway1, HIGH);
    digitalWrite(directionPinNorthSway2, LOW);
  }
  else
  {
    digitalWrite(directionPinNorthSway1, LOW);
    digitalWrite(directionPinNorthSway2, HIGH);
  }
}

void thrusterSouthSway(int pwm, int isRight)
{
  pwm = abs(pwm);
  pwm = btd122(pwm);
  analogWrite(pwmPinSouthSway, 255 - pwm);
  if (isRight)
  {
    digitalWrite(directionPinSouthSway1, HIGH);
    digitalWrite(directionPinSouthSway2, LOW);
  }
  else
  {
    digitalWrite(directionPinSouthSway1, LOW);
    digitalWrite(directionPinSouthSway2, HIGH);
  }
}

void thrusterEast(int pwm, int isForward)
{
  pwm = abs(pwm);
  pwm = btd092(pwm);
  analogWrite(pwmPinEast, 255 - pwm);
  if (isForward)
  {
    digitalWrite(directionPinEast1, HIGH);
    digitalWrite(directionPinEast2, LOW);
  }
  else
  {
    digitalWrite(directionPinEast1, LOW);
    digitalWrite(directionPinEast2, HIGH);
  }
}

void thrusterWest(int pwm, int isForward)
{
  pwm = abs(pwm);
  pwm = btd099(pwm);
  analogWrite(pwmPinWest, 255 - pwm);
  if (isForward)
  {
    digitalWrite(directionPinWest1, HIGH);
    digitalWrite(directionPinWest2, LOW);
  }
  else
  {
    digitalWrite(directionPinWest1, LOW);
    digitalWrite(directionPinWest2, HIGH);
  }
}

void PWMCbForward(const std_msgs::Int32 &msg)
{
  if (msg.data > 0)
  {
    thrusterEast(msg.data, true);
    thrusterWest(msg.data, true);
  }
  else
  {
    thrusterEast(msg.data, false);
    thrusterWest(msg.data, false);
  }
  isMovingForward = true;
}

void PWMCbSideward(const std_msgs::Int32 &msg)
{
  if (msg.data > 0)
  {
    thrusterNorthSway(msg.data, true);
    thrusterSouthSway(msg.data, true);
  }
  else
  {
    thrusterNorthSway(msg.data, false);
    thrusterSouthSway(msg.data, false);
  }
  isMovingForward = false;
}

void PWMCbUpward(const std_msgs::Int32 &msg)
{
  if (msg.data > 0)
  {
    thrusterNorthUp(msg.data, true);
    thrusterSouthUp(msg.data, true);
  }
  else
  {
    thrusterNorthUp(msg.data, false);
    thrusterSouthUp(msg.data, false);
  }
}

void PWMCbTurnSway(const std_msgs::Int32 &msg)
{
  if (msg.data > 0)
  {
    thrusterEast(msg.data, true);
    thrusterWest(msg.data, false);
  }
  else
  {
    thrusterEast(msg.data, false);
    thrusterWest(msg.data, true);
  }
}

void PWMCbTurn(const std_msgs::Int32 &msg)
{
  if (msg.data > 0)
  {
    thrusterNorthSway(msg.data, false);
    thrusterSouthSway(msg.data, true);
  }
  else
  {
    thrusterNorthSway(msg.data, true);
    thrusterSouthSway(msg.data, false);
  }
}

ros::Subscriber<std_msgs::Int32> subPwmForward("/pwm/forward", &PWMCbForward);
ros::Subscriber<std_msgs::Int32> subPwmSideward("/pwm/sideward", &PWMCbSideward);
ros::Subscriber<std_msgs::Int32> subPwmUpward("/pwm/upward", &PWMCbUpward);
ros::Subscriber<std_msgs::Int32> subPwmTurnSway("/pwm/turnsway", &PWMCbTurnSway);
ros::Subscriber<std_msgs::Int32> subPwmTurn("/pwm/turn", &PWMCbTurn);

void setup()
{
  nh.initNode();

  pinMode(directionPinEast1, OUTPUT);
  pinMode(directionPinEast2, OUTPUT);
  pinMode(pwmPinWest, OUTPUT);
  pinMode(directionPinWest2, OUTPUT);
  pinMode(pwmPinEast, OUTPUT);
  pinMode(directionPinWest1, OUTPUT);

  pinMode(directionPinSouthSway1, OUTPUT);
  pinMode(directionPinSouthSway2, OUTPUT);
  pinMode(pwmPinNorthSway, OUTPUT);
  pinMode(directionPinNorthSway2, OUTPUT);
  pinMode(pwmPinSouthSway, OUTPUT);
  pinMode(directionPinNorthSway1, OUTPUT);

  pinMode(directionPinSouthUp1, OUTPUT);
  pinMode(directionPinSouthUp2, OUTPUT);
  pinMode(pwmPinNorthUp, OUTPUT);
  pinMode(directionPinNorthUp2, OUTPUT);
  pinMode(pwmPinSouthUp, OUTPUT);
  pinMode(directionPinNorthUp1, OUTPUT);

  nh.subscribe(subPwmForward);
  nh.subscribe(subPwmSideward);
  nh.subscribe(subPwmUpward);
  nh.subscribe(subPwmTurn);
  nh.subscribe(subPwmTurnSway);
  Serial.begin(57600);
}

void loop()
{
  nh.spinOnce();
  delay(1);
}
