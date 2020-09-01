int8_t lv_lockout = 0;
unsigned long lv_lockout_ms;

// target control parameters (inverter gets unhappy if targets change too quickly, so derate module specifies targets and then ramps avtuals towards these targets)
// charge voltage, mV
uint16_t trg_chg_v;
// charge current 0.01A
uint16_t trg_chg_i;
// bad input count
int derate_error_count;

void derate_setup(void) {
  bat_mode = 0; // charge mode
  bat_mode_ms = millis();
  bat_chg_v = BAT_SAFE_V;
  bat_chg_i = 0; // start at 0 charge and ramp up... //BAT_CHG_I * 100U;
  bat_dis_i = BAT_DIS_I * 100U;
  bat_dis_v = BAT_DIS_V;
  derate_error_count = 0;
}

// set the target in a safe way, without overflows
void derate_set_trg_v(uint32_t ofs) {
  uint32_t t32;
  
  t32 = bat_v;
  t32 *= 10U;
  t32 += ofs;
  if(bat_mode == 0) {
    if(t32 > (uint32_t)BAT_CHG_V) t32 = BAT_CHG_V; // clamp to normal max
  } else {
    if(t32 > (uint32_t)BAT_FLOAT_V) t32 = BAT_FLOAT_V; // clamp to float max
  }
  trg_chg_v = t32;
}

int8_t derate_check_input(void) {
  // the bms occasionally spits out really bad data...
  // no idea how bad it can be (range wise), but try to detect it, and if we do, skip for a while
  // if it is still bad force a watchdog reset and try again...
  // not going to quality check bat_maxv - rather stopping charging prematurely than overcharging...
  if(bat_v < 6000 && bat_minv < 4000) {
    // total battery voltage < 60V and minimum cell voltage < 4V
    // assume OKish
    derate_error_count = 0;
    return 0;
  }
  oled_error("*** UNRELIABLE DATA? ***");
  derate_error_count++;
  if(derate_error_count > 5) {
    oled_error("TOO MANY ERRORS - REBOOTING!");
    // hang and let watchdog reboot us
    while(1) {}
  }
  return 1;
}

// we are in charge mode - test if all the minimum requirements are met
// (EXCEPT TIME) for switching to float.
int8_t derate_test_float(void) {
  if((bat_v * 10U) < BAT_FLOAT_V) return 0; // overall voltage too low
  Serial.println("batv OK");
  if(bat_minv < CELL_FLOAT_MIN_V) return 0; // lowest cell below min
  Serial.println("batv min OK");
  if(bat_maxv < CELL_FLOAT_MAX_V) return 0; // highset cell lestt than min
  Serial.println("batv max OK");
  Serial.println(-(int16_t)bat_i);
  Serial.println(BAT_FLOAT_I * 10);
  if(-(int16_t)bat_i > (BAT_FLOAT_I * 10)) return 0; // current too high
  Serial.println("FLOAT OK");
  return 1;
}

void derate(void) {
  uint16_t delta;
  uint32_t t32;

  // check if input is good before processing it
  if(derate_check_input()) return;

  // overall mode control
  if(bat_mode == 0) {
    // charge mode - test if we must switch to float
    if(derate_test_float()) {
       // we meet all the requirements (except possibly time) for float mode
       if((millis() - bat_mode_ms) > BAT_FLOAT_MS) {
         Serial.println("FLOAT MODE NOW");
         bat_mode = 1;
       }
    } else {
      // not ready for float - update timestamp for latest non-float time
      bat_mode_ms = millis();
    }
  } else {
    // float mode - test if we must switch to charge
    if(bat_minv < CELL_FLOAT_END_V) {
      Serial.println("FLOAT END");
      bat_mode = 0;
      bat_mode_ms = millis();
    }
  }

  // always start with some sane defaults
  // for sanity sake, always keep the lowest meaningful charge voltage, and ramp up as required...
  // this  could potentially overflow
  t32 = CELL_MAX_V - bat_maxv;
  t32 *= (uint32_t)CELL_CNT;  // set the target voltage = current voltage + whatever is required to make the max cell full (spread over all cells)
  // inverter is too gentle close to voltage limits - lets make it slightly more aggressive
  t32 *= (uint32_t)2;
  derate_set_trg_v(t32);
  
  // default to normal max current
  trg_chg_i = BAT_CHG_I*100;

  // HIGH VOLTAGE LOGIC
  if(bat_maxv > CELL_MAX_V) {
    // over - shut off charging...
    Serial.println("OVERVOLTAGE!");
    trg_chg_i = 0;
    derate_set_trg_v(0);
    // do not do this - implausible values can cause inverter to ignore us!
    //trg_chg_v -= (bat_maxv - BAT_MAX_V) * 200; // retard by 200mV for each mV above target...
  } else if(bat_maxv > CELL_DERATE_V) {
    Serial.println("HIGH VOLTAGE!");
    // derate voltage - done globally above as the default
    //trg_chg_v = bat_v * 10U;
    //trg_chg_v += 16*(BAT_MAX_V - bat_maxv); // set the target voltage = current voltage + whatever is required to make the max cell full
    //if(trg_chg_v > BAT_CHG_V) trg_chg_v = BAT_CHG_V; // clamp to normal max

    // derate current
    /*delta = bat_maxv - CELL_DERATE_V;
    f = delta;
    f /= (float)(CELL_MAX_V - CELL_DERATE_V);
    // (min derate) 0 < f <= 1 (full derate)
    f = 1.0 - f;
    //f *= f; // make it very rapid at first
    trg_chg_i = (float)(BAT_CHG_I * 100U) * f; */
    // FIXME - fixed point...
    // f = 1-(b - d) / (m - d)
    // f = ((m -d) - (b - d)) / (m - d)
    // f = (m - d - b + d) / (m - d)
    // f = (m - b) / (m - d)
    t32 = CELL_MAX_V - bat_maxv;
    t32 *= trg_chg_i;
    t32 /= (uint32_t)(CELL_MAX_V - CELL_DERATE_V); // can never remeber C rules - just make 100% sure this doesn't demote somewhere
    trg_chg_i = t32;
  }
  
  // test - hard limit current in closing phases...
  if(bat_maxv > 3500) trg_chg_i = trg_chg_i / 10; // max 15A

  // ramp functions
  if(trg_chg_i < bat_chg_i) {
    // going down? rapid derate
    delta = bat_chg_i - trg_chg_i;
    delta /= 2;
    if(delta == 0) delta = 1;
    bat_chg_i -= delta;
  } else if(trg_chg_i > bat_chg_i) {
    // going up? very slow
    delta = trg_chg_i / 100; //100s ramp up total
    if(delta < 1) delta = 1;
    bat_chg_i += delta;
    if(bat_chg_i > trg_chg_i) bat_chg_i = trg_chg_i;
  }

  if(trg_chg_v < bat_chg_v) {
    // going down? rapid derate
    delta = bat_chg_v - trg_chg_v;
    delta /= 2;
    if(delta == 0) delta = 1;
    bat_chg_v -= delta;
  } else if(trg_chg_v > bat_chg_v) {
    // going up? very slow
    bat_chg_v += 10; // 100 s per volt... 10s per unit that the inverter can notice
    if(bat_chg_v > trg_chg_v) bat_chg_v = trg_chg_v;
  }
  Serial.print("bat_chg_v: ");
  Serial.println(bat_chg_v);
  Serial.print("trg_chg_v: ");
  Serial.println(trg_chg_v);
  Serial.print("bat_chg_i: ");
  Serial.println(bat_chg_i);
  Serial.print("trg_chg_i: ");
  Serial.println(trg_chg_i);

  // LOW VOLTAGE LOGIC

  // default to normal maximum
  bat_dis_i = BAT_DIS_I * 100U;
  
  if(bat_minv <= CELL_MIN_V) {
    // any cell <= minimum? shut off discharge
    Serial.println("LOW VOLTAGE LIMIT!");
    bat_dis_i = 0;
    bat_soc = 0;
    bat_soh = 0;
    lv_lockout = 1;
    lv_lockout_ms = millis();
  } else {
    // otherwise wait for lockout interval then release
    if(lv_lockout) {
      Serial.println("LOW VOLTAGE LOCKOUT");
      bat_dis_i = 0;
      bat_soc = 0;
      bat_soh = 0;
      if(millis() - lv_lockout_ms >= LV_LOCKOUT_MS) {
        lv_lockout = 0;        
      }
    }
  }
}
