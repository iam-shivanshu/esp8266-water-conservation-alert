/*
 * Water-tap voice reminder - ESP8266 NodeMCU - SIMPLE MODE
 *
 * Detection: Ultrasonic sensor within 40 cm
 * Action: Play audio sound once per person
 *
 * Pins: TRIG=D5(GPIO14), ECHO=D6(GPIO12), Audio=D1(GPIO5)
 */

#include <ESP8266WiFi.h>
#include <sigma_delta.h>
#include "audio_data.h"

#define TRIG_PIN   14  // D5
#define ECHO_PIN   12  // D6
#define DETECT_CM  40
#define HWSD_PIN   5   // D1

#define GPSD  ESP8266_REG(0x368)
#define GPSDT 0

static inline int16_t audioAt(uint32_t i) {
  return (int16_t)pgm_read_word(&audioData[i]);
}

static inline void hwsdWriteDuty(uint8_t duty) {
  uint32_t reg = GPSD;
  reg = (reg & ~(0xFF << GPSDT)) | ((uint32_t)duty << GPSDT);
  GPSD = reg;
}

static void audioBegin() {
  sigmaDeltaSetup(0, 312500);
  sigmaDeltaWrite(0, 128);
  sigmaDeltaAttachPin(HWSD_PIN);
  delay(250);
}

static void playAudio() {
  const uint32_t cyclesPerSample = ESP.getCpuFreqMHz() * 1000000UL / AUDIO_SAMPLE_RATE;
  uint32_t next = ESP.getCycleCount();

  for (uint32_t i = 0; i < audioLen; i++) {
    int16_t s = audioAt(i);
    next += cyclesPerSample;
    while ((int32_t)(ESP.getCycleCount() - next) < 0) {}
    hwsdWriteDuty((uint8_t)((s >> 8) + 128));
    if ((i & 0x7F) == 0) {
      yield();
      if ((int32_t)(ESP.getCycleCount() - next) > (int32_t)(4 * cyclesPerSample)) {
        next = ESP.getCycleCount();
      }
    }
  }
  hwsdWriteDuty(128);
}

static long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  return duration / 58;
}

static bool personPresent = false;

void setup() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  audioBegin();

  // Watchdog: auto-reset if loop hangs for 10 seconds
  ESP.wdtEnable(10000);
}

void loop() {
  // Simple median of 3 readings to filter noise
  long a = readDistanceCM();
  long b = readDistanceCM();
  long c = readDistanceCM();
  if (a > b) { long t = a; a = b; b = t; }
  if (b > c) { long t = b; b = c; c = t; }
  if (a > b) { long t = a; a = b; b = t; }
  long dist = b;

  // Simple trigger: within 40 cm, play once
  if (dist < DETECT_CM) {
    if (!personPresent) {
      personPresent = true;
      playAudio();
    }
  } else {
    personPresent = false;
  }

  delay(150);

  // Feed watchdog - confirm loop is alive
  ESP.wdtFeed();
}
