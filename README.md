# 💧 Water Conservation Alert System

**An ESP8266-based smart water-tap reminder that plays a spoken alert when someone approaches — no app, no Wi-Fi, no cloud. Just a chip, a sensor, and a speaker.**

Built for real-world deployment: schools, hostels, offices — anywhere people need a gentle nudge to not waste water.

---

## 🎯 What it does

An HC-SR04 ultrasonic sensor watches a water tap. When someone comes within **40 cm**, it plays a spoken voice message once — *"Please don't waste water. Water is precious. Use water wisely."* No repeated nagging while the person lingers; it re-arms automatically once they step away.

## ⚙️ How it works

- **Audio playback** uses the ESP8266's internal **hardware sigma-delta modulator** (not `analogWrite()`, not I2S) — a single GPIO pin drives an amplifier and speaker directly.
- Voice is generated once offline (neural TTS), preprocessed (DC-block, band-limit, soft-limited gain, fade in/out) and baked into flash as a 16-bit PCM array — no SD card, no MP3 decoder needed.
- Distance is sampled with a 3-reading median filter to reject sensor noise.
- A hardware watchdog (`ESP.wdtEnable`) guarantees the unit self-recovers if it ever hangs — important for unattended, always-on deployment.

## 🔌 Hardware

| Component | Notes |
|---|---|
| ESP8266 NodeMCU | Any NodeMCU v2/v3 board |
| HC-SR04 ultrasonic sensor | 5V powered — **not** 3.3V |
| Class-D amplifier (e.g. PAM8403 / TPA class-D board) | Driven directly from a GPIO bitstream |
| 4–8 Ω speaker | |
| RC low-pass filter (2.2 kΩ + 4 nF, two stages recommended) | Removes ultrasonic carrier noise before the amplifier |

### Pinout

| Signal | GPIO | NodeMCU label |
|---|---|---|
| Audio out | GPIO5 | D1 |
| Ultrasonic TRIG | GPIO14 | D5 |
| Ultrasonic ECHO | GPIO12 | D6 |

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
```

## 🔊 Using your own voice message

Record or generate any short WAV/MP3, then process it to 16 kHz mono 16-bit PCM with DC-blocking and soft-limiting before regenerating `audio_data.h`. Keep clips short (2–6 seconds) to fit comfortably in flash alongside the sketch.

## 🛠️ Customization

- `DETECT_CM` — trigger distance in cm (default 40)
- Swap `audio_data.h` for a different message/language
- Add a cooldown timer if deploying somewhere with continuous foot traffic

## 📜 License

MIT — use it, fork it, deploy it anywhere water is being wasted.

---

⭐ If this helped you build something, consider starring the repo — it helps others find it too.
