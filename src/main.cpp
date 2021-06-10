#include <Arduino.h>
#include <IRremote.h>
#include <ir_Toshiba.h>

// Pin Configuration
int PIN_IR_LED = 13;
int PIN_MOTION_SENSOR = 2;
int PIN_BUTTON_ON = 7;
int PIN_BUTTON_OFF = 8;

// Last Timestamps
double TIMER_LAST_MOVEMENT_DETECTION = 0;
double TIMER_LAST_CHECK = 0; // How long since the last AC status check

// Time Configuration
int TIME_STARTUP_MILS = -1;     // The time after the startup routine when the application first starts
int TIME_AC_SHUT_OFF = 60 * 60;      // Time in seconds since the last detection we should wait before shutting off the AC
int TIME_CHECK_AC_RATE = 5;     // Time in seconds we should check the state of the AC
int TIME_REFRESH_AC_STATE = 60; // Time in seconds that we should wait before refreshing the AC state [Anti Han Mechanism]
int TIME_WARUP_TIME = 1;        // Warmup time in seconds for motion sensor..

// AC configurations
int AC_TEMPERATURE = 17; // What the temperature we want is
int AC_STATE = 0;        // What the last state is set to
int AC_TARGET_STATE = 0; // What state we are aiming for.

IRsend irsend;

void logSerial(const String &s)
{
  Serial.println(s);
}

double secondsSinceBoardTurnOn()
{
  return (millis() - TIME_STARTUP_MILS) / 1000;
}

double secondsSinceLastCheck()
{
  double timeSince = secondsSinceBoardTurnOn() - TIMER_LAST_CHECK;
  return timeSince;
}

double secondSinceLastMotion()
{
  double timeSince = secondsSinceBoardTurnOn() - TIMER_LAST_MOVEMENT_DETECTION;
  return timeSince;
}

void acTurnOn()
{
  logSerial("Turning On AC");

  IRsend::ir_toshiba_cmd_s cmd;
  cmd.values.bytes[0] = 0;
  cmd.values.bytes[1] = 0;
  cmd.values.bytes[2] = 0;

  cmd.type = IRsend::HEAT_PUMP_CMD;
  cmd.values.field.fan = TOSHIBA_FAN_5;
  cmd.values.field.mode = TOSHIBA_MODE_COOL;
  cmd.values.field.temp = AC_TEMPERATURE - TOSHIBA_HEAT_BASE;

  if (cmd.values.field.temp < 0 || cmd.values.field.temp > 13)
  {
    Serial.println("Ac Temperature Is Out Of Range");
    return;
  }

  irsend.sendTOSHIBA(cmd);
  AC_STATE = 1;

  logSerial("Turned  On");
}

void acHighPower()
{
  // This isnt working right? IDK...
  AC_STATE = 2;
  return;

  logSerial("Turning On High Power");
  IRsend::ir_toshiba_cmd_s cmd;
  cmd.values.bytes[0] = 0;
  cmd.values.bytes[1] = 0;
  cmd.values.bytes[2] = 0;

  cmd.type = IRsend::HI_PWR_CMD;
  irsend.sendTOSHIBA(cmd);
  AC_STATE = 2;

  logSerial("High Power On");
}

void acOff()
{
  logSerial("Turning off AC");

  IRsend::ir_toshiba_cmd_s cmd;
  cmd.values.bytes[0] = 0;
  cmd.values.bytes[1] = 0;
  cmd.values.bytes[2] = 0;

  cmd.type = IRsend::HEAT_PUMP_CMD;
  cmd.values.field.mode = TOSHIBA_MODE_OFF;

  irsend.sendTOSHIBA(cmd);
  AC_STATE = 0;

  logSerial("AC Off");
}


void setup()
{
  // Setup Pins
  pinMode(PIN_MOTION_SENSOR, INPUT);
  pinMode(PIN_BUTTON_ON, INPUT);
  pinMode(PIN_BUTTON_OFF, INPUT);

  Serial.begin(9600);

  // Once the application has finished initializing, take a snapshot of the milliseconds
  TIME_STARTUP_MILS = millis();
}

void loop()
{
  int buttonOn = digitalRead(PIN_BUTTON_ON);
  int buttonOff = digitalRead(PIN_BUTTON_OFF);

  double dblSecondsSinceBoardTurnedOn = secondsSinceBoardTurnOn();

  // Check the hard buttons first
  if (buttonOn) {
    logSerial("BUTTON ON");
    AC_TARGET_STATE = 3;
    TIMER_LAST_MOVEMENT_DETECTION = secondsSinceBoardTurnOn();
    return;
  }

  if (buttonOff) {
    logSerial("BUTTON OFF");
    AC_TARGET_STATE = 0;
    return;
  }


  if (dblSecondsSinceBoardTurnedOn < TIME_WARUP_TIME)
  {
    logSerial("Warming up...");
    return;
  }

  int motionWasDetected = digitalRead(PIN_MOTION_SENSOR);

  if (motionWasDetected)
  {
    TIMER_LAST_MOVEMENT_DETECTION = secondsSinceBoardTurnOn();
    AC_TARGET_STATE = 2;
  }

  double dblSecondsSInceLastMotion = secondSinceLastMotion();

  if (AC_STATE > 0 && dblSecondsSInceLastMotion >= TIME_AC_SHUT_OFF)
  {
    AC_TARGET_STATE = 0;
  }

  double dblSecondsSinceLastCheck = secondsSinceLastCheck();

  if (dblSecondsSinceLastCheck >= TIME_CHECK_AC_RATE)
  {
    TIMER_LAST_CHECK = millis() / 1000;

    logSerial("Checking AC State...");

    Serial.println("State:");
    Serial.println("sOn: " + String(dblSecondsSinceBoardTurnedOn));
    Serial.println("sLM: " + String(dblSecondsSInceLastMotion));
    Serial.println("sLC: " + String(dblSecondsSinceLastCheck));
    Serial.println("ACS: " + String(AC_STATE));
    Serial.println("TACS: " + String(AC_TARGET_STATE));
    Serial.println("TLC: " + String(TIMER_LAST_CHECK));
    Serial.println("--");


    // todo: check anti-Han mechanism;
    if (AC_STATE != AC_TARGET_STATE)
    {
      if (AC_TARGET_STATE > 0 && AC_STATE < 1)
      {
        acTurnOn();
      }
      else if (AC_TARGET_STATE > 1 && AC_STATE < 2)
      {
        acHighPower();
      }
      else if (AC_TARGET_STATE == 0 && AC_STATE != 0)
      {
        acOff();
      }
    }

    logSerial("---------------------------");
    
  }
}
