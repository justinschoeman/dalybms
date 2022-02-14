// DALY COMMS

// PINS FOR RS-485 DRIVER
// transmit enable (active high)
#define BMS_DE 2
// receive enable (active low)
#define BMS_NRE 3
// host (us) rs485 addr (doc says 0x80, but their software uses 0x40
#define BMS_HOST_ADDR 0x40
// slave rs485 (doc says 0x01)
#define BMS_SLAVE_ADDR 0x01

// sample message
// A5 40 90 08 00 00 00 00 00 00 00 00 7D

// guess how long we should wait for a response? (ms)
#define BMS_RX_TO 100

#if 1
#include <SoftwareSerial.h>

SoftwareSerial mySerial(8, 9); // RX, TX

// NB - TX leaves RS485 transceiver enabled for RX!
int bms_tx(uint8_t cmd) {
  int i;

  // prepare tx message
  mbuf[0] = 0xa5; //sync char
  mbuf[1] = BMS_HOST_ADDR; // our addr
  mbuf[2] = cmd;
  mbuf[3] = 0x08;
  memset(mbuf+4, 0, 8); // rest must be 0
  mbuf[12] = 0;
  // calculate checksum
  for(i = 0; i <= 11; i++) {
    mbuf[12] += mbuf[i];
    //Serial.println(mbuf[i], HEX);
  }
  //Serial.println(mbuf[12], HEX);

  // flush input buffer
  while(mySerial.available()) mySerial.read();

  delay(100);

  // send buffer
  mySerial.write(mbuf, 13);
  //for(i = 0; i<=12; i++) Serial.write(mbuf[i]);
  // wait for last bit to be out of the tx shift register (flush() does this on recent arduino builds)
  mySerial.flush();
}

// read a frame into mbuf
// return 0 on error
int bms_rx(uint8_t cmd) {
  int i;
  uint8_t cs = 0;
  unsigned long start_time = millis();

  // receive buffer (13 bytes)
  for(i = 0; i <= 12; ) {
    // serial available?
    if(mySerial.available()) {
      // if so read it
      mbuf[i] = mySerial.read();
      // debug
      //Serial.print(i);
      //Serial.print(" ");
      //Serial.println(mbuf[i], HEX);
      // state check
      if((i == 0 && mbuf[i] != 0xa5) || // first byte = sync char
          (i == 1 && mbuf[i] != BMS_SLAVE_ADDR) || // 2nd byte = slave addr
          (i == 2 && mbuf[i] != cmd) || // 3rd byte = cmd
          (i == 3 && mbuf[i] != 0x08) || //4th byte = fixed length (8)
          (i == 12 && mbuf[i] != cs)) { // 13th byte = checksum
          // not frame header - reset
          i = 0; // first byte
          cs = 0; // clear checksum
      } else {
        cs += mbuf[i]; //compute checksum
        i++; // next byte
      }
    }
    // timeout?
    if((millis() - start_time) > BMS_RX_TO) {
      return 0;
    }
  }
  // managed to exit for loop with all 13 bytes validated
  return 1;
}

// disable recevier after we have received last packet...
void bms_rx_end(void) {
  //digitalWrite(BMS_NRE, 1); // disable re
}

// set up bms rs-485 driver pins
void bms_setup(void) {
  mySerial.begin(9600);
}

#else
// NB - TX leaves RS485 transceiver enabled for RX!
int bms_tx(uint8_t cmd) {
  int i;

  // prepare tx message
  mbuf[0] = 0xa5; //sync char
  mbuf[1] = BMS_HOST_ADDR; // our addr
  mbuf[2] = cmd;
  mbuf[3] = 0x08;
  memset(mbuf+4, 0, 8); // rest must be 0
  mbuf[12] = 0;
  // calculate checksum
  for(i = 0; i <= 11; i++) {
    mbuf[12] += mbuf[i];
    //Serial.println(mbuf[i], HEX);
  }
  //Serial.println(mbuf[12], HEX);

  // flush input buffer
  while(Serial.available()) Serial.read();
  // flush output buffer
  Serial.flush();
  // enable rs-485
  digitalWrite(BMS_NRE, 1); // disable read (FIXME - should we monitor/collision detect tx/rx?)
  digitalWrite(BMS_DE, 1); // enable write
  // send buffer
  Serial.write(mbuf, 13);
  //for(i = 0; i<=12; i++) Serial.write(mbuf[i]);
  // wait for last bit to be out of the tx shift register (flush() does this on recent arduino builds)
  Serial.flush();
  // enable rs-485 input
  digitalWrite(BMS_NRE, 0);
  // disable rs-485 output
  digitalWrite(BMS_DE, 0);
}

// guess how long we should wait for a response? (ms)
#define BMS_RX_TO 100

// read a frame into mbuf
// return 0 on error
int bms_rx(uint8_t cmd) {
  int i;
  uint8_t cs = 0;
  unsigned long start_time = millis();

  // receive buffer (13 bytes)
  for(i = 0; i <= 12; ) {
    // serial available?
    if(Serial.available()) {
      // if so read it
      mbuf[i] = Serial.read();
      // debug
      //Serial.print(i);
      //Serial.print(" ");
      //Serial.println(mbuf[i], HEX);
      // state check
      if((i == 0 && mbuf[i] != 0xa5) || // first byte = sync char
          (i == 1 && mbuf[i] != BMS_SLAVE_ADDR) || // 2nd byte = slave addr
          (i == 2 && mbuf[i] != cmd) || // 3rd byte = cmd
          (i == 3 && mbuf[i] != 0x08) || //4th byte = fixed length (8)
          (i == 12 && mbuf[i] != cs)) { // 13th byte = checksum
          // not frame header - reset
          i = 0; // first byte
          cs = 0; // clear checksum
      } else {
        cs += mbuf[i]; //compute checksum
        i++; // next byte
      }
    }
    // timeout?
    if((millis() - start_time) > BMS_RX_TO) {
      return 0;
    }
  }
  // managed to exit for loop with all 13 bytes validated
  return 1;
}

// disable recevier after we have received last packet...
void bms_rx_end(void) {
  digitalWrite(BMS_NRE, 1); // disable re
}

// set up bms rs-485 driver pins
// NOTE: DOES NOT SET UP SERIAL PORT!
void bms_setup(void) {
  digitalWrite(BMS_NRE, 1);
  pinMode(BMS_NRE, OUTPUT);
  digitalWrite(BMS_DE, 0);
  pinMode(BMS_DE, OUTPUT);
}
#endif

// read a word from mbuf from data byte offset i
uint16_t bms_word(int i) {
  uint16_t w;
  i += 4;
  w = mbuf[i];
  w <<= 8;
  w += mbuf[i+1];
  return w;
}

// read a byte from mbuf from data byte offset i
uint16_t bms_byte(int i) {
  return mbuf[i+4];
}

int bms_update(void) {
  // send 90 (voltage and current)
  bms_tx(0x90);
  // rx response
  if(!bms_rx(0x90)) {
    Serial.println("Rx fail cmd 90!");
    goto fail;
  } else {
    bat_v = bms_word(0) * 10;
    Serial.print("voltage: ");
    Serial.println(bat_v/100.0);
    //Serial.print("acquisition?: ");
    //Serial.println(bms_word(2));
    bat_i = bms_word(4) - 30000; // leave as uint but 2's complement should be correct in binary anyway...
    Serial.print("current: ");
    Serial.println((int16_t)(bat_i)/10.0);
    bat_soc = bms_word(6)/10;
    Serial.print("soc: ");
    Serial.println(bat_soc);
  }
  // send 91 (max/min cell voltage)
  bms_tx(0x91);
  // rx response
  if(!bms_rx(0x91)) {
    Serial.println("Rx fail cmd 91!");
    goto fail;
  } else {
    bat_maxv = bms_word(0);
    bat_maxc = bms_byte(2);
    bat_minv = bms_word(3);
    bat_minc = bms_byte(5);
    Serial.print("max cell voltage: ");
    Serial.println(bat_maxv);
    Serial.print("max cell no: ");
    Serial.println(bat_maxc);
    Serial.print("min cell voltage: ");
    Serial.println(bat_minv);
    Serial.print("min cell no: ");
    Serial.println(bat_minc);
  }
  // send 92 (max/min temp)
  bms_tx(0x92);
  // rx response
  if(!bms_rx(0x92)) {
    Serial.println("Rx fail cmd 92!");
    goto fail;
  } else {
    bat_t = ((int)bms_byte(0) - 40) * 10;
    Serial.print("max temp: ");
    Serial.println(bat_t);
  }
  // send 97 (balancer status?) - just testing
  bms_tx(0x97);
  // rx response
  if(!bms_rx(0x97)) {
    Serial.println("Rx fail cmd 97!");
    goto fail;
  } else {
    Serial.print("bal status 1-8: ");
    Serial.println(bms_byte(0), BIN);
    Serial.print("bal status 9-16: ");
    Serial.println(bms_byte(1), BIN);
    bat_stat = bms_byte(1);
    bat_stat <<= 8;
    bat_stat |= bms_byte(0);
  }
  // no input for soh - just reset to 100...
  bat_soh = 100;
  bms_rx_end();
  return 1;
fail:
  bms_rx_end();
  return 0;
}
