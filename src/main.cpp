#include "main.h"
#include "sniffer.h"

uint16_t loopNum = 0;

int8_t minSignal = 127;
int8_t maxSignal = -127;

void updateSignalRange(int8_t signal) {
  if(signal < minSignal) {
    minSignal = signal;
  }

  if (signal > maxSignal) {
    maxSignal = signal;
  }
}

static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;
  // showMetadata(snifferPacket);
  Serial.print("<");
  setLEDs(snifferPacket);
}

void setupLeds() {
  delay(3000);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  //FastLED.setMaxRefreshRate(120);

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

  int8_t signal = snifferPacket->rx_ctrl.rssi;;
  updateSignalRange(signal);

  // int8_t scaledSignal = ((signal - minSignal) / (maxSignal - minSignal)) * 255;

  uint8_t scaledSignal = map(minSignal, maxSignal, 10, 255, signal);
  //uint8_t scaledSignal = 100;

  nblend(
         targetLeds[led_num],
         CHSV(simplified_mac_first_3, 220, scaledSignal),
         250
         );
}

void naddColor(CRGB *target, CRGB *other) {
  target->r = qadd8(target->r, other->r);
  target->g = qadd8(target->g, other->g);
  target->b = qadd8(target->b, other->b);
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
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    naddColor(&leds[i], &targetLeds[i]);
  }

  sinLEDs();
  fadeLEDs();

  FastLED.show();
}

#define FADE_LEDS_INTERVAL_MS 10
static os_timer_t fadeLEDs_timer;


void fadeLEDs() {
  if (loopNum % 3 == 0) {
    fadeUsingColor(targetLeds, NUM_LEDS, CRGB(200, 245, 254));
  }

  if (loopNum % 17 == 0) {
    // fadeUsingColor(leds, NUM_LEDS, CRGB(210, 253, 254));
    //blur1d(leds, NUM_LEDS, 10);
  }

  // Serial.print("v");
}

void setupChannelHoppingTimer() {
  // setup the channel hoping callback timer
  os_timer_disarm(&channelHop_timer);
  os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
  os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}

void setup() {
  Serial.begin(115200);
  Serial.println("wifi-sniff started");
  setupLeds();
  setupSniffer(&sniffer_callback);

  // os_timer_disarm(&fadeLEDs_timer);
  // os_timer_setfn(&fadeLEDs_timer, (os_timer_func_t *) fadeLEDs, NULL);
  // os_timer_arm(&fadeLEDs_timer, FADE_LEDS_INTERVAL_MS, 1);
  Serial.println("setup done");
}

uint8_t introAnimationMode = 1;

void FillLEDsFromPaletteColors( uint8_t colorIndex) {
  uint8_t brightness = 255;

  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 13;
  }
}

void introAnimationUpdateLEDs() {
  static uint8_t colorIndex = 0;

  FillLEDsFromPaletteColors(colorIndex);

  colorIndex += 1;
}

void loop() {
  loopNum += 1;
  FastLED.delay(1);

  if (introAnimationMode == 0) {
    updateLEDs();
  } else {
    if (millis() > 6000) {
      introAnimationMode = 0;
      Serial.println("end intro animation");
    } else {
      introAnimationUpdateLEDs();
    }
  }
}
