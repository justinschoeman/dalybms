#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>

// BATTERY CONFIGURATION

// cell parameters
// cell count
#define CELL_CNT 16
// maximum cell voltage (mV)
#define CELL_MAX_V 3600U
// minimum cell voltage (mV)
#define CELL_MIN_V 2500U
// safe default charge voltage (mV)
#define CELL_SAFE_V 3400U
// maximum cell voltage before we derate
#define CELL_DERATE_V (CELL_MAX_V - 100U)

// default max charge voltage (mV)
#define BAT_CHG_V (CELL_CNT * CELL_MAX_V)
// safe start-up limit
#define BAT_SAFE_V (CELL_CNT * CELL_SAFE_V)
// max charge current (A)
#define BAT_CHG_I 150
// max discharge current (A)
#define BAT_DIS_I 360
// min discharge voltage (mV)
#define BAT_DIS_V (CELL_CNT * CELL_MIN_V)
// low voltage lockout in ms
#define LV_LOCKOUT_MS (30UL*1000UL)
// can report interval in ms
#define CAN_RPT_MS 1000UL

// shared battery state - write here in pylon/can units
// measured parameters
//voltage, 10mV
uint16_t bat_v = 0; //uninit flag
// current, 0.1A
uint16_t bat_i;
// bat temp, 0.1C
uint16_t bat_t;

// output/control parameters
// charge voltage, mV
uint16_t bat_chg_v = BAT_SAFE_V;
// charge current 0.01A
uint16_t bat_chg_i = 0;// start at 0 charge and ramp up... BAT_CHG_I * 100U;
// discharge current 0.01A
uint16_t bat_dis_i = BAT_DIS_I * 100U;
// discharge voltage mV
uint16_t bat_dis_v = BAT_DIS_V;
// state of charge, %
uint16_t bat_soc;
// state of health, %
uint16_t bat_soh = 100;

uint16_t bat_maxv;
uint16_t bat_minv;
uint8_t bat_maxc;
uint8_t bat_minc;
uint16_t bat_stat;

// shared message buffer
// can-bms is 8 bytes
// daly rs485 is 13 bytes

uint8_t mbuf[13];

#include "bms.h"
#include "can.h"
#include "derate.h"

unsigned long can_rpt_ms;

void setup() {
  // set up serial port
  Serial.begin(9600);
  // set up display
  Wire.begin();  
  Wire.setClock(400000);
  oled.init();
  oled.clearDisplay();
  // Initialze SSD1306 OLED display
  // set up bms pins
  bms_setup();
  // set up can bus
  can_setup();
  // schedule first run now...
  can_rpt_ms = millis() - CAN_RPT_MS;
}

void oled_eol(void) {
  oled.putString("                ");
}

void loop() {
  int i;
  int l = 0;
  if(millis() - can_rpt_ms < CAN_RPT_MS) {
    return;
  }
  can_rpt_ms = millis();
  Serial.println("POLL");
  if(!bms_update()) {
    Serial.println("BMS update failed."); 
    oled.clearDisplay();
    oled.setTextXY(0,0);
    oled.putString("BMS READ FAILED");
 } else {
    // derate charge/discharge
    derate();
    // transmit can data
    can_update();
    // update display
    oled.setTextXY(l++,0);
    oled.putFloat(bat_v/100.0, 2);
    oled.putString("V ");
    oled.putFloat((int16_t)(bat_i)/10.0, 2);
    oled.putString("A");
    oled_eol();
    oled.setTextXY(l++,0);
    oled.putString("Min: ");
    oled.putNumber(bat_minv);
    oled.putString(" (");
    oled.putNumber(bat_minc);
    oled.putString(")");
    oled_eol();
    oled.setTextXY(l++,0);
    oled.putString("Max: ");
    oled.putNumber(bat_maxv);
    oled.putString(" (");
    oled.putNumber(bat_maxc);
    oled.putString(")");
    oled_eol();
    oled.setTextXY(l++,0);
    oled.putString("Diff: ");
    oled.putNumber(bat_maxv - bat_minv);
    oled_eol();
    oled.setTextXY(l++,0);
    oled.putString("CV: ");
    oled.putFloat(bat_chg_v/1000.0, 2);
    oled.putString("/");
    oled.putFloat(BAT_CHG_V/1000.0, 2);
    oled_eol();
    oled.setTextXY(l++,0);
    oled.putString("CI: ");
    oled.putFloat(bat_chg_i/100.0, 1);
    oled.putString("/");
    oled.putNumber(BAT_CHG_I);
    oled_eol();
    oled.setTextXY(l++,0);
    oled.putString("DI: ");
    oled.putFloat(bat_dis_i/100.0, 1);
    oled.putString("/");
    oled.putNumber(BAT_DIS_I);
    oled_eol();
    oled.setTextXY(l++,0);
    for(i = 0; i < 16; i++) {
      if(bat_stat & 1) oled.putChar('*'); else oled.putChar('.');
      bat_stat >>= 1;
      //if(i == 7) oled.setTextXY(l++,0);
    }
  }
}
