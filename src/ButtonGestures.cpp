/*\
|*| ButtonGestures.cpp
|*| Push Button Gesture Library
|*|
|*| version 1.0
|*| written 2011 - trent m. wyatt
|*| Original writing
|*|
|*| version 2.0
|*| written 2022 - trent m. wyatt
|*| Converted to class library
|*|
\*/

#include <Arduino.h>
#include "ButtonGestures.h"

void ButtonGestures::init_functors() {
    functors[0].state = SHORT1;  functors[0].func = nullptr;
    functors[1].state = LONG1;   functors[1].func = nullptr;
    functors[2].state = SHORT2;  functors[2].func = nullptr;
    functors[3].state = LONG2;   functors[3].func = nullptr;
    functors[4].state = SHORT3;  functors[4].func = nullptr;
    functors[5].state = LONG3;   functors[5].func = nullptr;
}

ButtonGestures::ButtonGestures(const int _pin) :
    state(NONE),
    pin(_pin),
    active(HIGH),
    input_mode(INPUT)
{
    set_button_input();
    init_functors();
}

ButtonGestures::ButtonGestures(const int _pin, const int _active) :
    state(NONE),
    pin(_pin),
    active(_active),
    input_mode(INPUT)
{
    set_button_input();
    init_functors();
}

ButtonGestures::ButtonGestures(const int _pin, const int _active, const int _input_mode) :
    state(NONE),
    pin(_pin),
    active(_active),
    input_mode(_input_mode)
{
    set_button_input();
    init_functors();
}

bool ButtonGestures::set_callback(const uint8_t _state, const ButtonPressCallback _cb) {
    for (unsigned index=0; index < (sizeof(functors)/sizeof(*functors)); ++index) {
        if (_state == functors[index].state) {
            functors[index].func = _cb;
            return true;
        }
    }

    return false;
}

ButtonPressCallback ButtonGestures::callback(const uint8_t _state) const {
    for (unsigned index=0; index < (sizeof(functors)/sizeof(*functors)); ++index) {
        if (_state == functors[index].state) {
            if (nullptr == functors[index].func) {
                Serial.print("Functor for ");
                Serial.print(_state, HEX);
                Serial.println("is nullptr.");
                return nullptr;
            }
            functors[index].func(pin, _state);
            return functors[index].func;
        }
    }

    return nullptr;
}

/*\
|*| Set up a specific input pin for use as a push button input.
|*| Note: The input pin will be configured to be an INPUT or an
|*| INPUT_PULLUP depending on the value of input_mode.
\*/
void ButtonGestures::set_button_input() {
  pinMode(pin, input_mode);     // set the button pin as an input with optional pull-up resistor
}


/*\
|*| Get the state of a push button input
|*| Returns: true if the specified button was continuously pressed, otherwise returns false.
|*|
|*| Note: The button must be continuously pressed (no jitter/makes/breaks) for at least as long as KEYDBDELAY
|*|       (key debounce delay). This smooths out the dozens of button connections/disconnections detected at
|*|       the speed of the CPU when the button first begins to make contact into a lower frequency responce.
\*/
bool ButtonGestures::button_pressed() {
    uint32_t presstime = millis() + KEYDBDELAY;
    while (active == digitalRead(pin)) {
        if (millis() >= presstime) {
            return true;
        }
    }

    return false;
}


/*\
|*| See if the user presses a button once, twice, or three times.  Also detect whether the user has held
|*| down the button to indicate a "long" press.  This is an enhanced button press check
|*| (as opposed to button_pressed(...)) in that it attempts to detect gestures.
\*/
uint8_t ButtonGestures::check_button_gesture() {
    if (!button_pressed()) {
        return NOT_PRESSED;
    }

    // The button is pressed.
    // Time how long the user presses the button to get the intent/gesture
    uint32_t presstime = millis() + KEYLONGDELAY;  // get the current time in milliseconds plus the long-press offset
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


/*\
|*| This wrapper function is used to allow consistent return values for back-to-back calls of
|*| check_button_internal(...) while the button is continuously pressed.  Without this extra step
|*| DOUBLE_BUTTON_LONG and TRIPLE_BUTTON_LONG presses would return correctly on their first check but
|*| would then be reported as SINGLE_BUTTON_LONG after that if the user kept the button pressed.
|*|
|*| Additionally this function prevents the spurious reporting of a *_BUTTON_SHORT state to be reported
|*| after the user has let go of a button once one or more *_BUTTON_LONG states have been observed.
\*/
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
            return state = NOT_PRESSED;
        }
    }

    state = newstate;

    if (NONE != state) {
        callback(state);
    }

    return state;
}
