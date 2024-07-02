#include <TimeLib.h>
#include <Bounce2.h>

#include <cstring>
#include <climits>
#include <type_traits>

#include <InternalTemperature.h>

#include <EEPROM.h>
// TODO https://github.com/pictographer/didah/blob/master/EEPROMAnything.h
#include <Keyboard.h>

// The marvelous FastLED library provides a high-level interface to
// many kinds of RGB LEDs and drives them efficiently. See
// http://fastled.io
#include <FastLED.h>

#if defined(TEENSYDUINO)
#if defined(__MK20DX128__)
#define BOARD "Teensy 3.0"
#elif defined(__MK20DX256__)
#define BOARD "Teensy 3.1/3.2"
#elif defined(__MKL26Z64__)
#define BOARD "Teensy LC"
#elif defined(__MK64FX512__)
#define BOARD "Teensy 3.5"
#elif defined(__MK66FX1M0__)
#define BOARD "Teensy 3.6"
#elif defined(__IMXRT1062__)
define BOARD "Teensy 4.0/4.1"
// Can distinguish by testing ARDUINO_TEENSY41 and ARDUINO_TEENSY40
#else
#error "Unknown board"
#endif
#endif

// Desklight
const int PIR_INPUT_YELLOW = 2;

const int BUTTON_INPUT_WHITE = 9;
const int BUTTON_POWER_WHITE = 10;
const int DEBOUNCE_MS = 10;
Bounce momentaryButton = Bounce(BUTTON_INPUT_WHITE, DEBOUNCE_MS);

// Parameters for our LEDs
#define LED_TYPE APA102
#define COLOR_ORDER BGR
//const unsigned int NUM_LEDS = 96;
const unsigned int NUM_LEDS = 8;
const unsigned int CLOCK_MHZ = 12;

const int TOUCH_CASE_SENSOR_BLUE = 0;
const int TOUCH_CASE_MAX = 814;
const int TOUCH_CASE_MIN = 750;
const int DATA_PIN = 11;
const int CLOCK_PIN = 14;

// PBX81-1 photodiode reverse biased
const int PHOTODIODE_ANODE_BLACK = 16;
const int PHOTODIODE_CATHODE_RED = 17;
const int PHOTODIODE_MAX = 64000;
const int PHOTODIODE_MIN = 360;
const int PHOTODIODE_DAYLIGHT = 2300;
const int PHOTODIODE_THRESHOLD = 700;

const int SPEAKER_BLACK = 22;
const int SPEAKER_RED = 23;

// Storage for LED colors.
CRGB leds[NUM_LEDS];

const size_t KBDMAX = 64;
char kbdmacro[KBDMAX] = { 0 };

// Size of null-terminated command buffer.
const size_t cmax = 256;
static char cmdline[cmax];

elapsedMillis sincePIR;
static bool didBeep = false;

const long millisecondsPerSecond = 1000;
const long millisecondsPerMinute = 60 * millisecondsPerSecond;

void cmdSetColor(const char* cmdline) {
  fill_solid(leds, NUM_LEDS, colorNameToCode(cmdline));
  FastLED.show();
  Serial.println(cmdline);
}

void cmdReboot() {
  Serial.printf("Rebooting...\n");
  for (auto i = 2000; 1000 < i; i -= 100) {
    playTone(i, 10);
    delay(50);
  }
  _reboot_Teensyduino_();
}

// Expects i in [0, 900]. For this function, LEDs are numbered 100 to 800 with
// 100 at the bottom and 800 at the top. Values between centuries result in
// proportional illumnation for the two LEDs bracketing the value.
void antialias(int i) {
  auto low = i / 100U - 1U;
  auto high = low + 1U;
  auto highlevel = i - 100U * high;
  auto lowlevel = 100U - highlevel;
  highlevel *= 2U;
  lowlevel *= 2U;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  if (0U <= low && low < NUM_LEDS) {
    leds[low] = CRGB(lowlevel, lowlevel, lowlevel);
  }
  if (0U <= high && high < NUM_LEDS) {
    leds[high] = CRGB(highlevel, highlevel, highlevel);
  }
  FastLED.show();
}

void drop() {
  int s[] = { 1, 4, 9, 16, 25, 36, 49, 64 };
  static_assert(sizeof(s) / sizeof(s[0]) == NUM_LEDS, "Mismatch between s and NUM_LEDS");
  for (auto k = 0U; k < 1U; ++k) {
    for (auto i = 0U; i < NUM_LEDS; ++i) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      auto j = NUM_LEDS - i - 1U;
      leds[j] = CRGB::White;
      FastLED.delay(10U * s[j]);
    }
  }
}

void spin() {
  for (auto i = 0; i < 3; ++i) {
    for (auto j = NUM_LEDS - 1; j != (0U - 1U); --j) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      for (auto b = 0U; b < 256; b += 1) {
        auto k = (j + 1) % NUM_LEDS;
        auto b1 = 255 - b;
        auto b2 = b;
        leds[k] = CRGB(b1, b1, b1);
        leds[j] = CRGB(b2, b2, b2);
        FastLED.delay(b / 128);
      }
    }
  }
}

// Send Super-L to lock the screen on Ubuntu 20.
// Caution: Do not call this in a tight loop.
void lockScreen() {
  Keyboard.set_modifier(0);
  Keyboard.set_modifier(MODIFIERKEY_GUI);
  Keyboard.set_key1(uint8_t(KEY_L));
  Keyboard.send_now();
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.send_now();
}

// Tap the shift key to wake the computer up.
void tapShift() {
  Keyboard.set_modifier(0);
  Keyboard.set_modifier(MODIFIERKEY_SHIFT);
  Keyboard.send_now();
  delay(5);
  Keyboard.set_modifier(0);
  Keyboard.send_now();
  basicBeep();
}

void tapSuper() {
  Keyboard.set_modifier(0);
  Keyboard.set_modifier(MODIFIERKEY_GUI);
  Keyboard.send_now();
  delay(5);
  Keyboard.set_modifier(0);
  Keyboard.send_now();
  playTone(880, 20);
}

int readLight() {
  const auto count = 1024L;
  digitalWriteFast(PHOTODIODE_ANODE_BLACK, 1);
  auto sum = 0L;
  for (auto i = 0L; i < count; ++i) {
    sum += analogRead(PHOTODIODE_CATHODE_RED);
  }
  digitalWriteFast(PHOTODIODE_ANODE_BLACK, 0);
  return sum / count;
}

float readTemperatureF() {
  const auto count = 16L;
  auto sum = 0.0f;
  for (auto i = 0L; i < count; ++i) {
    sum += InternalTemperature.readUncalibratedTemperatureF();
  }
  return sum / count;
}

void rotate(int k) {
  auto c = leds[0];
  for (auto i = 0U; i < NUM_LEDS; ++i) {
    leds[i] = leds[(i + k) % NUM_LEDS];
  }
  leds[NUM_LEDS - 1] = c;
}

void upDownDot(unsigned count) {
  for (auto k = 0U; k < 3U; ++k) {
    for (auto i = 0U; i <= 900U; i += 1U) {
      antialias(i);
      FastLED.delay(1);
    }
    for (auto i = 100U; i <= 800U; i += 1U) {
      antialias(900U - i);
      FastLED.delay(1);
    }
  }
}

bool pollTouch() {
  const int threshold = TOUCH_CASE_MIN + (TOUCH_CASE_MAX - TOUCH_CASE_MIN) / 4;
  int touch_reading = touchRead(TOUCH_CASE_SENSOR_BLUE);
  return threshold < touch_reading;
}

bool pollButton() {
  digitalWriteFast(BUTTON_POWER_WHITE, 1);
  bool result = (momentaryButton.update() && momentaryButton.fallingEdge());
  digitalWriteFast(BUTTON_POWER_WHITE, 0);
  return result;
}

void playTone(int frequency_Hz, int duration_ms) {
  analogWriteFrequency(SPEAKER_RED, frequency_Hz);
  analogWrite(SPEAKER_RED, 127);
  delay(duration_ms);
  analogWrite(SPEAKER_RED, 0);
}

void basicBeep() {
  playTone(440, 100);
}

void clickBeep() {
  playTone(880, 2);
}

void flangeAscending() {
  for (auto i = 0; i < 100; ++i) {
    analogWriteFrequency(SPEAKER_RED, 200 + millis() % 1000);
    analogWrite(SPEAKER_RED, 127);
    delay(7);
  }
  analogWrite(SPEAKER_RED, 0);
}

/// Duration of a dit in microseconds
const uint32_t ditMicrosDefault = 50000;

/// Default transmission frequency in Hz.
const uint32_t defaultFrequency = 750;

/// A dit lasts one unit of time.
const int ditFactor = 1;

/// The dah duration is this factor times the dit duration.
const int dahFactor = 3;

/// The inter-letter gap is this factor times the dit duration.
const int letterGapFactor = 3;

/// The inter-word gap is this factor times the dit duration.
const int wordGapFactor = 7;

void blinkMorseDigit(uint8_t digit) {
  auto b = FastLED.getBrightness();
  const signed char b111110(0x3E);
  const signed char dend(digit - 5);
  for (signed char i(digit); i != dend; --i) {
    playTone(defaultFrequency,
             (ditMicrosDefault / 1000) * (b111110 >> i & 1 ? ditFactor : dahFactor));
    FastLED.setBrightness(0);
    FastLED.show();
    delayMicroseconds(ditMicrosDefault * (i - 1 == dend ? wordGapFactor : letterGapFactor));
    FastLED.setBrightness(b);
    FastLED.show();
  }
}

void cmdBeepMorseNumber(const char* cmdline) {
  auto i = 0;
  while (cmdline[i]) {
    if (isdigit(cmdline[i])) {
      blinkMorseDigit(cmdline[i] - '0');
    }
    ++i;
  }
  Serial.println("number");
}

// Read a line into cmdline. Complain and discard if longer than cmax - 1.
void pollCommand(char cmdline[], size_t cmax) {
  size_t cindex = 0;
  while (Serial.available()) {
    unsigned int r = Serial.read();
    if (!cindex && isspace(r)) continue;  // Discard leading spaces.
    if (isprint(r)) {
      cmdline[cindex++] = r;
      cmdline[cindex] = 0;
      if (cindex == cmax) {
        // The input is too long, thus invalid. Discard it.
        while (Serial.available()) Serial.read();
        Serial.println("Command buffer overflow!");
        basicBeep();
        break;
      }
    }
  }
  // Trim trailing spaces.
  while (cindex && isspace(cmdline[cindex - 1])) {
    --cindex;
  }
  cmdline[cindex] = 0;
}

bool isEqual(const char* a, const char* b) {
  return !strcmp(a, b);
}

// Is a a prefix of abc?
bool isPrefix(const char* a, const char* abc) {
  while (*a && *abc) {
    if (*a != *abc) {
      return false;
    }
    ++a;
    ++abc;
  }
  return *a == 0;
}

const struct {
  const CRGB colorCode;
  const char* colorName;
} colorName[] = {
  { CRGB::Black, "black" },
  { CRGB::Blue, "blue" },
  { CRGB::BlueViolet, "blueviolet" },
  { CRGB::Cyan, "cyan" },
  { CRGB::Gray, "gray" },
  { CRGB::Green, "green" },
  { CRGB::Magenta, "magenta" },
  { CRGB::Orange, "orange" },
  { CRGB::OrangeRed, "orangered" },
  { CRGB::Red, "red" },
  { CRGB::White, "white" },
  { CRGB::Yellow, "yellow" },
};
const size_t colorNameCount = sizeof colorName / sizeof colorName[0];

const char* colorToString(const CRGB color) {
  for (auto i = 0U; i < colorNameCount; ++i) {
    if (colorName[i].colorCode == color) return colorName[i].colorName;
  }
  return NULL;
}

const bool isColorName(const char* name) {
  auto result = false;
  for (auto i = 0U; i < colorNameCount; ++i) {
    if (isEqual(name, colorName[i].colorName)) {
      result = true;
      break;
    }
  }
  return result;
}

// Convert name to a CRGB code, or black if the string is not in our small table
const CRGB colorNameToCode(const char* name) {
  for (auto i = 0U; i < colorNameCount; ++i) {
    if (isEqual(name, colorName[i].colorName)) return colorName[i].colorCode;
  }
  return CRGB::Black;
}

void info() {
  Serial.printf("Version: green\n");
  Serial.printf("Serial connection time (ms): %d\n", millis());
  Serial.printf("Teensy board type: %s\n", BOARD);
  Serial.printf("sizeof(long) = %d; sizeof(int) = %d\n", sizeof(long), sizeof(int));
  // SIM_UIDMH is x55 on two Teensy LCs
  // SIM_UIDML is 0x262014 on my desk lamp, 0x2b2014 on a spare Teensy LC
  // SIM_UIDL ia x33974e45 on two Teensy LCs
  char id[36];
#if defined(__MKL26Z64__)
  sprintf(id, "%08lX-%08lX-%08lX", SIM_UIDMH, SIM_UIDML, SIM_UIDL);
#else
  sprintf(id, "%08lX-%08lX-%08lX-%08lX", SIM_UIDH, SIM_UIDMH, SIM_UIDML, SIM_UIDL);
#endif
  Serial.printf("CPU ID: %s\n", id);
  Serial.printf("CPU Frequency (MHz): %d\n", F_CPU / 1000000);
  Serial.printf("CPU Bus Frequency (MHz): %d\n", F_BUS / 1000000);
  Serial.printf("CPU Memory Frequency (MHz): %d\n", F_MEM / 1000000);
  Serial.printf("Build time: %s\n", __DATE__ " " __TIME__);
  Serial.printf("Arduino version: %d\n", ARDUINO);
  Serial.printf("Main source file: %s\n", __FILE__);
  Serial.printf("Touch Read: %d\n", touchRead(TOUCH_CASE_SENSOR_BLUE));
  Serial.printf("Light Read: %d\n", readLight());
  Serial.printf("Temperature (°F): ");
  Serial.println(readTemperatureF(), 1);
  Serial.printf("Temperature uncalibrated (°F): ");
  Serial.println(InternalTemperature.readUncalibratedTemperatureF(), 1);
  Serial.printf("Inactivity timer: %d\n", sincePIR / millisecondsPerMinute);
  clickBeep();
}

bool pollPIR() {
  bool sawMotion = digitalReadFast(PIR_INPUT_YELLOW);
  if (sawMotion) {
    //clickBeep();
  }

  if (sawMotion && 3 * millisecondsPerSecond < sincePIR) {
    Serial.printf("Inactivity timer: %d\n", (long)sincePIR);
    fill_rainbow(leds, NUM_LEDS, random(), 8);
    FastLED.show();
    sincePIR = 0;
  }
  return sawMotion;
}

// LC_RTC.h
// https://forum.pjrc.com/threads/37412-Working-Semi-accurate-quot-RTC-quot-for-Teensy-LC-from-internal-clock
/* Code to create a working, semi-accurate RTC in the Teensy LC
  My strategy:
  Make the RTC run off of the 1kHz LPO clock.
  This makes the RTC 1/32.768 * normal speed
  To fix this, we set the RTC prescaler to add 1 second on every 1k clock cycles
    instead of the normal 32768 cycles. Now the RTC is approx. normal speed.
  The prescaler would normally just rollover back to 32768 cycles, so we use
    an interrupt on every RTC second that resets the prescaler.
*/

#ifndef LC_RTC_h
#define LC_RTC_h


#if defined(KINETISL)

unsigned long LC_RTC_get() {
  return RTC_TSR;
}

//max 65535
const uint16_t COUNTS_PER_SEC = 31768 - 2; //32767;
// const uint16_t OLD_COUNTS_PER_SEC = 31768;

void LC_RTC_set(unsigned long t) {
  RTC_SR = 0;           // status register - disable RTC, only way to write other registers
  RTC_TPR = COUNTS_PER_SEC;      // prescaler register, 16bit
  RTC_TSR = t;          // inits the seconds
  RTC_SR = RTC_SR_TCE;  // status register again - enable RTC
}


void LC_RTC_enable() {
  // Enable write-access for RTC registers
  SIM_SCGC6 |= SIM_SCGC6_RTC;

  // Disable RTC so we can write registers
  RTC_SR = 0;

  // Set the prescaler to overflow in 1k cycles
  RTC_TPR = COUNTS_PER_SEC;

  // Disable the 32kHz oscillator
  RTC_CR = 0;

  // Set the RTC clock source to the 1kHz LPO
  SIM_SOPT1 = SIM_SOPT1 | SIM_SOPT1_OSC32KSEL(3);

  // Enable the interrupt for every RTC second
  RTC_IER |= 0x10;  // set the TSIE bit (Time Seconds Interrupt Enable)
  NVIC_ENABLE_IRQ(IRQ_RTC_SECOND);

  // Enable RTC
  RTC_SR = RTC_SR_TCE;
}

void rtc_seconds_isr() {
  // Disable RTC so we can write registers
  RTC_SR = 0;

  // Set the prescaler to overflow in 1k cycles
  RTC_TPR = COUNTS_PER_SEC;

  // Enable RTC
  RTC_SR = RTC_SR_TCE;
}
#endif
#endif
// EOF LC_RTC.h

void cmdAlarm() {
  Serial.println("alarm");
  auto brightness = FastLED.getBrightness();
  for (auto i(0); i < 3; ++i) {
    basicBeep();
    fill_solid(leds, NUM_LEDS, colorNameToCode("red"));
    FastLED.setBrightness(255);
    FastLED.show();
    delay(250);
    fill_solid(leds, NUM_LEDS, colorNameToCode("black"));
    FastLED.show();
  }
  FastLED.setBrightness(brightness);
  fill_solid(leds, NUM_LEDS, colorNameToCode("red"));
  FastLED.show();
}

void cmdTime() {
#if defined(KINETISL)
  Serial.printf("Seconds: %d\n", LC_RTC_get());
  time_t t = LC_RTC_get();
  Serial.printf("Date and time: %d/%d/%d %d:%02d:%02d\n",
                year(t), month(t), day(t), hour(t), minute(t), second(t));
#else
  Serial.printf("Not implemented\n");
#endif
}

// From bash, set the current local time by sending
//    time $(($EPOCHSECONDS - 7 * 60 * 60))
// to the Teensy. My time zone is GMT-7.
//
// Here's a UDEV rule that is run several times when a Teensy is plugged in
//
//    ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04*", RUN="/usr/local/bin/teensytime"
// 
// Here's the corresponding script
//
// #! /usr/bin/bash
//
// if [ "${ACTION}" = add -a "${REL}" = 1040 -a -d "/sys${DEVPATH}" ]; then
//     # Return the timezone offset, e.g. -700.
//     z=$(date +%-z)
//     # Lop off the last two digits yielding offset in hours, e.g. -7.
//     z=${z::-2}
//     # Tell the Teensy what the local time is.
//     devcmd time $(($(date +%s) + $z * 60 * 60))
// fi
//
void cmdSetTime(const char* timestr) {
  unsigned long s = 0;
  int count = sscanf(timestr, "%lu", &s);
  if (count == 1) {
    Serial.printf("Setting time to %lu\n", s);
    LC_RTC_set(s);
  }
}

void cmdBeep(const char* cmdline) {
  unsigned int Hz, ms;
  int count = sscanf(cmdline, "%u %u", &Hz, &ms);
  if (count == 2) {
    Serial.printf("beep frequency: %u Hz; duration: %u ms\n", Hz, ms);
    playTone(Hz, ms);
  } else {
    Serial.printf("Expecting frequency in Hz and duration in milliseconds.\n");
  }
}

// Configure USB serial I/O, GPIO, and temperature sensing.
void configureTeensy() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000)
    ;
  pinMode(BUTTON_INPUT_WHITE, INPUT_PULLDOWN);
  pinMode(BUTTON_POWER_WHITE, OUTPUT);

  pinMode(PIR_INPUT_YELLOW, INPUT_PULLDOWN);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SPEAKER_BLACK, OUTPUT);
  pinMode(SPEAKER_RED, OUTPUT);
  digitalWriteFast(SPEAKER_BLACK, 0);
  digitalWriteFast(SPEAKER_RED, 0);
  pinMode(PHOTODIODE_CATHODE_RED, INPUT_PULLDOWN);
  pinMode(PHOTODIODE_ANODE_BLACK, OUTPUT);
  digitalWriteFast(PHOTODIODE_ANODE_BLACK, 0);

#if defined(KINETISL)
  LC_RTC_enable();
  LC_RTC_set(0);
#endif
  InternalTemperature.begin(/* TEMPERATURE_NO_ADC_SETTING_CHANGES */);
  {
    //  bool isOkay = InternalTemperature.singlePointCalibrationF(80.0f, 80.0f);
    bool isOkay = InternalTemperature.dualPointCalibrationC(73.9f, 74.6f,
                                                            80.0f, 84.0f);
    if (!isOkay && Serial) {
      Serial.printf("Temperature calibration failed.\n");
    }
  }
}

void cmdHelp() {
  Serial.printf("%s",
                "alarm - beep and flash red three times\n"
                "aa - antialiased animation\n"
                "reboot - reboot\n"
                "spin - spin\n"
                "zorp - flange ascending\n"
                "super - tap the super key\n"
                "time - LC_RTC_get or if given seconds since the epoch, sets the time.\n"
                "beep freq ms - play a tone at the given frequency for the given milliseconds\n"
                "info - configuration and state information\n"
                "macro text - store text for later playback\n"
                "pir - report time since pir activity detected\n"
                "rgb r g b - set all the lights to an RGB value\n"
                "ramp - white gradient\n"
                "dither on/off - set the FastLED dither mode\n"
                "rainbow - animate a rainbow\n"
                "rotate - rotate the light pattern\n"
                "brightness b - test the brightness\n"
                "light - report the light level\n"
                "temperature - report the temperature\n"
                "colors - current state of the LEDs\n"
                "touch - report the touch reading\n"
                "number n - beep the number in Morse code\n");
  for (auto i = 0U; i < colorNameCount; ++i) {
    Serial.printf("%s - set all LEDs to %s\n", colorName[i].colorName, colorName[i].colorName);
  }
}

void cmdMacro(const char* cmdline) {
  strncpy(kbdmacro, cmdline, KBDMAX);
  kbdmacro[KBDMAX - 1] = '\0';
  Serial.printf("Got macro '%s'.\n", kbdmacro);
}

void cmdRgb(const char* cmdline) {
  unsigned int r, g, b;
  int count = sscanf(cmdline, "%u %u %u", &r, &g, &b);
  if (count != 3) {
    Serial.printf("Expecting red, green, and blue values in the range 0..255.\n");
  }
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  FastLED.show();
}

void cmdDither(const char* cmdline) {
  bool isEnabled = strstr(cmdline, "on") || strstr(cmdline, "1");
  bool isDisabled = strstr(cmdline, "off") || strstr(cmdline, "0");
  if (isEnabled != isDisabled) {
    FastLED.setDither(isEnabled);
    Serial.printf("Set dither % s\n", isEnabled ? "on" : "off");
  } else {
    Serial.printf("Expecting dither setting of 'on' or 'off'\n");
  }
}

void cmdRainbow() {
  int hue_span = 128 / NUM_LEDS;
  if (hue_span == 0) hue_span = 1;
  for (auto i = 0U; i < (128 + 32) * 6; ++i) {
    fill_rainbow(leds, NUM_LEDS, i, hue_span);
    FastLED.delay(10);
  }
}

void cmdBrightness(const char* cmdline) {
  const auto level = atoi(cmdline);
  const auto arglen = strlen(cmdline);
  if (0 < arglen && arglen < 4 && level < 256) {
    FastLED.setBrightness(level);
  } else if (arglen) {
    Serial.printf("Expecting brightness from 0..255. Got '%s'.\n", cmdline);
  }
  Serial.printf("Brightness: % u\n", (unsigned int)FastLED.getBrightness());
}

void cmdTemperature() {
  Serial.print("Temperature (°F): ");
  Serial.println(readTemperatureF(), 1);
  Serial.print("Uncalibrated: ");
  Serial.println(InternalTemperature.readUncalibratedTemperatureF(), 1);
}

void cmdListColors() {
  for (auto i = 0U; i < NUM_LEDS; ++i) {
    // TODO This command seems to cause the Teensy to hang intermittently.
    auto name = colorToString(leds[i]);
    if (!name) name = "";
    Serial.printf("% 2u: rgb( % 3u, % 3u, % 3u) %s\n", i, leds[i].red, leds[i].green, leds[i].blue, name);
  }
}

void setup() {
  configureTeensy();

  // And now the star of the show. Initialize the LEDs.
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN,
                  COLOR_ORDER, DATA_RATE_MHZ(CLOCK_MHZ)>(leds, NUM_LEDS);

  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.setBrightness(8);
  FastLED.show();

  if (Serial) {
    info();
  }
}

void loop() {
  if (pollButton()) {
    sincePIR = 0;
    didBeep = false;
    if (strlen(kbdmacro)) {
      Keyboard.print(kbdmacro);
    } else {
      Serial.printf("Enter 'macro <text>' to assign a macro.\n");
    }
    basicBeep();
  } else if (pollTouch()) {
    sincePIR = 0;
    didBeep = false;
    fill_rainbow(leds, NUM_LEDS, millis() >> 3, 8);
    FastLED.delay(4);

    // tapSuper();
    Serial.printf("Touch case: % u\n", touchRead(TOUCH_CASE_SENSOR_BLUE));
  }

  if (pollPIR()) {
    // Motion was detected. The detector goes to sleep for 2.5 seconds after a detection.
    didBeep = false;
  }

  if (29 * millisecondsPerMinute < sincePIR) {
    // 1 minute until locking the screen; warn the user.
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    if (!didBeep) {
      playTone(2000, 10);
      didBeep = true;
    }
  }

  // We need to re-think this. I sit still while programming and reading. Doh!
  //  if (30 * millisecondsPerMinute < sincePIR) {
  //    sincePIR = 0;
  //    didBeep = false;
  //    // Lock the screen; it's been 30 minutes since the PIR sensor triggered.
  //    delay(1000); // Delay a second just in case.
  //    lockScreen();
  //  }

  auto light_level = readLight();
  if (light_level < PHOTODIODE_MIN) {
    Serial.printf("Light = %d\n", light_level);
    delay(1000);
  }

  // Handle USB-serial commands.

  pollCommand(cmdline, cmax);
  if (cmdline[0]) {
    if (isColorName(cmdline)) {
      cmdSetColor(cmdline);
    } else if (isEqual("aa", cmdline)) {
      upDownDot(3);
    } else if (isEqual("reboot", cmdline)) {
      cmdReboot();
    } else if (isEqual("spin", cmdline)) {
      spin();
    } else if (isEqual("zorp", cmdline)) {
      flangeAscending();
    } else if (isEqual("super", cmdline)) {
      tapSuper();
    } else if (isEqual("time", cmdline)) {
      cmdTime();
    } else if (isPrefix("time", cmdline)) {
      cmdSetTime(cmdline + sizeof "time");
    } else if (isPrefix("beep", cmdline)) {
      cmdBeep(cmdline + sizeof "beep");
    } else if (isEqual("help", cmdline)) {
      cmdHelp();
    } else if (isEqual("info", cmdline)) {
      info();
    } else if (isPrefix("macro", cmdline)) {
      cmdMacro(cmdline + sizeof "macro");
    } else if (isEqual("pir", cmdline)) {
      Serial.printf("Time since PIR (seconds): % ld\n", sincePIR / 1000);
    } else if (isPrefix("rgb", cmdline)) {
      cmdRgb(cmdline + sizeof "rgb");
    } else if (isEqual(cmdline, "ramp")) {
      fill_gradient_RGB(leds, 0, CRGB::White, NUM_LEDS - 1, CRGB(1, 1, 1));
      FastLED.show();
    } else if (isPrefix("dither", cmdline)) {
      cmdDither(cmdline + sizeof "dither");
    } else if (isEqual(cmdline, "rainbow")) {
      cmdRainbow();
    } else if (isEqual(cmdline, "rotate")) {
      rotate(1);
      Serial.println("rotate");
    } else if (isPrefix("brightness", cmdline)) {
      cmdBrightness(cmdline + sizeof "brightness");
    } else if (isEqual(cmdline, "light")) {
      Serial.printf("Light: % d\n", readLight());
    } else if (isEqual(cmdline, "temperature")) {
      cmdTemperature();
    } else if (isEqual(cmdline, "colors")) {
      cmdListColors();
    } else if (isEqual(cmdline, "touch")) {
      Serial.printf("Touch case: % u\n", touchRead(TOUCH_CASE_SENSOR_BLUE));
    } else if (isEqual(cmdline, "alarm")) {
      cmdAlarm();
    } else if (isPrefix("number", cmdline)) {
      cmdBeepMorseNumber(cmdline + sizeof "number");
    }

    FastLED.show();
    cmdline[0] = 0;
    memset(cmdline, 0, cmax);
  }
}
