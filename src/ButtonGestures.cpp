// ====================================================================================================
//
// PushButton library version 1.0
// written 2011 - trent m. wyatt
//

#include <Arduino.h>
#include "ButtonGestures.h"

ButtonGestures::ButtonGestures(int _pin) :
    state(NONE),
    pin(_pin),
    active(HIGH),
    input_mode(INPUT)
{
    set_button_input();
}

ButtonGestures::ButtonGestures(int _pin, int _active) :
    state(NONE),
    pin(_pin),
    active(_active),
    input_mode(INPUT)
{
    set_button_input();
}

ButtonGestures::ButtonGestures(int _pin, int _active, int _input_mode) :
    state(NONE),
    pin(_pin),
    active(_active),
    input_mode(_input_mode)
{
    (*this).set_button_input();
}

bool ButtonGestures::set_callback(const uint8_t _state, ButtonPressCallback _cb) {
    for (int index=0; index < int(sizeof(functors)/sizeof(*functors)); index++) {
        if (_state == functors[index].state) {
            functors[index].func = _cb;
            return true;
        }
    }

    return false;
}

ButtonPressCallback ButtonGestures::callback(const uint8_t _state) const {
    for (int index=0; index < sizeof(functors)/sizeof(*functors); index++) {
        if (_state == functors[index].state) {
            if (nullptr != functors[index].func) {
                functors[index].func(pin, _state);
                return functors[index].func;
            }
            else {
                return nullptr;
            }
        }
    }

    return nullptr;
}

// ====================================================================================================
// Set up a specific input pin for use as a push button input.
// Note: The input pin will be configured to be an INPUT or an
// INPUT_PULLUP depending on the value of input_mode.
// ====================================================================================================
void ButtonGestures::set_button_input() {
  pinMode(pin, input_mode);     // set the button pin as an input with optional pull-up resistor
}


// ====================================================================================================
// Get the state of a push button input
// Returns: true if the specified button was continuously pressed, otherwise returns false.
//
// Note: The button must be continuously pressed (no jitter/makes/breaks) for at least as long as KEYDBDELAY
//       (key debounce delay). This smooths out the dozens of button connections/disconnections detected at
//       the speed of the CPU when the button first begins to make contact into a lower frequency responce.
// ====================================================================================================
bool ButtonGestures::button_pressed() {
    unsigned long int presstime = millis() + KEYDBDELAY;

    while (active == digitalRead(pin)) {
        if (millis() >= presstime) {
            return true;
        }
    }

    return false;
}


// ====================================================================================================
// See if the user presses a button once, twice, or three times.  Also detect whether the user has held
// down the button to indicate a "long" press.  This is an enhanced button press check
// (as opposed to button_pressed(...)) in that it attempts to detect gestures.
// ====================================================================================================
uint8_t ButtonGestures::check_button_gesture() {
    if (!button_pressed()) {
        return NOT_PRESSED;
    }

    // The button is pressed.
    // Time how long the user presses the button to get the intent/gesture
    unsigned long int presstime = millis() + KEYLONGDELAY;  // get the current time in milliseconds plus the long-press offset
    while (button_pressed()) {
        if (millis() >= presstime) {
            return SINGLE_PRESS_LONG;
        }
    }

    // check for double-tap/press
    // The user has released the button, but this might be a double-tap.  Check again and decide.
    presstime = millis() + ALLOWED_MULTIPRESS_DELAY;
    while (millis() < presstime) {
    if (button_pressed()) {
        presstime = millis() + KEYLONGDELAY;
            while (button_pressed()) {
                if (millis() >= presstime) {
                    return DOUBLE_PRESS_LONG;
                }
            }

            // check for triple-tap/press
            // The user has released the button, but this might be a triple-tap.  Check again and decide.
            presstime = millis() + ALLOWED_MULTIPRESS_DELAY;
            while (millis() < presstime) {
                if (button_pressed()) {
                    presstime = millis() + KEYLONGDELAY;
                    while (button_pressed()) {
                        if (millis() >= presstime) {
                            return TRIPLE_PRESS_LONG;
                        }
                    }

                    return TRIPLE_PRESS_SHORT;
                }
            }

            return DOUBLE_PRESS_SHORT;
        }
    }

    return SINGLE_PRESS_SHORT;
}


// ====================================================================================================
// This wrapper function is used to allow consistent return values for back-to-back calls of
// check_button_internal(...) while the button is continuously pressed.  Without this extra step
// DOUBLE_BUTTON_LONG and TRIPLE_BUTTON_LONG presses would return correctly on their first check but
// would then be reported as SINGLE_BUTTON_LONG after that if the user kept the button pressed.
//
// Additionally this function prevents the spurious reporting of a *_BUTTON_SHORT state to be reported
// after the user has let go of a button once one or more *_BUTTON_LONG states have been observed.
// ====================================================================================================
uint8_t ButtonGestures::check_button() {
    uint8_t newstate = check_button_gesture();
    if (newstate & LONG_PRESS) {
        if (state & LONG_PRESS) {
            newstate = LONG_PRESS | (state & 0x0F);
            callback(newstate);
            return newstate;
        }
    }
    else {
        if (state & LONG_PRESS) {
            callback(NOT_PRESSED);
            return state = NOT_PRESSED;
        }
    }

    callback(newstate);
    return state = newstate;
}


// ====================================================================================================
//
// example use:
//
// ====================================================================================================

//#define EXAMPLE_SETUP_AND_LOOP
#ifdef EXAMPLE_SETUP_AND_LOOP

// NOTE: change/define the following pin(s) based on your project/connections
#define   BTNPIN1    2
#define   BTNPIN2    3

ButtonGestures  btn1(BTNPIN1);
ButtonGestures  btn2(BTNPIN2);

void report_button(const uint8_t state, const char* const label = NULL)  {
    switch (state) {
        case SINGLE_PRESS_SHORT: Serial.print(F("Single button short press")); break;
        case SINGLE_PRESS_LONG:  Serial.print(F("Single button long  press")); break;
        case DOUBLE_PRESS_SHORT: Serial.print(F("Double button short press")); break;
        case DOUBLE_PRESS_LONG:  Serial.print(F("Double button long  press")); break;
        case TRIPLE_PRESS_SHORT: Serial.print(F("Triple button short press")); break;
        case TRIPLE_PRESS_LONG:  Serial.print(F("Triple button long  press")); break;
        default:
            case NOT_PRESSED:
            return;
    }

    if (NULL != label) {
        Serial.print(F(" on "));
        Serial.print(label);
    }

    Serial.println();
}


void setup(void) {
    Serial.begin(9600);
    uint32_t timer = millis() + 2000;
    while (!Serial && millis() < timer);
    Serial.flush();
    Serial.println(F("\n\nArduino Core Library - ButtonGestures Library Test"));
}


void loop(void) {
    report_button(btn1.check_button(), "push button 1");
    report_button(btn2.check_button(), "push button 2");
}

#endif  // #ifdef EXAMPLE_SETUP_AND_LOOP
