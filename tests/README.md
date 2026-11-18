# Firmware simulation checks

Run from the repository root:

```sh
node --test tests/ota.test.mjs
node tests/run-ota-stream.mjs
node tests/run-simulation.mjs 18 recovery
```

Requires Node.js and a C++17 compiler. Linux uses `g++` (override with `CXX`); Windows uses Visual Studio 2022 Community's x64 compiler (override its setup batch file with `VCVARS64`). Outputs go under the gitignored `build/simulation` directory.

The OTA stream test also requires ArduinoJson 6.21.6. On Windows it uses the installed Arduino library; elsewhere clone `https://github.com/bblanchon/ArduinoJson.git` at tag `v6.21.6` into `build/ArduinoJson`, or set `ARDUINOJSON_INCLUDE` to its `src` directory. It reproduces premature EOF between TLS fragments and tests the production reader against delayed, truncated, stalled and oversized responses, overall deadlines and clock rollover.

The runner assembles the production math, geometry, car simulation, race director, weather transition logic and championship headers into a host translation unit. Rendering, networking, clocks and device storage are replaced at their boundaries. Preferences use an in-memory store to check save/load behavior. It never reads local Wi-Fi or publishing credentials.

The first numeric argument selects the number of ordinary ten-lap races, across circuits, six classes, field sizes and dry/wet/fog conditions. Adding `recovery` runs 66 scenarios in which a qualifying front-runner starts last: two seeds per circuit in dry, drying, and becoming-wet conditions. Their car abilities are unchanged. The tests also exercise attack loops, pair cooldowns, pass logging, tyre tradeoffs, qualifying, retirements and persistence.

Pole conversion and comeback counts are diagnostics, not probabilities guaranteed by a small seeded sample. Host simulation does not measure ESP32 frame time, heap behavior, actual TLS connectivity or hardware OTA installation. The Arduino build and release checks cover compilation and OTA wiring separately; device performance still needs observation after installing.
