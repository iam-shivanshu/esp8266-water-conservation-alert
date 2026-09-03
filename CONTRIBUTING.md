# Contributing

Pull requests are welcome — bug fixes, new features, ports to other boards, translated voice messages, whatever's useful.

## Before opening a PR

- Test on real hardware if you're touching `esp8266_water.ino` — this project relies on cycle-accurate timing for audio playback (the ESP8266 hardware sigma-delta output), so a change that compiles cleanly can still sound wrong or break sensor timing.
- If you change the audio pipeline in `convert_audio.py`, mention what changed and why in the PR description — loudness/quality tradeoffs (drive, peak, clipping) aren't always obvious from a diff.
- Keep `DETECT_CM`, pin assignments, and the watchdog timeout as configurable `#define`s rather than hardcoding new values inline.

## Reporting issues

Include your board (NodeMCU version), amplifier/speaker setup, and what you observed vs. expected — audio issues especially depend a lot on the specific hardware chain.

## Ideas for contributions

- Support for other ESP8266/ESP32 board variants
- A cooldown timer option for high-traffic deployments
- Additional language/voice message examples
- Battery-powered / solar variant notes
