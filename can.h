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

void can_send(uint16_t cmd, int len, uint8_t * b) {
  int i;
  /*Serial.print(cmd, HEX);
  Serial.print(" [");
  Serial.print(len);
  Serial.print("]");
  for(i = 0; i < len; i++) {
    Serial.print(" ");
    Serial.print(b[i], HEX);
  }
  Serial.println("");*/
  CAN.sendMsgBuf(cmd, 0, len, b);
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
  can_set_word(0, bat_chg_v/100U);
  can_set_word(2, bat_chg_i/10U);
  can_set_word(4, bat_dis_i/10U);
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
#if 0  
  // build 0x351 - charge specs
  can_clear();
  can_set_word(0, bat_chg_v/100U);
  can_set_word(2, bat_chg_i/10U);
  can_set_word(4, bat_dis_i/10U);
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


// BYD DUMP
//https://powerforum.co.za/topic/4406-revovbydvictron/
//  can0  305   [8]  00 00 00 00 00 00 00 00 -- inverter response
//  can0  351   [8]  3A 02 64 00 64 00 A4 01
//  can0  355   [6]  32 00 64 00 00 00
//  can0  356   [6]  B4 14 64 00 F0 00
//  can0  35A   [8]  A8 82 82 02 A8 82 82 02  
//  can0  35E   [8]  42 59 44 00 00 00 00 00
//  can0  35F   [8]  4C 69 01 02 00 00 00 00

#if 0
  // build 0x351 - charge specs
  //  can0  351   [8]  3A 02 64 00 64 00 A4 01
  //                   570   100   100   420
  can_clear();
  can_set_word(0, bat_chg_v/100U);
  can_set_word(2, bat_chg_i/10U);
  can_set_word(4, bat_dis_i/10U);
  can_set_word(6, bat_dis_v/100U);
  CAN.sendMsgBuf(0x351, 0, 8, mbuf);

  delay(5);
  // build 0x355 - bat state
  //  can0  355   [6]  32 00 64 00 00 00
  //                   50    100   0
  can_clear();
  can_set_word(0, bat_soc);
  can_set_word(2, bat_soh);
  CAN.sendMsgBuf(0x355, 0, 6, mbuf);

  delay(5);
  // build 0x356 - bat measurements
  //  can0  356   [6]  B4 14 64 00 F0 00
  //                   5300  100   240
  can_clear();
  can_set_word(0, bat_v);
  can_set_word(2, bat_i);
  can_set_word(4, bat_t);
  CAN.sendMsgBuf(0x356, 0, 6, mbuf);

  delay(5);
  // build 0x35a - fault/warning flags
  //  can0  35A   [8]  A8 82 82 02 A8 82 82 02  
  //  should be safe to send as zeros
  can_clear();
  CAN.sendMsgBuf(0x35A, 0, 8, mbuf);


  delay(5);
  // build 0x35e - mfg name
  //  can0  35E   [8]  42 59 44 00 00 00 00 00
  can_clear();
  mbuf[0] = 0x42;
  mbuf[1] = 0x59;
  mbuf[2] = 0x44;
  CAN.sendMsgBuf(0x35E, 0, 8, mbuf);
  
  delay(5);
  // build 0x35a - chemistry
  //  can0  35F   [8]  4C 69 01 02 00 00 00 00
  //                   c1 c2  1.2  0     0.0
  //  should be safe to send as zeros
  can_clear();
  mbuf[0] = 0x4c;
  mbuf[1] = 0x69;
  mbuf[2] = 0x01;
  mbuf[3] = 0x02;
  can_set_word(4, 360); // capacity?
  CAN.sendMsgBuf(0x35F, 0, 8, mbuf);
#endif


// simpbms
#if 0
  // build 0x351 - charge specs
  can_clear();
  can_set_word(0, bat_dis_i/10U);
  can_set_word(2, 400);
  can_set_word(4, bat_chg_i/10U);
  can_set_word(6, bat_chg_v/100U);
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

#if 1
  // can dump from plonkster@powerforum
  // can1  351   [6]  14 02 50 02 C8 05
  //                  532   592   1480
  can_clear();
  can_set_word(0, bat_chg_v/100U);
  can_set_word(2, bat_chg_i/10U);
  can_set_word(4, bat_dis_i/10U);
  can_send(0x351, 6, mbuf);
  
  // can1  355   [4]  5F 00 63 00
  //                  95    99
  delay(5);
  can_clear();
  can_set_word(0, bat_soc);
  can_set_word(2, bat_soh);
  can_send(0x355, 4, mbuf);
  
  // can1  356   [6]  77 13 A9 FE 0B 01
  //                  4983  -343  267
  delay(5);
  can_clear();
  can_set_word(0, bat_v);
  can_set_word(2, bat_i);
  can_set_word(4, bat_t);
  can_send(0x356, 6, mbuf);
  
  // can1  359   [7]  00 00 00 00 04 50 4E
  delay(5);
  can_clear();
  mbuf[4] = 1;
  mbuf[5] = 'P';
  mbuf[6] = 'N';
  can_send(0x359, 7, mbuf);
  
  // can1  35C   [2]  C0 00
  delay(5);
  can_clear();
  if(bat_chg_i) mbuf[0] |= 0x80;
  if(bat_dis_i) mbuf[0] |= 0x40;
  //mbuf[0] = 0xC0;
  can_send(0x35C, 2, mbuf);
  
  // can1  35E   [8]  50 59 4C 4F 4E 20 20 20
  delay(5);
  can_clear();
  mbuf[0] = 0x50;
  mbuf[1] = 0x59;
  mbuf[2] = 0x4C;
  mbuf[3] = 0x4F;
  mbuf[4] = 0x4E;
  mbuf[5] = 0x20;
  mbuf[6] = 0x20;
  mbuf[7] = 0x20;
  can_send(0x35E, 8, mbuf);
#endif

}
