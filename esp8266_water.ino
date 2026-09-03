/*
 * Water-tap voice reminder - ESP8266 NodeMCU - SIMPLE MODE
 *
 * Behavior:
 *  - Someone enters DETECT_CM -> message starts playing.
 *  - If they walk away before it finishes, the message is NOT cut short -
 *    it always plays to completion.
 *  - If they leave and then come back under DETECT_CM again before the
 *    message finishes, it restarts from the beginning.
 *  - Once a person has stood there through the whole message with no
 *    gap, it does not auto-repeat - a fresh approach (leave, then
 *    re-enter) is needed to play it again.
 *
 * Pins: TRIG=D5(GPIO14), ECHO=D6(GPIO12), Audio=D1(GPIO5)
 */

#include <ESP8266WiFi.h>
#include <sigma_delta.h>
#include "audio_data.h"

#define TRIG_PIN   14  // D5
#define ECHO_PIN   12  // D6
#define HWSD_PIN   5   // D1
#define DETECT_CM  40

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

// Full 30 ms-timeout ping used for the idle-loop check (robust, but slow).
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

// Short-timeout ping (~2.5 ms) used only *during* playback, so checking
// for a re-entry doesn't stall the sample-accurate audio loop for long.
// 2500us round trip corresponds to roughly 43 cm - plenty for a 40 cm gate.
static bool quickPresent() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long d = pulseIn(ECHO_PIN, HIGH, 2500);
  if (d == 0) return false;
  return (long)(d / 58) < DETECT_CM;
}

// Plays the clip start to finish. Every ~250 ms of audio it takes a quick
// look at the sensor; if the person had stepped away and just came back
// under DETECT_CM, playback restarts from sample 0. Someone leaving does
// NOT stop playback early - the loop always runs to the end unless a
// fresh re-entry restarts it. Returns the last observed presence state so
// the caller's edge-detector (wasPresent) stays in sync.
static bool playAudio() {
  const uint32_t cyclesPerSample = ESP.getCpuFreqMHz() * 1000000UL / AUDIO_SAMPLE_RATE;
  const uint32_t CHECK_EVERY = 4000;   // ~250 ms of audio between checks
  bool present = true;                 // caller only calls this right after detecting entry
  uint32_t i = 0;
  uint32_t next = ESP.getCycleCount();

  while (i < audioLen) {
    int16_t s = audioAt(i);
    next += cyclesPerSample;
    while ((int32_t)(ESP.getCycleCount() - next) < 0) {}
    hwsdWriteDuty((uint8_t)((s >> 8) + 128));
    i++;

    if ((i % CHECK_EVERY) == 0) {
      yield();
      ESP.wdtFeed();
      bool now = quickPresent();
      if (now && !present) {
        i = 0;                         // fresh re-entry: restart the message
        next = ESP.getCycleCount();
      }
      present = now;
      if ((int32_t)(ESP.getCycleCount() - next) > (int32_t)(4 * cyclesPerSample)) {
        next = ESP.getCycleCount();
      }
    }
  }
  hwsdWriteDuty(128);
  return present;
}

static bool wasPresent = false;

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
  // Robust median-of-3 for the idle-loop trigger decision.
  long a = readDistanceCM();
  long b = readDistanceCM();
  long c = readDistanceCM();
  if (a > b) { long t = a; a = b; b = t; }
  if (b > c) { long t = b; b = c; c = t; }
  if (a > b) { long t = a; a = b; b = t; }
  long dist = b;
  bool present = dist < DETECT_CM;

  if (present && !wasPresent) {
    wasPresent = playAudio();   // blocks until the clip finishes; handles its own restarts
  } else {
    wasPresent = present;
  }

  delay(150);
  ESP.wdtFeed();
}
