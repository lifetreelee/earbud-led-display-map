// Charlieplexed LED display driver (8 lines, GPIO1-7 + GPIO10 via 1k each).
// setSegment() only edits a frame buffer; a periodic esp_timer scans it in the
// background, so callers never block.
#pragma once
#include <Arduino.h>

namespace display {

// Start the scan. slotUs is the time per anode slot; 7 slots per frame, so each
// group refreshes at 1e6 / (7 * slotUs) Hz (default 1000 us -> ~143 Hz).
void begin(uint32_t slotUs = 1000);

// Switch a named segment (see segments.json). Returns false for an unknown name.
bool setSegment(const char* name, bool on);

void clear();

// Show 0-99 on the two digits (leading zero kept). Hundreds stub is left alone.
void setDigits(int value);

// Light the first n bars (0-5, counted from the nub) of the 'L' or 'R' battery.
void setBars(char side, int n);

}  // namespace display
