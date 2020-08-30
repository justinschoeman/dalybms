// NB - USE MINICORE TO BURN UP TO DATE BOOTLOADER OR WATCHDOG WILL RESULT IN A BOOT LOOP

#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>
#include <avr/wdt.h>

// BATTERY CONFIGURATION

// cell parameters
// cell count
#define CELL_CNT 16
// maximum cell voltage (mV)
#define CELL_MAX_V 3650U
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
#define LV_LOCKOUT_MS (300UL*1000UL)
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

// output/control parameters (SET IN DERATE.H)
// these are updated by ramp functions, so start with safe values, which
// will ramp up to target values

// charge voltage, mV
uint16_t bat_chg_v = BAT_SAFE_V;
// charge current 0.01A
uint16_t bat_chg_i = 0;// start at 0 charge and ramp up... //BAT_CHG_I * 100U;
// discharge current 0.01A
uint16_t bat_dis_i = BAT_DIS_I * 100U;
// discharge voltage mV
uint16_t bat_dis_v = BAT_DIS_V;
// state of charge, % (*shared measurement parameter)
uint16_t bat_soc;
// state of health, % (no source - just fake)
uint16_t bat_soh = 100;

uint16_t bat_maxv; // max cell voltage
uint16_t bat_minv; // min cell voltage
uint8_t bat_maxc; // cell number (1 at negative pole) with max voltage
uint8_t bat_minc; // cell number with min voltage
uint16_t bat_stat; // balancer status bit0 = cell 1 '1' == balancing

// shared message buffer
// can-bms is 8 bytes
// daly rs485 is 13 bytes

uint8_t mbuf[13];

#include "bms.h"
#include "can.h"
#include "derate.h"

unsigned long can_rpt_ms;

void setup() {
  // start watchdog
  wdt_enable(WDTO_8S);

  // set up serial port
  Serial.begin(9600);

  // set up display
  Serial.println("Init display");
  Wire.begin();  
  Wire.setClock(400000);
  // Initialze SSD1306 OLED display
  oled.init();
  oled.clearDisplay();

  // set up bms pins
  Serial.println("Init BMS");
  bms_setup();

  // set up can bus
  Serial.println("Init CAN");
  can_setup();

  // schedule first run now...
  can_rpt_ms = millis() - CAN_RPT_MS;
}

// need to put proper column counting into library...
void oled_eol(void) {
  oled.putString("                ");
}

void loop() {
  // wait for next poll interval
  if(millis() - can_rpt_ms < CAN_RPT_MS) {
    return;
  }
  can_rpt_ms = millis();

  // read bms
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
    display_update();
    // bump watchdog (only if EVERYTHING was successfull)
    wdt_reset();
  }
}

void display_update(void) {
  int i;
  int l = 0;

  // current measurements
  oled.setTextXY(l++,0);
  oled.putFloat(bat_v/100.0, 2);
  oled.putString("V ");
  oled.putFloat((int16_t)(bat_i)/10.0, 2);
  oled.putString("A");
  oled_eol();

  // min cell
  oled.setTextXY(l++,0);
  oled.putString("Min: ");
  oled.putNumber(bat_minv);
  oled.putString(" (");
  oled.putNumber(bat_minc);
  oled.putString(")");
  oled_eol();

  // max cell
  oled.setTextXY(l++,0);
  oled.putString("Max: ");
  oled.putNumber(bat_maxv);
  oled.putString(" (");
  oled.putNumber(bat_maxc);
  oled.putString(")");
  oled_eol();

  // cell delta
  oled.setTextXY(l++,0);
  oled.putString("Diff: ");
  oled.putNumber(bat_maxv - bat_minv);
  oled_eol();

  // target charge voltage
  oled.setTextXY(l++,0);
  oled.putString("CV: ");
  oled.putFloat(bat_chg_v/1000.0, 2);
  oled.putString("/");
  oled.putFloat(BAT_CHG_V/1000.0, 2);
  oled_eol();

  // target charge current
  oled.setTextXY(l++,0);
  oled.putString("CI: ");
  oled.putFloat(bat_chg_i/100.0, 1);
  oled.putString("/");
  oled.putNumber(BAT_CHG_I);
  oled_eol();

  // target discharge current
  oled.setTextXY(l++,0);
  oled.putString("DI: ");
  oled.putFloat(bat_dis_i/100.0, 1);
  oled.putString("/");
  oled.putNumber(BAT_DIS_I);
  oled_eol();

  // balancer status
  oled.setTextXY(l++,0);
  for(i = 0; i < 16; i++) {
    if(bat_stat & 1) {
      oled.putChar('.');
    } else {
      if(i < 9) {
        oled.putNumber(i + 1);
      } else {
        oled.putNumber(i - 9);
      }
    }
    bat_stat >>= 1;
  }
}
