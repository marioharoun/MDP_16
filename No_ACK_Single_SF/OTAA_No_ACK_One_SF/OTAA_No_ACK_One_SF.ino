/*******************************************************************************
   Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

   Permission is hereby granted, free of charge, to anyone
   obtaining a copy of this document and accompanying files,
   to do whatever they want with them without any restriction,
   including, but not limited to, copying, modification and redistribution.
   NO WARRANTY OF ANY KIND IS PROVIDED.

   This example sends a valid LoRaWAN packet with payload "Hello,
   world!", using frequency and encryption settings matching those of
   the (early prototype version of) The Things Network.

   Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in g1,
    0.1% in g2).

   Change DEVADDR to a unique address!
   See http://thethingsnetwork.org/wiki/AddressSpace

   Do not forget to define the radio type correctly in config.h.

 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define LMIC_DEBUG_LEVEL 2

// Change this!
// This should be in little endian format.
static const u1_t PROGMEM DEVEUI[8] = {0x4c,0x15,0x62,0xa4,0xd0,0x6a,0xd7,0x54};
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

// Change this!
static const u1_t PROGMEM APPKEY[16] = {0xad,0x21,0x79,0x24,0xf0,0x4c,0xaf,0x04,0x99,0xe6,0x2e,0x84,0x11,0x35,0x2b,0xd8};
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

static osjob_t sendjob;

// Schedule TX every this many seconds
const unsigned TX_INTERVAL = 1;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = {2, 6, 7},
};

// Define the single channel and data rate (SF) to use
int channel = 0;
int dr = DR_SF7;

// Disables all channels, except for the one defined above, and sets the
// data rate (SF). This only affects uplinks; for downlinks the default
// channels or the configuration from the OTAA Join Accept are used.
//
// Not LoRaWAN compliant; FOR TESTING ONLY!
//
void forceTxSingleChannelDr() {
    for(int i=0; i<9; i++) { // For EU; for US use i<71
        if(i != channel) {
            LMIC_disableChannel(i);
        }
    }
    // Set data rate (SF) and transmit power for uplink
    LMIC_setDrTxpow(dr, 14);
}

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
    // Ignore the channels from the Join Accept
    forceTxSingleChannelDr();

    // Disable link check validation (automatically enabled during join)
    LMIC_setLinkCheckMode(0);

    break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.dataLen) {
        // data received in rx slot after tx
        Serial.print(F("Data Received: "));
        Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        Serial.println();
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

byte buffer[52];

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    String message = String(LMIC.txCnt);
    message.getBytes(buffer, message.length() + 1);
    Serial.println("Sending: " + message);
    LMIC_setTxData2(1, (uint8_t*) buffer, message.length() , 0);
    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

// Leave this empty
static const u1_t PROGMEM APPEUI[8] = {};
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting"));

#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Make LMiC initialize the default channels, choose a channel, and
// schedule the OTAA join
LMIC_startJoining();

// LMiC will already have decided to send on one of the 3 default
// channels; ensure it uses the one we want
LMIC.txChnl = channel;

// ...and make sure we see the EV_JOINING event being logged
os_runloop_once();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  LMIC_setupChannel(0, 868100000, DR_SF7,  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7B),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  // Disable link check validation
  LMIC_setLinkCheckMode(0);
  LMIC_setAdrMode(0);

// Set data rate (SF) and transmit power for uplink
LMIC_setDrTxpow(dr, 14);
//  LMIC_setDrTxpow(DR_SF7, 14);
//  LMIC_setDrTxpow(DR_SF8, 23);
//  LMIC_setDrTxpow(DR_SF9, 23);
//  LMIC_setDrTxpow(DR_SF10, 23);
//  LMIC_setDrTxpow(DR_SF11, 23);
//  LMIC_setDrTxpow(DR_SF12, 23);
  // Start job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
