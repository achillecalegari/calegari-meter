#include "board.h"
#include <Adafruit_XCA9554.h>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <ESP_I2S.h>
#include <Wire.h>
#include <XPowersLib.h>
#include <soc/rtc_cntl_reg.h>
#include "config.h"
#include "gfx.h"
extern "C" {
#include "es8311.h"
}

namespace board {

static Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
static Arduino_SH8601 *panel = new Arduino_SH8601(bus, GFX_NOT_DEFINED, CFG_ROTATE_180 ? 2 : 0, W, H);
static Adafruit_XCA9554 expander;
static XPowersPMU pmu;
static I2SClass i2s;
static bool audio = false;
static uint16_t *fb = nullptr;

// ------------------------------------------------------------------------------------------ dial

static volatile int32_t encCount = 0;
static volatile uint8_t encState = 0;
static int32_t encTaken = 0;

// a quadrature step table: previous state (2 bits) and new state (2 bits) -> -1, 0, +1
static const int8_t QDEC[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static void IRAM_ATTR encIsr() {
  uint8_t s = (uint8_t)((digitalRead(ENC_A) << 1) | digitalRead(ENC_B));
  encCount += QDEC[(encState << 2) | s];
  encState = s;
}

int dialSteps() {
  int32_t c = encCount;
  int steps = (c - encTaken) / CFG_ENC_PULSES;   // an EC11 makes four edges per detent
  encTaken += steps * CFG_ENC_PULSES;
  return CFG_ENC_REVERSE ? -steps : steps;
}

bool redDown() { return digitalRead(KEY_RED) == LOW; }

// ------------------------------------------------------------------------------------------ touch
// The FT3168 answers at 0x38: register 2 holds the number of fingers, 3..6 the first finger's x and y.

Touch touch() {
  Touch t{false, 0, 0};
  if (digitalRead(TP_INT) == HIGH) return t;
  Wire.beginTransmission(0x38);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return t;
  if (Wire.requestFrom(0x38, 5) != 5) return t;
  uint8_t n = Wire.read() & 0x0F, xh = Wire.read(), xl = Wire.read(), yh = Wire.read(), yl = Wire.read();
  if (n == 0 || n > 2) return t;
  t.down = true;
  t.x = ((xh & 0x0F) << 8) | xl;
  t.y = ((yh & 0x0F) << 8) | yl;
  if (CFG_ROTATE_180) { t.x = W - 1 - t.x; t.y = H - 1 - t.y; }
  return t;
}

// ------------------------------------------------------------------------------------------ power

Battery battery() {
  Battery b;
  b.usb = pmu.isVbusIn();
  b.charging = pmu.isCharging();
  b.percent = pmu.isBatteryConnect() ? pmu.getBatteryPercent() : 100;
  if (b.percent < 0) b.percent = 0;
  return b;
}

PowerKey powerKey() {
  static uint32_t last = 0;
  if (millis() - last < 40) return PK_NONE;
  last = millis();
  pmu.getIrqStatus();
  PowerKey k = PK_NONE;
  if (pmu.isPekeyShortPressIrq()) k = PK_SHORT;
  if (pmu.isPekeyLongPressIrq()) k = PK_LONG;
  pmu.clearIrqStatus();
  return k;
}

void shutdown() {
  panel->setBrightness(0);
  panel->displayOff();
  delay(20);
  pmu.shutdown();
  for (;;) delay(100);   // on USB the chip keeps charging; the ESP32 has no power by now
}

void bootloader() {
  panel->setBrightness(0);
  REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
  esp_restart();
}

// ------------------------------------------------------------------------------------------ sound

static bool audioBegin() {
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);
  i2s.setPins(I2S_BCK, I2S_WS, I2S_DO, I2S_DI, I2S_MCK);
  if (!i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) return false;
  es8311_handle_t es = es8311_create(0, ES8311_ADDRRES_0);
  if (!es) return false;
  const es8311_clock_config_t clk = {false, false, true, 16000 * 256, 16000};
  if (es8311_init(es, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK) return false;
  es8311_voice_volume_set(es, 70, nullptr);
  return true;
}

void beep(int hz, int ms) {
  if (!audio) return;
  static int16_t buf[2 * 16000 / 10];   // up to 100 ms, stereo
  int n = 16000 * ms / 1000;
  if (n > 1600) n = 1600;
  for (int i = 0; i < n; i++) {
    float env = fminf(1.0f, fminf(i, n - i) / 80.0f);   // 5 ms in and out, no click
    int16_t v = (int16_t)(9000 * env * sinf(6.2831853f * hz * i / 16000.0f));
    buf[2 * i] = buf[2 * i + 1] = v;
  }
  digitalWrite(PA, HIGH);
  i2s.write((uint8_t *)buf, n * 4);
  delay(ms + 5);
  digitalWrite(PA, LOW);
}

// ------------------------------------------------------------------------------------------ display

uint16_t *framebuffer() { return fb; }

void push(const uint16_t *f) { panel->draw16bitRGBBitmap(0, 0, (uint16_t *)f, W, H); }

void brightness(int v) {
  static int last = -1;
  v = v < 0 ? 0 : (v > 255 ? 255 : v);
  if (v == last) return;
  last = v;
  panel->setBrightness((uint8_t)v);
}

bool begin() {
  pinMode(KEY_RED, INPUT_PULLUP);
  pinMode(TP_INT, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  encState = (uint8_t)((digitalRead(ENC_A) << 1) | digitalRead(ENC_B));
  attachInterrupt(ENC_A, encIsr, CHANGE);
  attachInterrupt(ENC_B, encIsr, CHANGE);

  Wire.begin(I2C_SDA, I2C_SCL, 400000);

  // the expander holds the display and touch in reset: a short low pulse on its first three pins
  if (expander.begin(0x20, &Wire)) {
    for (int p = 0; p < 3; p++) { expander.pinMode(p, OUTPUT); expander.digitalWrite(p, LOW); }
    delay(20);
    for (int p = 0; p < 3; p++) expander.digitalWrite(p, HIGH);
    delay(50);
  }

  bool ok = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
  if (ok) {
    pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    pmu.clearIrqStatus();
    pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ);
    pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    pmu.setIrqLevelTime(XPOWERS_AXP2101_IRQ_TIME_1S5);   // held this long: the long-press IRQ
    pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_6S);   // the chip's own fallback if the firmware hangs
    pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_300MA);
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
  }

  fb = (uint16_t *)ps_malloc(W * H * 2);
  if (!fb) return false;
  if (!panel->begin(80000000)) return false;
  panel->fillScreen(0);
  panel->setBrightness(0);

  audio = audioBegin();
  return ok;
}

}  // namespace board
