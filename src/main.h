#include <Arduino.h>
#include "sniffer.h"
#include <FastLED.h>

#define LED_PIN 2
#define NUM_LEDS 8
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 170

CRGB leds[NUM_LEDS];
CRGB targetLeds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

void fadeLEDs();

static void showMetadata(SnifferPacket *snifferPacket);
static void setLEDs(SnifferPacket *snifferPacket);

// works on 3.something V
// doesn't work through a logic level converter
// reset breaks everything - migth be a problem with WS2812 protocol
