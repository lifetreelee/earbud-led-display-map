// Demo: count 00-99 with the batteries tracking the value, sweep the bars,
// then walk every segment once (names go to Serial). Non-blocking, millis() driven.
#include "display.h"

enum Phase { COUNT, SWEEP, WALK };
Phase phase = COUNT;
uint32_t lastStep = 0;
int step = 0;

const char* const WALK_NAMES[] = {
  "R_outline", "L_outline", "letter_R", "letter_L", "pct", "hundreds_1", "hundreds_2",
  "ones_a", "ones_b", "ones_c", "ones_d", "ones_e", "ones_f", "ones_g",
  "tens_a", "tens_b", "tens_c", "tens_d", "tens_e", "tens_f", "tens_g"};
const int NWALK = sizeof(WALK_NAMES) / sizeof(WALK_NAMES[0]);

void enter(Phase p) {
  phase = p;
  step = 0;
  lastStep = 0;
  display::clear();
  Serial.printf("phase %d\n", (int)p);
}

void setup() {
  Serial.begin(115200);
  display::begin();
  enter(COUNT);
}

void loop() {
  uint32_t now = millis();
  switch (phase) {
    case COUNT:
      if (now - lastStep >= 120) {
        lastStep = now;
        display::setDigits(step);
        display::setBars('R', step / 20);
        display::setBars('L', 5 - step / 20);
        display::setSegment("R_outline", step % 2);
        if (++step > 99) enter(SWEEP);
      }
      break;
    case SWEEP:  // fill R while draining L, three times
      if (now - lastStep >= 250) {
        lastStep = now;
        int k = step % 6;
        display::setBars('R', k);
        display::setBars('L', 5 - k);
        display::setSegment("letter_R", true);
        display::setSegment("letter_L", true);
        display::setSegment("pct", true);
        display::setSegment("L_outline", true);
        if (++step >= 18) enter(WALK);
      }
      break;
    case WALK:
      if (now - lastStep >= 700) {
        lastStep = now;
        if (step > 0) display::setSegment(WALK_NAMES[step - 1], false);
        if (step < NWALK) {
          display::setSegment(WALK_NAMES[step], true);
          Serial.println(WALK_NAMES[step]);
        }
        if (++step > NWALK) enter(COUNT);
      }
      break;
  }
}
