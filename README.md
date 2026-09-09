# 💧 Water Conservation Alert System

[![Build](https://github.com/Student2026-maker/esp8266-water-conservation-alert/actions/workflows/build.yml/badge.svg)](https://github.com/Student2026-maker/esp8266-water-conservation-alert/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Platform](https://img.shields.io/badge/platform-ESP8266-blue)

**An ESP8266-based smart water-tap reminder that plays a spoken alert when someone approaches — no app, no Wi-Fi, no cloud. Just a chip, a sensor, and a speaker.**

Built for real-world deployment: schools, hostels, offices — anywhere people need a gentle nudge to not waste water.

---

## 🎯 What it does

An ultrasonic sensor (HC-SR04 / HY-SRF05, interchangeable) watches a water tap. When someone comes within **40 cm**, it plays a spoken voice message once — *"Please don't waste water. Water is precious. Use water wisely."* It re-arms automatically once they step away, ready for the next person.

## ⚙️ How it works

- **Audio playback** uses the ESP8266's internal **hardware sigma-delta modulator** (not `analogWrite()`, not I2S) — a single GPIO pin drives an amplifier and speaker directly.
- Voice is generated once offline (neural TTS), preprocessed (DC-block, band-limit, soft-limited gain, fade in/out) and baked into flash as a 16-bit PCM array — no SD card, no MP3 decoder needed.
- The amplifier and speaker's own limited high-frequency response naturally roll off the sigma-delta carrier, so no extra analog filtering stage is needed — see [Hardware](#-hardware) for the current build.

## 🔌 Hardware

Current build runs on a single 12V supply and a higher-power amplifier instead of a 5V-only module like the PAM8403:

| Component | Notes |
|---|---|
| ESP8266 NodeMCU | Any NodeMCU v2/v3 board |
| HC-SR04 / HY-SRF05 ultrasonic sensor | 5V powered — **not** 3.3V |
| 12V DC input (barrel jack) | Single supply for the whole circuit |
| LM7805 voltage regulator | Steps the 12V down to 5V for the NodeMCU only |
| TPA1031 class-D amplifier (DC8–28V rated) | Powered directly from the 12V rail (within its rated range) — NOT from the LM7805's 5V output |
| 4–8 Ω speaker | |

**Wiring notes:**
- The amplifier's audio input is driven directly from GPIO5/D1 (a salvaged 3.5 mm jack is used here purely as a connector, not for an external audio source).
- **All grounds must be common**: 12V supply GND, LM7805 GND, NodeMCU GND, and the amplifier's GND (including its audio-input-side ground) all tie together. This is essential — the amplifier reads the GPIO signal relative to NodeMCU's ground, so if that reference is missing, the amp gets a floating/incorrect signal.
- If you use a lower-power 5V-only amp (e.g. PAM8403) instead, skip the LM7805/12V rail and power everything from the NodeMCU's own 5V — see the project's commit history for that variant.

### Pinout

| Signal | GPIO | NodeMCU label |
|---|---|---|
| Audio out | GPIO5 | D1 |
| Ultrasonic TRIG | GPIO14 | D5 |
| Ultrasonic ECHO | GPIO12 | D6 |

![Wiring diagram](wiring_diagram.jpg)

## 🚀 Getting started

1. Install the [ESP8266 Arduino core](https://github.com/esp8266/Arduino) in Arduino IDE (Board Manager → search "esp8266").
2. Select board: **NodeMCU 1.0 (ESP-12E Module)**, CPU frequency **160 MHz**.
3. Wire up the hardware per the pinout above.
4. Open `esp8266_water.ino`, select your COM port, and upload.
5. Power on — the unit starts detecting immediately.

## 📁 Project structure

```
esp8266_water.ino   — main sketch: sensor logic + hardware audio playback
audio_data.h         — precompiled 16-bit PCM voice message (PROGMEM)
water_raw.bin         — raw audio source used to generate audio_data.h
convert_audio.py      — turns any audio file into audio_data.h / water_raw.bin
requirements.txt      — Python deps for convert_audio.py
wiring_diagram.jpg     — hand-drawn circuit diagram for the 12V build
```

## 🔊 Using your own voice message

Record anything — your own voice, a different language, a different script — as an MP3, WAV, M4A, or anything else `ffmpeg`/`PyAV` can decode. Then run:

```bash
pip install -r requirements.txt
python convert_audio.py my_message.mp3
```

This regenerates `water_raw.bin` and `audio_data.h` in place, ready to be picked up by the next Arduino upload — no manual audio editing needed. The script:

1. Resamples to 16 kHz mono.
2. DC-blocks (80 Hz high-pass) and band-limits (7 kHz low-pass) for clean, intelligible speech through a small speaker.
3. Applies tanh soft-limiting for extra loudness without hard clipping.
4. Fades the clip in/out over 5 ms so playback starts and ends at exact silence (no clicks/pops).

Useful flags:

```bash
# Louder (more compression) - try values between 1.8 (clean) and ~3.5 (loud, more compressed)
python convert_audio.py my_message.mp3 --drive 3.0

# Lower peak level, more headroom
python convert_audio.py my_message.mp3 --peak 0.85
```

After running it, just re-upload the sketch in Arduino IDE — `esp8266_water.ino` already `#include`s `audio_data.h`.

**Tips:**
- Keep clips short (2–6 seconds) — they're stored in program flash alongside the sketch.
- The script prints a `duty range` at the end; keep it away from the extremes (`0` and `255`) — the ESP8266's hardware sigma-delta output loses linearity near those, which is what causes speech to sound distorted rather than louder. If your range is creeping close to the edges, lower `--drive` or `--peak`.

## 🛠️ Customization

- `DETECT_CM` — trigger distance in cm (default 40)
- Swap `audio_data.h` for a different message/language
- Add a cooldown timer if deploying somewhere with continuous foot traffic

## 📜 License

MIT — use it, fork it, deploy it anywhere water is being wasted.

---

⭐ If this helped you build something, consider starring the repo — it helps others find it too.
