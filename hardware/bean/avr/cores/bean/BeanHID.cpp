

/* Copyright (c) 2011, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include <Arduino.h>
#include "BeanHID.h"

// Singletons for BeanKeyboard and BeanMouse

BeanKeyboard_ BeanKeyboard;
BeanMouse_ BeanMouse;

#define HID_DEV_DATA_LEN 8

// HID keyboard input report length
#define HID_KEYBOARD_IN_RPT_LEN 8
#define HID_MOUSE_IN_RPT_LEN 4

typedef struct {
  uint8_t id;
  uint8_t type;
  uint8_t len;
  uint8_t data[HID_DEV_DATA_LEN];
} hidDevReport_t;

//==============================================================================
//==============================================================================

// HID report descriptor

#define LSB(_x) ((_x)&0xFF)
#define MSB(_x) ((_x) >> 8)

#define RAWHID_USAGE_PAGE 0xFFC0
#define RAWHID_USAGE 0x0C00
#define RAWHID_TX_SIZE 64
#define RAWHID_RX_SIZE 64

extern const uint8_t _hidReportDescriptor[] PROGMEM;
const uint8_t _hidReportDescriptor[] = {
    // BeanMouse
    0x05, 0x01,  // USAGE_PAGE (Generic Desktop) // 54
    0x09, 0x02,  // USAGE (BeanMouse)
    0xa1, 0x01,  // COLLECTION (Application)
    0x09, 0x01,  //   USAGE (Pointer)
    0xa1, 0x00,  //   COLLECTION (Physical)
    0x85, 0x01,  //     REPORT_ID (1)
    0x05, 0x09,  //     USAGE_PAGE (Button)
    0x19, 0x01,  //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,  //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,  //     LOGICAL_MINIMUM (0)
    0x25, 0x01,  //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,  //     REPORT_COUNT (3)
    0x75, 0x01,  //     REPORT_SIZE (1)
    0x81, 0x02,  //     INPUT (Data,Var,Abs)
    0x95, 0x01,  //     REPORT_COUNT (1)
    0x75, 0x05,  //     REPORT_SIZE (5)
    0x81, 0x03,  //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,  //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,  //     USAGE (X)
    0x09, 0x31,  //     USAGE (Y)
    0x09, 0x38,  //     USAGE (Wheel)
    0x15, 0x81,  //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,  //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,  //     REPORT_SIZE (8)
    0x95, 0x03,  //     REPORT_COUNT (3)
    0x81, 0x06,  //     INPUT (Data,Var,Rel)
    0xc0,        //   END_COLLECTION
    0xc0,        // END_COLLECTION

    // Keyboard
    0x05, 0x01,  // USAGE_PAGE (Generic Desktop) // 47
    0x09, 0x06,  // USAGE (Keyboard)
    0xa1, 0x01,  // COLLECTION (Application)
    0x85, 0x02,  //   REPORT_ID (2)
    0x05, 0x07,  //   USAGE_PAGE (Keyboard)

    0x19, 0xe0,  //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,  //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,  //   LOGICAL_MINIMUM (0)
    0x25, 0x01,  //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,  //   REPORT_SIZE (1)

    0x95, 0x08,  //   REPORT_COUNT (8)
    0x81, 0x02,  //   INPUT (Data,Var,Abs)
    0x95, 0x01,  //   REPORT_COUNT (1)
    0x75, 0x08,  //   REPORT_SIZE (8)
    0x81, 0x03,  //   INPUT (Cnst,Var,Abs)

    0x95, 0x06,  //   REPORT_COUNT (6)
    0x75, 0x08,  //   REPORT_SIZE (8)
    0x15, 0x00,  //   LOGICAL_MINIMUM (0)
    0x25, 0x65,  //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,  //   USAGE_PAGE (Keyboard)

    0x19, 0x00,  //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,  //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,  //   INPUT (Data,Ary,Abs)
    0xc0,        // END_COLLECTION

#if RAWHID_ENABLED
    // RAW HID
    0x06, LSB(RAWHID_USAGE_PAGE), MSB(RAWHID_USAGE_PAGE),  // 30
    0x0A, LSB(RAWHID_USAGE), MSB(RAWHID_USAGE),

    0xA1, 0x01,        // Collection 0x01
    0x85, 0x03,        // REPORT_ID (3)
    0x75, 0x08,        // report size = 8 bits
    0x15, 0x00,        // logical minimum = 0
    0x26, 0xFF, 0x00,  // logical maximum = 255

    0x95, 64,    // report count TX
    0x09, 0x01,  // usage
    0x81, 0x02,  // Input (array)

    0x95, 64,    // report count RX
    0x09, 0x02,  // usage
    0x91, 0x02,  // Output (array)
    0xC0         // end collection
#endif
};

#define HID_RPT_ID_KEY_IN 2    // Keyboard input report ID
#define HID_RPT_ID_MOUSE_IN 1  // Mouse input report ID
#define HID_RPT_ID_CC_IN 3     // Consumer Control input report ID

#define HID_RPT_ID_LED_OUT 0  // LED output report ID
#define HID_RPT_ID_FEATURE 0  // Feature report ID

#define HID_REPORT_TYPE_INPUT 1
#define HID_REPORT_TYPE_OUTPUT 2
#define HID_REPORT_TYPE_FEATURE 3

//==============================================================================
//==============================================================================
// Driver

uint8_t _hid_idle = 1;

//==============================================================================
//==============================================================================
// BeanMouse

BeanMouse_::BeanMouse_(void) : _buttons(0) {}

void BeanMouse_::begin(void) {}

void BeanMouse_::end(void) {}

void BeanMouse_::click(uint8_t b) {
  _buttons = b;
  move(0, 0, 0);
  _buttons = 0;
  move(0, 0, 0);
}

void BeanMouse_::move(signed char x, signed char y, signed char wheel) {
  BeanMouseReport m;
  m.mouse[0] = _buttons;
  m.mouse[1] = x;
  m.mouse[2] = y;
  m.mouse[3] = wheel;
  sendReport(&m);
}

void BeanMouse_::buttons(uint8_t b) {
  if (b != _buttons) {
    _buttons = b;
    move(0, 0, 0);
  }
}

void BeanMouse_::press(uint8_t b) { buttons(_buttons | b); }

void BeanMouse_::release(uint8_t b) { buttons(_buttons & ~b); }

bool BeanMouse_::isPressed(uint8_t b) {
  if ((b & _buttons) > 0) return true;
  return false;
}

void BeanMouse_::sendReport(BeanMouseReport *commands) {
  hidDevReport_t report;
  report.type = HID_REPORT_TYPE_INPUT;
  report.id = HID_RPT_ID_MOUSE_IN;
  report.len = HID_MOUSE_IN_RPT_LEN;

  if (report.len <= HID_DEV_DATA_LEN) {
    memcpy((void *)report.data, (void *)commands, sizeof(BeanMouseReport));
    Serial.write_message(MSG_ID_HID_SEND_REPORT, (uint8_t *)&report,
                         sizeof(hidDevReport_t));
  }
}

//==============================================================================
//==============================================================================
// Keyboard

BeanKeyboard_::BeanKeyboard_(void) {}

void BeanKeyboard_::begin(void) {}

// Macros for the HID Consumer Control 2-byte report
#define HID_CC_RPT_SET_NUMERIC(s, x) \
  (s)[0] &= HID_CC_RPT_NUMERIC_BITS; \
  (s)[0] = (x)
#define HID_CC_RPT_SET_CHANNEL(s, x) \
  (s)[0] &= HID_CC_RPT_CHANNEL_BITS; \
  (s)[0] |= ((x)&0x03) << 4
#define HID_CC_RPT_SET_VOLUME_UP(s) \
  (s)[0] &= HID_CC_RPT_VOLUME_BITS; \
  (s)[0] |= 0x40
#define HID_CC_RPT_SET_VOLUME_DOWN(s) \
  (s)[0] &= HID_CC_RPT_VOLUME_BITS;   \
  (s)[0] |= 0x80
#define HID_CC_RPT_SET_BUTTON(s, x) \
  (s)[1] &= HID_CC_RPT_BUTTON_BITS; \
  (s)[1] |= (x)
#define HID_CC_RPT_SET_SELECTION(s, x) \
  (s)[1] &= HID_CC_RPT_SELECTION_BITS; \
  (s)[1] |= ((x)&0x03) << 4

static void hidCCBuildReport(uint8_t *pBuf, uint8_t cmd) {
  /*
    Byte 0 - 4 LSB(bits 0,1,2,3) are used by Keypad

    Byte 0 - bits 4 & 5 are used for Channel Up/Down

    Byte 0 - bits 6 & 7 are used for Volume Up/Down

    Byte 1 - 4 LSB(bits 0,1,2,3) are used for Media keys(Play,Pause, mute etc..)

    Byte 1 - bits 4 & 5 are used for Button state information.

    Byte 1 - bits 6 & 7 are not defined in report map and hence ignored by host.
  */

  switch (cmd) {
    case HID_CONSUMER_CHANNEL_UP:
      HID_CC_RPT_SET_CHANNEL(pBuf, HID_CC_RPT_CHANNEL_UP);
      break;

    case HID_CONSUMER_CHANNEL_DOWN:
      HID_CC_RPT_SET_CHANNEL(pBuf, HID_CC_RPT_CHANNEL_DOWN);
      break;

    case HID_CONSUMER_VOLUME_UP:
      HID_CC_RPT_SET_VOLUME_UP(pBuf);
      break;

    case HID_CONSUMER_VOLUME_DOWN:
      HID_CC_RPT_SET_VOLUME_DOWN(pBuf);
      break;

    case HID_CONSUMER_MUTE:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_MUTE);
      break;

    case HID_CONSUMER_POWER:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_POWER);
      break;

    case HID_CONSUMER_RECALL_LAST:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_LAST);
      break;

    case HID_CONSUMER_ASSIGN_SEL:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_ASSIGN_SEL);
      break;

    case HID_CONSUMER_PLAY:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_PLAY);
      break;

    case HID_CONSUMER_PAUSE:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_PAUSE);
      break;

    case HID_CONSUMER_RECORD:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_RECORD);
      break;

    case HID_CONSUMER_FAST_FORWARD:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_FAST_FWD);
      break;

    case HID_CONSUMER_REWIND:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_REWIND);
      break;

    case HID_CONSUMER_SCAN_NEXT_TRK:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_SCAN_NEXT_TRK);
      break;

    case HID_CONSUMER_SCAN_PREV_TRK:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_SCAN_PREV_TRK);
      break;

    case HID_CONSUMER_STOP:
      HID_CC_RPT_SET_BUTTON(pBuf, HID_CC_RPT_STOP);
      break;

    default:
      if ((cmd >= HID_KEYBOARD_1) && (cmd <= HID_KEYBOARD_0)) {
        HID_CC_RPT_SET_BUTTON(pBuf, (cmd - HID_KEYBOARD_1 + 1) % 10);
      }
      break;
  }
}

void BeanKeyboard_::sendCC(uint8_t command) {
  hidDevReport_t report;
  report.type = HID_REPORT_TYPE_INPUT;
  report.id = HID_RPT_ID_CC_IN;
  report.len = HID_CC_IN_RPT_LEN;

  uint8_t buf[HID_CC_IN_RPT_LEN] = {0};

  hidCCBuildReport(buf, command);

  memcpy((void *)report.data, buf, HID_CC_IN_RPT_LEN);
  Serial.write_message(MSG_ID_HID_SEND_REPORT, (uint8_t *)&report,
                       sizeof(hidDevReport_t));
}

void BeanKeyboard_::holdCC(uint8_t command) {
  hidDevReport_t report;
  report.type = HID_REPORT_TYPE_INPUT;
  report.id = HID_RPT_ID_CC_IN;
  report.len = HID_CC_IN_RPT_LEN;

  hidCCBuildReport(ccHoldBuffer, command);

  memcpy((void *)report.data, ccHoldBuffer, HID_CC_IN_RPT_LEN);
  Serial.write_message(MSG_ID_HID_SEND_REPORT, (uint8_t *)&report,
                       sizeof(hidDevReport_t));
}

void BeanKeyboard_::releaseCC(uint8_t command) {
  hidDevReport_t report;
  report.type = HID_REPORT_TYPE_INPUT;
  report.id = HID_RPT_ID_CC_IN;
  report.len = HID_CC_IN_RPT_LEN;

  uint8_t disableBuf[HID_CC_IN_RPT_LEN] = {0};
  hidCCBuildReport(disableBuf, command);

  for (int i = 0; i < HID_CC_IN_RPT_LEN; i++) {
    ccHoldBuffer[i] &= ~disableBuf[i];
  }

  memcpy((void *)report.data, ccHoldBuffer, HID_CC_IN_RPT_LEN);
  Serial.write_message(MSG_ID_HID_SEND_REPORT, (uint8_t *)&report,
                       sizeof(hidDevReport_t));
}

void BeanKeyboard_::end(void) {}

void BeanKeyboard_::sendReport(BeanKeyReport *keys) {
  hidDevReport_t report;
  report.type = HID_REPORT_TYPE_INPUT;
  report.id = HID_RPT_ID_KEY_IN;
  report.len = HID_KEYBOARD_IN_RPT_LEN;

  if (report.len <= HID_DEV_DATA_LEN) {
    memcpy((void *)report.data, (void *)keys, sizeof(BeanKeyReport));
    Serial.write_message(MSG_ID_HID_SEND_REPORT, (uint8_t *)&report,
                         sizeof(hidDevReport_t));
  }
}

extern const uint8_t _asciimap[128] PROGMEM;

#define SHIFT 0x80
const uint8_t _asciimap[128] = {
    0x00,  // NUL
    0x00,  // SOH
    0x00,  // STX
    0x00,  // ETX
    0x00,  // EOT
    0x00,  // ENQ
    0x00,  // ACK
    0x00,  // BEL
    0x2a,  // BS Backspace
    0x2b,  // TAB Tab
    0x28,  // LF Enter
    0x00,  // VT
    0x00,  // FF
    0x00,  // CR
    0x00,  // SO
    0x00,  // SI
    0x00,  // DEL
    0x00,  // DC1
    0x00,  // DC2
    0x00,  // DC3
    0x00,  // DC4
    0x00,  // NAK
    0x00,  // SYN
    0x00,  // ETB
    0x00,  // CAN
    0x00,  // EM
    0x00,  // SUB
    0x00,  // ESC
    0x00,  // FS
    0x00,  // GS
    0x00,  // RS
    0x00,  // US

    0x2c,          //  ' '
    0x1e | SHIFT,  // !
    0x34 | SHIFT,  // "
    0x20 | SHIFT,  // #
    0x21 | SHIFT,  // $
    0x22 | SHIFT,  // %
    0x24 | SHIFT,  // &
    0x34,          // '
    0x26 | SHIFT,  // (
    0x27 | SHIFT,  // )
    0x25 | SHIFT,  // *
    0x2e | SHIFT,  // +
    0x36,          // ,
    0x2d,          // -
    0x37,          // .
    0x38,          // /
    0x27,          // 0
    0x1e,          // 1
    0x1f,          // 2
    0x20,          // 3
    0x21,          // 4
    0x22,          // 5
    0x23,          // 6
    0x24,          // 7
    0x25,          // 8
    0x26,          // 9
    0x33 | SHIFT,  // :
    0x33,          // ;
    0x36 | SHIFT,  // <
    0x2e,          // =
    0x37 | SHIFT,  // >
    0x38 | SHIFT,  // ?
    0x1f | SHIFT,  // @
    0x04 | SHIFT,  // A
    0x05 | SHIFT,  // B
    0x06 | SHIFT,  // C
    0x07 | SHIFT,  // D
    0x08 | SHIFT,  // E
    0x09 | SHIFT,  // F
    0x0a | SHIFT,  // G
    0x0b | SHIFT,  // H
    0x0c | SHIFT,  // I
    0x0d | SHIFT,  // J
    0x0e | SHIFT,  // K
    0x0f | SHIFT,  // L
    0x10 | SHIFT,  // M
    0x11 | SHIFT,  // N
    0x12 | SHIFT,  // O
    0x13 | SHIFT,  // P
    0x14 | SHIFT,  // Q
    0x15 | SHIFT,  // R
    0x16 | SHIFT,  // S
    0x17 | SHIFT,  // T
    0x18 | SHIFT,  // U
    0x19 | SHIFT,  // V
    0x1a | SHIFT,  // W
    0x1b | SHIFT,  // X
    0x1c | SHIFT,  // Y
    0x1d | SHIFT,  // Z
    0x2f,          // [
    0x31,          // bslash
    0x30,          // ]
    0x23 | SHIFT,  // ^
    0x2d | SHIFT,  // _
    0x35,          // `
    0x04,          // a
    0x05,          // b
    0x06,          // c
    0x07,          // d
    0x08,          // e
    0x09,          // f
    0x0a,          // g
    0x0b,          // h
    0x0c,          // i
    0x0d,          // j
    0x0e,          // k
    0x0f,          // l
    0x10,          // m
    0x11,          // n
    0x12,          // o
    0x13,          // p
    0x14,          // q
    0x15,          // r
    0x16,          // s
    0x17,          // t
    0x18,          // u
    0x19,          // v
    0x1a,          // w
    0x1b,          // x
    0x1c,          // y
    0x1d,          // z
    0x2f | SHIFT,  //
    0x31 | SHIFT,  // |
    0x30 | SHIFT,  // }
    0x35 | SHIFT,  // ~
    0              // DEL
};

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BeanKeyboard_::press(uint8_t k) {
  uint8_t i;
  if (k >= 136) {  // it's a non-printing key (not a modifier)
    k = k - 136;
  } else if (k >= 128) {  // it's a modifier key
    _keyReport.modifiers |= (1 << (k - 128));
    k = 0;
  } else {  // it's a printing key
    k = pgm_read_byte(_asciimap + k);
    if (!k) {
      setWriteError();
      return 0;
    }
    if (k &
        0x80) {  // it's a capital letter or other character reached with shift
      _keyReport.modifiers |= 0x02;  // the left shift modifier
      k &= 0x7F;
    }
  }

  // Add k to the key report only if it's not already present
  // and if there is an empty slot.
  if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
      _keyReport.keys[2] != k && _keyReport.keys[3] != k &&
      _keyReport.keys[4] != k && _keyReport.keys[5] != k) {
    for (i = 0; i < 6; i++) {
      if (_keyReport.keys[i] == 0x00) {
        _keyReport.keys[i] = k;
        break;
      }
    }
    if (i == 6) {
      setWriteError();
      return 0;
    }
  }
  sendReport(&_keyReport);
  return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BeanKeyboard_::release(uint8_t k) {
  uint8_t i;
  if (k >= 136) {  // it's a non-printing key (not a modifier)
    k = k - 136;
  } else if (k >= 128) {  // it's a modifier key
    _keyReport.modifiers &= ~(1 << (k - 128));
    k = 0;
  } else {  // it's a printing key
    k = pgm_read_byte(_asciimap + k);
    if (!k) {
      return 0;
    }
    if (k &
        0x80) {  // it's a capital letter or other character reached with shift
      _keyReport.modifiers &= ~(0x02);  // the left shift modifier
      k &= 0x7F;
    }
  }

  // Test the key report to see if k is present.  Clear it if it exists.
  // Check all positions in case the key is present more than once (which it
  // shouldn't be)
  for (i = 0; i < 6; i++) {
    if (0 != k && _keyReport.keys[i] == k) {
      _keyReport.keys[i] = 0x00;
    }
  }

  sendReport(&_keyReport);
  return 1;
}

void BeanKeyboard_::releaseAll(void) {
  _keyReport.keys[0] = 0;
  _keyReport.keys[1] = 0;
  _keyReport.keys[2] = 0;
  _keyReport.keys[3] = 0;
  _keyReport.keys[4] = 0;
  _keyReport.keys[5] = 0;
  _keyReport.modifiers = 0;
  sendReport(&_keyReport);
}

size_t BeanKeyboard_::write(uint8_t c) {
  uint8_t p = press(c);    // Keydown
  uint8_t r = release(c);  // Keyup
  return (p);  // just return the result of press() since release() almost
               // always returns 1
}
