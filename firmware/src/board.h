// The Waveshare ESP32-S3-Touch-AMOLED-1.8 and what the case adds to it: the AMOLED, its touch layer, the
// power chip (battery, charger, power key), the speaker, the red key (BOOT, GPIO0) and the dial (an EC11
// encoder on GPIO17 and GPIO18).
#pragma once
#include <stdint.h>

namespace board {

// pins (Waveshare pin_config.h, and the two pads the dial is soldered to)
constexpr int LCD_SDIO0 = 4, LCD_SDIO1 = 5, LCD_SDIO2 = 6, LCD_SDIO3 = 7, LCD_SCLK = 11, LCD_CS = 12;
constexpr int I2C_SDA = 15, I2C_SCL = 14, TP_INT = 21;
constexpr int I2S_MCK = 16, I2S_BCK = 9, I2S_WS = 45, I2S_DO = 8, I2S_DI = 10, PA = 46;
constexpr int KEY_RED = 0, ENC_A = 17, ENC_B = 18;

bool begin();                      // false: the display or the power chip did not answer
void push(const uint16_t *fb);     // the whole 368 x 448 frame
void brightness(int v);            // 0..255, 0 turns the panel off
uint16_t *framebuffer();

// inputs, each read once per loop
int dialSteps();                   // detents turned since the last call, clockwise positive
bool redDown();

struct Touch { bool down; int x, y; };
Touch touch();

// power
struct Battery { int percent; bool charging, usb; };
Battery battery();
enum PowerKey { PK_NONE, PK_SHORT, PK_LONG };
PowerKey powerKey();
void shutdown();                   // everything off; the power key turns it on again
void bootloader();                 // restart into the ROM's USB download mode

void beep(int hz = 2400, int ms = 35);

}  // namespace board
