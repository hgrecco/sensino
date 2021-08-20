/**
 * This file is part of the sensino library.
 *
 */
#pragma once

#include <U8g2lib.h>
#include <U8x8lib.h>

namespace sensino {

/**
 * Abstract class over a U8G2 compatible screen.
 *
 */
class Screen {

public:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2 =
      U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

  Screen() {}

  void setup() {
    this->u8g2.begin();
    this->u8g2.clearBuffer();
  }

  // clear the screen
  void clear() {
    this->u8g2.clearBuffer();
    this->u8g2.sendBuffer();
  }

  // Put a large text and a number below. Useful for countdowns.
  void title_number(const char *title, int number, const char *suffix,
                    bool clear = true, bool send = true) {
    static char str[20];
    snprintf(str, 20, "%2d %s", number, suffix);
    this->rows2(title, str, clear, send);
  }

  // Text in two rows.
  void rows2(const char *row1, const char *row2, bool clear = true,
             bool send = true, int offset = 0) {
    if (clear) {
      this->u8g2.clearBuffer();
      this->u8g2.setFont(u8g2_font_crox4tb_tf);
    }

    if (row1 != nullptr) {
      this->u8g2.drawStr(offset, 24, row1);
    }
    if (row2 != nullptr) {
      this->u8g2.drawStr(offset, 48, row2);
    }

    if (send) {
      this->u8g2.sendBuffer();
    }
  }

  // Text in three rows.
  void rows3(const char *row1, const char *row2, const char *row3,
             bool clear = true, bool send = true, int offset = 0) {
    if (clear) {
      this->u8g2.clearBuffer();
      this->u8g2.setFont(u8g2_font_crox3tb_tf);
    }

    if (row1 != nullptr) {
      this->u8g2.drawStr(offset, 20 * 1, row1);
    }
    if (row2 != nullptr) {
      this->u8g2.drawStr(offset, 20 * 2, row2);
    }
    if (row3 != nullptr) {
      this->u8g2.drawStr(offset, 20 * 3, row3);
    }

    if (send) {
      this->u8g2.sendBuffer();
    }
  }

  // Text in three rows with scrolling.
  void rows3_scroll(const char *row1, const char *row2, const char *row3,
                    unsigned int period = 50) {

    for (unsigned int offset = 0; offset <= 100; offset++) {
      this->rows3(row1, row2, row3, true, true, -offset);
      delay(period);
    }
  }
};

} // namespace sensino
