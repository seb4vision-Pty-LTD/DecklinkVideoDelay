# Video Delay

Video delay takes in inputs from ports 1 and 3 and outputs a delayed signal on ports 
2 and 4 respectively.


## Accepted Inputs

1080i50 -> 1080i50


## Notes

> **Driver Required:** The application requires Decklink drivers to be installed — they are not bundled with the application.


> **Device Mode:** Decklink devices must be set to **Half-duplex** mode (`SDI1 → Device 1`), not full-duplex mode (`SDI1 → Device 1 & Device 2`).

## Build Instructions


> cmake -S . -B build

> cmake --build build --target Release

## Build Requirements

1. Qt6
2. Gstreamer 1.28
3. Microsoft visual c++ x86_amd64 compiler
4. Gstreamer plugins (good, bad, system) {All plugins bundled with gstreamer installer for windows}
