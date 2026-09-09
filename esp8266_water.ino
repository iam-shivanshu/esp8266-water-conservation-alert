#include <ESP8266WiFi.h>
#include <sigma_delta.h>
#include "audio_data.h"

// ===== SAME PINS =====
#define TRIG_PIN   14   // D5
#define ECHO_PIN   12   // D6
#define HWSD_PIN   5    // D1

#define DETECT_CM  40

#define GPSD  ESP8266_REG(0x368)
#define GPSDT 0

bool personPresent = false;

// ---------------- AUDIO ----------------

static inline int16_t audioAt(uint32_t i) {
  return (int16_t)pgm_read_word(&audioData[i]);
}

static inline void hwsdWriteDuty(uint8_t duty) {
  uint32_t reg = GPSD;

  reg = (reg & ~(0xFF << GPSDT)) |
        ((uint32_t)duty << GPSDT);

  GPSD = reg;
}

void audioBegin() {

  sigmaDeltaSetup(0, 312500);

  sigmaDeltaWrite(0, 128);

  sigmaDeltaAttachPin(HWSD_PIN);

  delay(200);
}

void playAudio() {

  Serial.println("PLAYING AUDIO...");

  uint32_t cyclesPerSample =
    ESP.getCpuFreqMHz() * 1000000UL / AUDIO_SAMPLE_RATE;

  uint32_t next = ESP.getCycleCount();

  for (uint32_t i = 0; i < audioLen; i++) {

    int16_t sample = audioAt(i);

    next += cyclesPerSample;

    while ((int32_t)(ESP.getCycleCount() - next) < 0) {
      yield();
    }

    uint8_t output =
      (uint8_t)((sample >> 8) + 128);

    hwsdWriteDuty(output);

    if ((i & 0x7F) == 0) {
      yield();
    }
  }

  // Center output
  hwsdWriteDuty(128);

  Serial.println("AUDIO FINISHED");
}

// ---------------- ULTRASONIC ----------------

long readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration =
    pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return 999;
  }

  return duration / 58;
}

// ---------------- SETUP ----------------

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("SYSTEM START");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  // Audio initialization
  audioBegin();

  Serial.println("AUDIO READY");
}

// ---------------- LOOP ----------------

void loop() {

  long distance = readDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Object detected
  if (distance <= DETECT_CM) {

    if (!personPresent) {

      personPresent = true;

      Serial.println("OBJECT DETECTED!");

      playAudio();
    }

  } else {

    // Object moved away
    personPresent = false;
  }

  delay(150);
}