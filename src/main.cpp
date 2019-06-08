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
  setLEDs(snifferPacket);
}

void setupLeds() {
  delay(10);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxRefreshRate(MAX_LED_REFRESH_RATE);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

static void setLEDs(SnifferPacket *snifferPacket) {
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

  uint8_t scaledSignal = map(minSignal, maxSignal, 10, 255, signal);
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

static void fadeLEDs() {
  if (loopNum % 3 == 0) {
    fadeUsingColor(targetLeds, NUM_LEDS, CRGB(200, 245, 254));
  }
}

static void blendInTargetLeds() {
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    naddColor(&leds[i], &targetLeds[i]);
  }
}

static void updateLEDs() {
  blendInTargetLeds();
  sinLEDs();
  fadeLEDs();

  FastLED.show();
}


void setupChannelHoppingTimer() {
  // setup the channel hoping callback timer
  os_timer_disarm(&channelHop_timer);
  os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
  os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}

#define BUTTON_PIN 0

void setupButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void setup() {
  Serial.begin(115200);
  Serial.println("wifi-sniff started");
  setupLeds();
  setupButton();
  setupSniffer(&sniffer_callback);

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

bool isButtonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;
}

uint8_t brightness = BRIGHTNESS;

void loop() {
  loopNum += 1;
  FastLED.delay(1);

  if (isButtonPressed()) {
    brightness += 1;
    FastLED.setBrightness(brightness);
    introAnimationUpdateLEDs();
  } else {
    if (introAnimationMode == 0) {
      updateLEDs();
    } else {
      if (millis() > 4000) {
        introAnimationMode = 0;
        Serial.println("end intro animation");
      } else {
        introAnimationUpdateLEDs();
      }
    }
  }
}
