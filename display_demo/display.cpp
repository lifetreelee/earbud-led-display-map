#include "display.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "segments_table.h"

namespace {

const uint8_t LINE_GPIO[8] = {1, 2, 3, 4, 5, 6, 7, 10};  // line n -> LINE_GPIO[n-1]
const uint8_t ANODES[] = {2, 3, 4, 5, 6, 7, 8};         // line 1 is never an anode
const uint8_t NANODES = sizeof(ANODES);
const int NSEG = sizeof(SEGMENTS) / sizeof(SEGMENTS[0]);

bool segOn[NSEG];
volatile uint8_t groupMask[9];  // indexed by anode line; bit i-1 = line i LOW
volatile uint8_t slot;
esp_timer_handle_t scanTimer;

// a..g -> bit 0..6
const uint8_t FONT[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

int find(const char* name) {
  for (int i = 0; i < NSEG; i++)
    if (!strcmp(SEGMENTS[i].name, name)) return i;
  return -1;
}

inline void idleLines() {
  for (uint8_t p : LINE_GPIO) gpio_set_direction((gpio_num_t)p, GPIO_MODE_INPUT);
}

inline void drive(uint8_t line, int level) {
  gpio_num_t p = (gpio_num_t)LINE_GPIO[line - 1];
  gpio_set_level(p, level);  // level first so the pin never glitches on enable
  gpio_set_direction(p, GPIO_MODE_OUTPUT);
}

// One anode slot per tick. Fixed slot time keeps brightness independent of content.
void scanTick(void*) {
  idleLines();
  uint8_t a = ANODES[slot];
  slot = (slot + 1) % NANODES;
  uint8_t m = groupMask[a];
  if (!m) return;
  for (uint8_t i = 1; i <= 8; i++)
    if (m & (1 << (i - 1))) drive(i, 0);
  drive(a, 1);
}

void recompute(uint8_t anode) {
  uint8_t m = 0;
  for (int i = 0; i < NSEG; i++)
    if (segOn[i] && SEGMENTS[i].anode == anode) m |= SEGMENTS[i].cathMask;
  groupMask[anode] = m;
}

}  // namespace

namespace display {

void begin(uint32_t slotUs) {
  for (uint8_t p : LINE_GPIO) {
    gpio_set_pull_mode((gpio_num_t)p, GPIO_FLOATING);
    gpio_set_drive_capability((gpio_num_t)p, GPIO_DRIVE_CAP_0);  // ~5 mA
    gpio_set_direction((gpio_num_t)p, GPIO_MODE_INPUT);
  }
  esp_timer_create_args_t args = {};
  args.callback = scanTick;
  args.name = "display_scan";
  esp_timer_create(&args, &scanTimer);
  esp_timer_start_periodic(scanTimer, slotUs);
}

bool setSegment(const char* name, bool on) {
  int i = find(name);
  if (i < 0) return false;
  segOn[i] = on;
  recompute(SEGMENTS[i].anode);
  return true;
}

void clear() {
  memset(segOn, 0, sizeof(segOn));
  memset((void*)groupMask, 0, sizeof(groupMask));
}

void setDigits(int value) {
  if (value < 0) value = 0;
  if (value > 99) value = 99;
  const char* names[2] = {"tens", "ones"};
  int digits[2] = {value / 10, value % 10};
  char buf[16];
  for (int d = 0; d < 2; d++)
    for (int s = 0; s < 7; s++) {
      snprintf(buf, sizeof(buf), "%s_%c", names[d], 'a' + s);
      setSegment(buf, FONT[digits[d]] & (1 << s));
    }
}

void setBars(char side, int n) {
  char buf[16];
  for (int b = 1; b <= 5; b++) {
    snprintf(buf, sizeof(buf), "%c_bar%d", side, b);
    setSegment(buf, b <= n);
  }
}

}  // namespace display
