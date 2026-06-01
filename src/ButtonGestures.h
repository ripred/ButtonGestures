/*\
|*| PushButton Gesture Library version 1.0
|*| written 2011 - trent m. wyatt
|*|
|*| v1.01  ++tmw, May 4, 2022
|*| Fixed typos in example code
|*|
|*| v2.0  ++tmw, May 5, 2022
|*| Rewrote as C++ class
|*| Packaged as Arduino Library
|*|
|*| v3.0  ++tmw, 2026
|*| Refactored gesture recognition to be non-blocking
|*| Added explicit long-press repeat/single-shot control
\*/

// include guard
#ifndef BUTTONGESTURES_INCL
#define BUTTONGESTURES_INCL

#include <stdint.h>

// ====================================================================================================
//
// manifest constants for button library
//
#define  NOT_PRESSED         0x00

#define  SINGLE_BUTTON       0x01
#define  DOUBLE_BUTTON       0x02
#define  TRIPLE_BUTTON       0x04

#define  BUTTON_MASK         0x07

#define  SHORT_PRESS         0x08
#define  LONG_PRESS          0x10

#define  SINGLE_PRESS_SHORT (SINGLE_BUTTON | SHORT_PRESS)
#define  SINGLE_PRESS_LONG  (SINGLE_BUTTON | LONG_PRESS)
#define  DOUBLE_PRESS_SHORT (DOUBLE_BUTTON | SHORT_PRESS)
#define  DOUBLE_PRESS_LONG  (DOUBLE_BUTTON | LONG_PRESS)
#define  TRIPLE_PRESS_SHORT (TRIPLE_BUTTON | SHORT_PRESS)
#define  TRIPLE_PRESS_LONG  (TRIPLE_BUTTON | LONG_PRESS)

// shorter alias' for the button gestures:
#define  NONE       NOT_PRESSED
#define  SHORT1     SINGLE_PRESS_SHORT
#define  LONG1      SINGLE_PRESS_LONG
#define  SHORT2     DOUBLE_PRESS_SHORT
#define  LONG2      DOUBLE_PRESS_LONG
#define  SHORT3     TRIPLE_PRESS_SHORT
#define  LONG3      TRIPLE_PRESS_LONG

#define  KEYDBDELAY               36                // original bell labs standard phone push button debounce delay in mS
#define  KEYLONGDELAY             (KEYDBDELAY * 20) // how long to consider a pressed button a "long press" versus a "short press"
#define  ALLOWED_MULTIPRESS_DELAY (KEYDBDELAY * 7)  // the amount of time allowed between multiple "taps" to be considered part of the last "tap"
#define  DEFAULT_LONG_REPEAT_DELAY KEYLONGDELAY     // default repeat interval for long press gestures in mS

#define  LONG_PRESS_SINGLE_SHOT   0x00
#define  LONG_PRESS_REPEAT        0x01

typedef void (*ButtonPressCallback)(const uint8_t /*_pin*/, const uint8_t /*_state*/);
typedef ButtonPressCallback BpCb;

struct ButtonGestures {
private:
    enum gesture_machine_state_t {
        WAITING_FOR_PRESS = 0,
        WAITING_FOR_RELEASE,
        WAITING_FOR_NEXT_PRESS
    };

    uint8_t     state;
    uint8_t     pin;
    int         active;
    int         input_mode;

    BpCb        short1 {nullptr};
    BpCb        long1  {nullptr};
    BpCb        short2 {nullptr};
    BpCb        long2  {nullptr};
    BpCb        short3 {nullptr};
    BpCb        long3  {nullptr};

    uint8_t     gesture_state;
    uint8_t     press_count;
    uint8_t     raw_pressed;
    uint8_t     debounced_pressed;
    uint8_t     long_reported;
    uint8_t     repeat_long;
    uint32_t    raw_changed_at;
    uint32_t    press_started_at;
    uint32_t    released_at;
    uint32_t    last_long_report_at;
    uint16_t    long_repeat_delay;

    ButtonGestures() = delete;

    static bool elapsed(uint32_t now, uint32_t since, uint32_t delay);
    bool read_raw_pressed() const;
    void update_button(uint32_t now);
    void reset_gesture_state();
    bool raw_press_started_inside_multipress_window() const;
    uint8_t short_gesture() const;
    uint8_t long_gesture() const;

public:
    ButtonGestures(const int _pin);
    ButtonGestures(const int _pin, int _active);
    ButtonGestures(const int _pin, int _active, int _input_mode);

    bool set_callback(const uint8_t _state, const ButtonPressCallback _cb);
    ButtonPressCallback callback(const uint8_t _state) const;
    void set_button_input();
    bool button_pressed();
    void reset();
    void set_long_press_repeat(bool _repeat);
    bool set_long_press_mode(uint8_t _mode);
    bool long_press_repeat() const;
    void set_long_press_repeat_delay(uint16_t _delay_ms);
    uint16_t long_press_repeat_delay() const;
    uint8_t check_button_gesture();
    uint8_t check_button();
};

#endif // #ifndef BUTTONGESTURES_INCL
