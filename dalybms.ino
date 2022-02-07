// NB - USE MINICORE TO BURN UP TO DATE BOOTLOADER OR WATCHDOG WILL RESULT IN A BOOT LOOP

#include <Wire.h>
// https://github.com/acrobotic/Ai_Ardulib_SSD1306
#include <ACROBOTIC_SSD1306.h>
#include <avr/wdt.h>

#define BUZZER_PIN 9

// BATTERY CONFIGURATION
// LBSA recommends the following for their built packs
//  charge: 56V = 3500mV
//  float:  54V = 3375mV
//  min:    48V = 3000mV
// Datasheet gives:
//  charge: 3650mV
//  min:    2500mV
//
// I will keep min as 2500mV, as inverter will cut off at 10% anyway...
// Max I will choose 3600mV - which will generally top out somewhere around 3550 due to proportional control loop.
// Will derate at -200mV though, as time from 3500-> max will be close to 0 at 150A
//
// I will also use CELL_SAFE_V as the float point (54.4V)

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
#define CELL_DERATE_V (CELL_MAX_V - 200U)

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

// float logic parameters
// specified float voltage
#define BAT_FLOAT_V (CELL_CNT * CELL_SAFE_V)
// cell minimum float voltage (minimum cell must be at least this voltage
#define CELL_FLOAT_MIN_V (CELL_SAFE_V + 50U)
// cell maximum float voltage (maximum cell must be at least this voltage
#define CELL_FLOAT_MAX_V (CELL_MAX_V - 50U)
// maximum float current (current must be below this limit - 3% of rated capacity is recommended)
#define BAT_FLOAT_I (BAT_DIS_I / 33)
// minimum time at above conditions before switching to float
// increase to 30 minutes - need some time at the top to balance...
#define BAT_FLOAT_MS (30UL * 60UL * 1000UL)
// min cell voltage for float release (when one cell falls below this, cancel float mode)
#define CELL_FLOAT_END_V 3350

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

// charge/float mode (0 = charge, 1 = float)
uint8_t bat_mode;
// timestamp for last float good parameter
unsigned long bat_mode_ms;

// charge voltage, mV
uint16_t bat_chg_v;
// charge current 0.01A
uint16_t bat_chg_i;
// discharge current 0.01A
uint16_t bat_dis_i;
// discharge voltage mV
uint16_t bat_dis_v;
// state of charge, % (*shared measurement parameter)
uint16_t bat_soc;
// state of health, % (no source - just fake)
uint16_t bat_soh = 100;

uint16_t bat_maxv; // max cell voltage
uint16_t bat_minv; // min cell voltage
uint8_t bat_maxc; // cell number (1 at negative pole) with max voltage
uint8_t bat_minc; // cell number with min voltage
uint16_t bat_stat; // balancer status bit0 = cell 1 '1' == balancing
uint16_t bat_ce_diff; // charge end cell difference in mV

// shared message buffer
// can-bms is 8 bytes
// daly rs485 is 13 bytes

uint8_t mbuf[13];

// shared functions
void oled_error(const char * s) {
  int r = 1;

  Serial.println(s);
  oled.clearDisplay();
  for(;;) {
    oled.setTextXY(r++,0);
    oled.putString(s);
    if(strlen(s) <= 16) break;
    s += 16;
  }
  delay(500UL);
}


#include "bms.h"
#include "can.h"
#include "derate.h"

unsigned long can_rpt_ms;

void setup() {
  // start watchdog
  wdt_enable(WDTO_8S);

  // beep the beeper
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, 1);
  delay(500UL);
  digitalWrite(BUZZER_PIN, 0);

  // keep watchdog ticking
  wdt_reset();

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


  // set up derate module
  Serial.println("Init DERATE");
  derate_setup();


  // set up can bus
  Serial.println("Init CAN");
  can_setup();

  // schedule first run now...
  can_rpt_ms = millis() - CAN_RPT_MS;

  // keep watchdog ticking
  wdt_reset();
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
    oled_error("BMS READ FAILED");
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
  if(bat_mode) oled.putChar('F');
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
  oled.putString("/");
  oled.putNumber(bat_ce_diff);
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
