// Line-probe firmware. Serial commands (one per line):
//   H n     line n (1-8) HIGH, others INPUT
//   P a b   line a HIGH, line b LOW, others INPUT
//   A n     line n HIGH, all other lines LOW
//   M a m   line a HIGH; lines whose bit is set in mask m (bit i-1 = line i) LOW; others INPUT
//   I       all INPUT
// Replies "OK" after each command.
static const int NLINES = 8;
// lines 1-7 -> GPIO1-7; line 8 = orange wire -> GPIO10 (1k series)
static const uint8_t PINS[NLINES] = {1, 2, 3, 4, 5, 6, 7, 10};

static void idleAll() {
  for (uint8_t p : PINS) pinMode(p, INPUT);
}

static void drive(int line, int level) {
  uint8_t p = PINS[line - 1];
  pinMode(p, OUTPUT);
  gpio_set_drive_capability((gpio_num_t)p, GPIO_DRIVE_CAP_0);
  digitalWrite(p, level);
}

void setup() {
  Serial.begin(115200);
  idleAll();
}

void loop() {
  if (!Serial.available()) return;
  String s = Serial.readStringUntil('\n');
  s.trim();
  if (!s.length()) return;
  char c = s[0];
  int a = 0, b = 0;
  idleAll();
  if (c == 'H' && sscanf(s.c_str() + 1, "%d", &a) == 1 && a >= 1 && a <= NLINES) {
    drive(a, HIGH);
  } else if (c == 'P' && sscanf(s.c_str() + 1, "%d %d", &a, &b) == 2 &&
             a >= 1 && a <= NLINES && b >= 1 && b <= NLINES && a != b) {
    drive(b, LOW);
    drive(a, HIGH);
  } else if (c == 'A' && sscanf(s.c_str() + 1, "%d", &a) == 1 && a >= 1 && a <= NLINES) {
    for (int i = 1; i <= NLINES; i++) if (i != a) drive(i, LOW);
    drive(a, HIGH);
  } else if (c == 'M' && sscanf(s.c_str() + 1, "%d %d", &a, &b) == 2 && a >= 1 && a <= NLINES) {
    for (int i = 1; i <= NLINES; i++) if (i != a && (b >> (i - 1)) & 1) drive(i, LOW);
    drive(a, HIGH);
  } else if (c != 'I') {
    Serial.println("ERR");
    return;
  }
  Serial.println("OK");
}
