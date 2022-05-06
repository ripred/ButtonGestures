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
\*/

// include guard
#ifndef BUTTONGESTURES_INCL
#define BUTTONGESTURES_INCL

// ====================================================================================================
//
// manifest constants for button library
//
#define  NOT_PRESSED         0x00

#define  SINGLE_BUTTON       0x01
#define  DOUBLE_BUTTON       0x02
#define  TRIPLE_BUTTON       0x04

#define  SHORT_PRESS         0x10
#define  LONG_PRESS          0x20

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

typedef void (*ButtonPressCallback)(const uint8_t, const uint8_t);

struct ButtonGestures {
    uint8_t     state;
    uint8_t     pin;
    int         active;
    int         input_mode;
    struct {
        uint8_t                 state;
        ButtonPressCallback     func;
    } functors[7] {
        { NONE,    nullptr },
        { SHORT1,  nullptr },
        { LONG1,   nullptr },
        { SHORT2,  nullptr },
        { LONG2,   nullptr },
        { SHORT3,  nullptr },
        { LONG3,   nullptr }
    };

    ButtonGestures(int _pin);
    ButtonGestures(int _pin, int _active);
    ButtonGestures(int _pin, int _active, int _input_mode);

    bool set_callback(const uint8_t _state, ButtonPressCallback _cb);
    ButtonPressCallback callback(const uint8_t _state) const;
    void set_button_input();
    bool button_pressed();
    uint8_t check_button_gesture();
    uint8_t check_button();
};

#endif // #ifndef BUTTONGESTURES_INCL
