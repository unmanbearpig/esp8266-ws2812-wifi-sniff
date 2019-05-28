#include "main.h"

uint16_t loopNum = 0;

void setupLeds() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  FastLED.setMaxRefreshRate(200);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

static void setLEDs(SnifferPacket *snifferPacket) {
  // leds[mod8(snifferPacket->data[0] % NUM_LEDS)] =

  uint8_t simplified_mac_first_3 =
    (
     snifferPacket->data[10] ^
     snifferPacket->data[11] ^
     snifferPacket->data[12]
     );

  uint8_t simplified_mac_last_3 =
    (
     snifferPacket->data[13] ^
     snifferPacket->data[14] ^
     snifferPacket->data[15]
     );


  uint8_t led_num = mod8(simplified_mac_last_3, NUM_LEDS);

  nblend(
         targetLeds[led_num],
         CHSV(simplified_mac_first_3, 220, (255 - ((abs(snifferPacket->rx_ctrl.rssi) -50) * 3))),
         50
         );
}

uint8_t sinFunc(uint16_t input) {
  return sin8(input % 256);
}

static void sinLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t value = sinFunc(loopNum + i * 4) / 10;
    if(!(leds[i].r == 0 && leds[i].g == 0 && leds[i].b == 0)) {
      nblend(leds[i], CRGB(value, value, value), 9);

    }
  }
}

static void updateLEDs() {
  for(int i = NUM_LEDS; i--; i >= 0) {
    nblend(leds[i], targetLeds[i], 20);
  }

  sinLEDs();
  fadeLEDs();

  //if (loopNum % 10 == 0) {
  // blur1d(leds, NUM_LEDS, 10);
  //}
  FastLED.show();
  // Serial.print("*");
}
static void showMetadata(SnifferPacket *snifferPacket) {

  unsigned int frameControl = ((unsigned int)snifferPacket->data[1] << 8) + snifferPacket->data[0];

  uint8_t version      = (frameControl & 0b0000000000000011) >> 0;
  uint8_t frameType    = (frameControl & 0b0000000000001100) >> 2;
  uint8_t frameSubType = (frameControl & 0b0000000011110000) >> 4;
  uint8_t toDS         = (frameControl & 0b0000000100000000) >> 8;
  uint8_t fromDS       = (frameControl & 0b0000001000000000) >> 9;

  // Only look for probe request packets
  if (frameType != TYPE_MANAGEMENT ||
      frameSubType != SUBTYPE_PROBE_REQUEST)
    return;

  Serial.print("RSSI: ");
  Serial.print(snifferPacket->rx_ctrl.rssi, DEC);

  Serial.print(" Ch: ");
  Serial.print(wifi_get_channel());

  char addr[] = "00:00:00:00:00:00";
  getMAC(addr, snifferPacket->data, 10);
  Serial.print(" Peer MAC: ");
  Serial.print(addr);

  uint8_t SSID_length = snifferPacket->data[25];
  Serial.print(" SSID: ");
  printDataSpan(26, SSID_length, snifferPacket->data);

  Serial.println();
}

/**
 * Callback for promiscuous mode
 */
static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;
  // showMetadata(snifferPacket);
  setLEDs(snifferPacket);
}

static void printDataSpan(uint16_t start, uint16_t size, uint8_t* data) {
  for(uint16_t i = start; i < DATA_LENGTH && i < start+size; i++) {
    Serial.write(data[i]);
  }
}

static void getMAC(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

#define CHANNEL_HOP_INTERVAL_MS   1000
static os_timer_t channelHop_timer;

/**
 * Callback for channel hoping
 */
void channelHop()
{
  // hoping channels 1-13
  uint8 new_channel = wifi_get_channel() + 1;
  if (new_channel > 13) {
    new_channel = 1;
  }
  wifi_set_channel(new_channel);
}

#define FADE_LEDS_INTERVAL_MS 10
static os_timer_t fadeLEDs_timer;


void fadeLEDs() {
  if (loopNum % 25 == 0) {
    fadeUsingColor(targetLeds, NUM_LEDS, CRGB(200, 245, 254));
  }

  if (loopNum % 17 == 0) {
    // fadeUsingColor(leds, NUM_LEDS, CRGB(210, 253, 254));
    //blur1d(leds, NUM_LEDS, 10);
  }

  // Serial.print("v");
}

#define DISABLE 0
#define ENABLE  1

void setup() {
  // set the WiFi chip to "promiscuous" mode aka monitor mode
  Serial.begin(115200);
  setupLeds();
  delay(10);
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(6);
  wifi_promiscuous_enable(DISABLE);
  delay(10);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  delay(10);
  wifi_promiscuous_enable(ENABLE);

  // setup the channel hoping callback timer
  // os_timer_disarm(&channelHop_timer);
  // os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
  // os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);

  // os_timer_disarm(&fadeLEDs_timer);
  // os_timer_setfn(&fadeLEDs_timer, (os_timer_func_t *) fadeLEDs, NULL);
  // os_timer_arm(&fadeLEDs_timer, FADE_LEDS_INTERVAL_MS, 1);

}


void loop() {
  loopNum += 1;
  FastLED.delay(1);
  updateLEDs();
}
