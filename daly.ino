// BATTERY CONFIGURATION
// default max charge voltage (mV)
#define BAT_CHG_V 58400
// max charge current (A)
#define BAT_CHG_I 150
// max discharge current (A)
#define BAT_DIS_I 360
// maximum cell voltage (mV)
#define BAT_MAX_V 3450
// minimum cell voltage (mV)
#define BAT_MIN_V 3200
// maximum delta before we derate (mV)
#define BAT_MAX_DELTA 50
// derate window before max v (mV)
#define BAT_WIN 100

// shared battery state - write here in pylon/can units
//voltage, 10mV
uint16_t bat_v = 0; //uninit flag
// current, 0.1A
uint16_t bat_i;
// bat temp, 0.1C
uint16_t bat_t;
// charge voltage, 0.1V
uint16_t bat_chg_v = BAT_CHG_V / 100;
// charge current 0.1A
uint16_t bat_chg_i = BAT_CHG_I * 10;
// discharge current 0.1A
uint16_t bat_dis_i = BAT_DIS_I * 10;
// state of charge, %
uint16_t bat_soc;
// state of health, %
uint16_t bat_soh = 100;

// shared message buffer
// can-bms is 8 bytes
// daly rs485 is 13 bytes

uint8_t mbuf[13];

#include "bms.h"
#include "can.h"

void setup() {
  // set up serial port
  Serial.begin(9600);
  // set up bms pins
  bms_setup();
  // set up can bus
  can_setup();
}

void loop() {
  Serial.println("foo");
  if(!bms_update()) {
    Serial.println("BMS update failed."); 
  } else {
    // transmit can data
    can_update();
  }
  delay(1000);
}
