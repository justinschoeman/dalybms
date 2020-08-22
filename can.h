//https://github.com/coryjfowler/MCP_CAN_lib

#define CAN_CS 10
// name to display - 8 chars
uint8_t can_name[] = "DALY.BMS";
// mfg to display - 8 chars
uint8_t can_mfg[] = "DALY.CAN";

#include <mcp_can.h>
//#include <SPI.h>

MCP_CAN CAN(CAN_CS); //set CS pin for can controlelr

void can_setup() {
  if(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    while(1); // fixme - error handler
  }
  CAN.setMode(MCP_NORMAL); 
}

void can_set_word(int i, uint16_t w) {
  mbuf[i] = lowByte(w);
  mbuf[i+1] = highByte(w);
}
void can_clear(void) {
  memset(mbuf, 0, 8);
}

int can_update(void) {
// pylontech
#if 0  
  // build 0x359 - alarms
  can_clear();
  mbuf[5] = 'P';
  mbuf[6] = 'N';
  CAN.sendMsgBuf(0x359, 0, 8, mbuf);

  delay(5);
  // build 0x351 - charge specs
  can_clear();
  can_set_word(0, bat_chg_v);
  can_set_word(2, bat_chg_i);
  can_set_word(4, bat_dis_i);
  CAN.sendMsgBuf(0x351, 0, 8, mbuf);

  delay(5);
  // build 0x355 - bat state
  can_clear();
  can_set_word(0, bat_soc);
  can_set_word(2, bat_soh);
  can_set_word(3, bat_soc * 10); // from vecan?
  CAN.sendMsgBuf(0x355, 0, 8, mbuf);

  delay(5);
  // build 0x356 - bat measurements
  can_clear();
  can_set_word(0, bat_v);
  can_set_word(2, bat_i);
  can_set_word(4, bat_t);
  CAN.sendMsgBuf(0x356, 0, 8, mbuf);
  
  delay(5);
  // build 0x35a - no idea - from vecan
  can_clear();
  CAN.sendMsgBuf(0x35A, 0, 8, mbuf);

  delay(5);
  // send 0x370 - name
  CAN.sendMsgBuf(0x370, 0, 8, can_name);

  delay(5);
  // send 0x35e - mfg
  CAN.sendMsgBuf(0x35E, 0, 8, can_mfg); 

  delay(5);
  // build 0x35c - charge/discharge status
  can_clear();
  // fixme - set from override flags
  mbuf[0] = 0xe0;
  CAN.sendMsgBuf(0x35AC, 0, 8, mbuf);
#endif


// elithium
#if 1  
  // build 0x351 - charge specs
  can_clear();
  can_set_word(0, bat_chg_v);
  can_set_word(2, bat_chg_i);
  can_set_word(4, bat_dis_i);
  can_set_word(6, 400);
  CAN.sendMsgBuf(0x351, 0, 8, mbuf);

  delay(5);
  // build 0x355 - bat state
  can_clear();
  can_set_word(0, bat_soc);
  can_set_word(2, bat_soh);
  CAN.sendMsgBuf(0x355, 0, 8, mbuf);

  delay(5);
  // build 0x356 - bat measurements
  can_clear();
  can_set_word(0, bat_v);
  can_set_word(2, bat_i);
  can_set_word(4, bat_t);
  CAN.sendMsgBuf(0x356, 0, 8, mbuf);

  delay(5);
  // build 0x35a - fault/warning flags
  can_clear();
  CAN.sendMsgBuf(0x35A, 0, 8, mbuf);
#endif


// simpbms
#if 0
  // build 0x351 - charge specs
  can_clear();
  can_set_word(0, bat_dis_i);
  can_set_word(2, 400);
  can_set_word(4, bat_chg_i);
  can_set_word(6, bat_chg_v);
  CAN.sendMsgBuf(0x351, 0, 8, mbuf);

  delay(5);
  // build 0x355 - bat state
  can_clear();
  can_set_word(0, bat_soc);
  can_set_word(2, bat_soh);
  can_set_word(3, bat_soc * 100); // from vecan?
  CAN.sendMsgBuf(0x355, 0, 8, mbuf);

  delay(5);
  // build 0x356 - bat measurements
  can_clear();
  can_set_word(0, bat_v);
  can_set_word(2, bat_i);
  can_set_word(4, bat_t);
  CAN.sendMsgBuf(0x356, 0, 8, mbuf);
  
  delay(5);
  // build 0x35a - no idea - from vecan
  can_clear();
  CAN.sendMsgBuf(0x35A, 0, 8, mbuf);

  delay(5);
  // send 0x370 - name
  can_set_word(0, 3300);
  can_set_word(2, 3400);
  can_set_word(4, bat_t + 273);
  can_set_word(6, bat_t + 273);
  CAN.sendMsgBuf(0x373, 0, 8, can_name);

#endif
}
