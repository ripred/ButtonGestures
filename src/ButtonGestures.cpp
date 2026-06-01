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
|*| version 3.0
|*| written 2026 - trent m. wyatt
|*| Converted gesture recognition to a non-blocking state machine
|*|
\*/

#ifndef BUTTONGESTURES_SKIP_ARDUINO_INCLUDE
#include <Arduino.h>
#endif
#include "ButtonGestures.h"

ButtonGestures::ButtonGestures(const int _pin) :
    state(NONE),
    pin(_pin),
    active(HIGH),
    input_mode(INPUT),
    gesture_state(WAITING_FOR_PRESS),
    press_count(0),
    raw_pressed(0),
    debounced_pressed(0),
    long_reported(0),
    repeat_long(LONG_PRESS_REPEAT),
    raw_changed_at(0),
    press_started_at(0),
    released_at(0),
    last_long_report_at(0),
    long_repeat_delay(DEFAULT_LONG_REPEAT_DELAY)
{
    set_button_input();
    reset();
}

ButtonGestures::ButtonGestures(const int _pin, const int _active) :
    state(NONE),
    pin(_pin),
    active(_active),
    input_mode(INPUT),
    gesture_state(WAITING_FOR_PRESS),
    press_count(0),
    raw_pressed(0),
    debounced_pressed(0),
    long_reported(0),
    repeat_long(LONG_PRESS_REPEAT),
    raw_changed_at(0),
    press_started_at(0),
    released_at(0),
    last_long_report_at(0),
    long_repeat_delay(DEFAULT_LONG_REPEAT_DELAY)
{
    set_button_input();
    reset();
}

ButtonGestures::ButtonGestures(const int _pin, const int _active, const int _input_mode) :
    state(NONE),
    pin(_pin),
    active(_active),
    input_mode(_input_mode),
    gesture_state(WAITING_FOR_PRESS),
    press_count(0),
    raw_pressed(0),
    debounced_pressed(0),
    long_reported(0),
    repeat_long(LONG_PRESS_REPEAT),
    raw_changed_at(0),
    press_started_at(0),
    released_at(0),
    last_long_report_at(0),
    long_repeat_delay(DEFAULT_LONG_REPEAT_DELAY)
{
    set_button_input();
    reset();
}

bool ButtonGestures::set_callback(const uint8_t _state, const ButtonPressCallback _cb) {
    switch (_state) {
        default:        return false;
        case SHORT1:    short1 = _cb;   return true;
        case  LONG1:     long1 = _cb;   return true;
        case SHORT2:    short2 = _cb;   return true;
        case  LONG2:     long2 = _cb;   return true;
        case SHORT3:    short3 = _cb;   return true;
        case  LONG3:     long3 = _cb;   return true;
    }
}

ButtonPressCallback ButtonGestures::callback(const uint8_t _state) const {
    switch (_state) {
        default:        return nullptr;
        case SHORT1:    if (nullptr == short1) { return nullptr; } else { short1(pin, _state);  return short1; }
        case  LONG1:    if (nullptr ==  long1) { return nullptr; } else {  long1(pin, _state);  return  long1; }
        case SHORT2:    if (nullptr == short2) { return nullptr; } else { short2(pin, _state);  return short2; }
        case  LONG2:    if (nullptr ==  long2) { return nullptr; } else {  long2(pin, _state);  return  long2; }
        case SHORT3:    if (nullptr == short3) { return nullptr; } else { short3(pin, _state);  return short3; }
        case  LONG3:    if (nullptr ==  long3) { return nullptr; } else {  long3(pin, _state);  return  long3; }
    }
}

bool ButtonGestures::elapsed(const uint32_t now, const uint32_t since, const uint32_t delay) {
    return static_cast<uint32_t>(now - since) >= delay;
}

bool ButtonGestures::read_raw_pressed() const {
    return active == digitalRead(pin);
}

void ButtonGestures::update_button(const uint32_t now) {
    const uint8_t current_raw_pressed = read_raw_pressed() ? 1 : 0;

    if (current_raw_pressed != raw_pressed) {
        raw_pressed = current_raw_pressed;
        raw_changed_at = now;
        if (!raw_pressed) {
            debounced_pressed = 0;
        }
    }

    if (raw_pressed && !debounced_pressed && elapsed(now, raw_changed_at, KEYDBDELAY)) {
        debounced_pressed = 1;
    }
}

void ButtonGestures::reset_gesture_state() {
    gesture_state = WAITING_FOR_PRESS;
    press_count = 0;
    long_reported = 0;
    press_started_at = 0;
    released_at = 0;
    last_long_report_at = 0;
}

bool ButtonGestures::raw_press_started_inside_multipress_window() const {
    return !elapsed(raw_changed_at, released_at, ALLOWED_MULTIPRESS_DELAY);
}

uint8_t ButtonGestures::short_gesture() const {
    switch (press_count) {
        default: return NOT_PRESSED;
        case 1:  return SINGLE_PRESS_SHORT;
        case 2:  return DOUBLE_PRESS_SHORT;
        case 3:  return TRIPLE_PRESS_SHORT;
    }
}

uint8_t ButtonGestures::long_gesture() const {
    switch (press_count) {
        default: return NOT_PRESSED;
        case 1:  return SINGLE_PRESS_LONG;
        case 2:  return DOUBLE_PRESS_LONG;
        case 3:  return TRIPLE_PRESS_LONG;
    }
}

/*\
|*| Set up a specific input pin for use as a push button input.
|*| Note: The input pin will be configured to be an INPUT or an
|*| INPUT_PULLUP depending on the value of input_mode.
\*/
void ButtonGestures::set_button_input() {
    pinMode(pin, input_mode);
}

/*\
|*| Reset gesture recognition without changing the configured pin,
|*| callbacks, input mode, active level, or long-press repeat policy.
\*/
void ButtonGestures::reset() {
    const uint32_t now = millis();
    raw_pressed = read_raw_pressed() ? 1 : 0;
    debounced_pressed = 0;
    raw_changed_at = now;
    state = NOT_PRESSED;
    reset_gesture_state();
}

/*\
|*| Enable or disable repeated long-press reports while the same
|*| physical press is still held. Repeats are enabled by default to
|*| preserve the legacy implementation's observable long-hold behavior.
\*/
void ButtonGestures::set_long_press_repeat(const bool _repeat) {
    repeat_long = _repeat ? LONG_PRESS_REPEAT : LONG_PRESS_SINGLE_SHOT;
}

bool ButtonGestures::set_long_press_mode(const uint8_t _mode) {
    switch (_mode) {
        case LONG_PRESS_REPEAT:
            set_long_press_repeat(true);
            return true;
        case LONG_PRESS_SINGLE_SHOT:
            set_long_press_repeat(false);
            return true;
        default:
            return false;
    }
}

bool ButtonGestures::long_press_repeat() const {
    return repeat_long == LONG_PRESS_REPEAT;
}

void ButtonGestures::set_long_press_repeat_delay(const uint16_t _delay_ms) {
    long_repeat_delay = _delay_ms ? _delay_ms : DEFAULT_LONG_REPEAT_DELAY;
}

uint16_t ButtonGestures::long_press_repeat_delay() const {
    return long_repeat_delay;
}

/*\
|*| Get the state of a push button input.
|*| Returns immediately. A press is true only after the button has been
|*| continuously active for KEYDBDELAY milliseconds. Release is reported
|*| immediately, matching the legacy blocking implementation.
\*/
bool ButtonGestures::button_pressed() {
    update_button(millis());
    return debounced_pressed != 0;
}

/*\
|*| See if the user presses a button once, twice, or three times. Also
|*| detect whether the final press has been held long enough to be a
|*| long press. This non-blocking implementation returns NOT_PRESSED
|*| until a gesture is fully determined.
\*/
uint8_t ButtonGestures::check_button_gesture() {
    const uint32_t now = millis();
    update_button(now);

    switch (gesture_state) {
        case WAITING_FOR_PRESS:
            if (debounced_pressed) {
                press_count = 1;
                long_reported = 0;
                press_started_at = static_cast<uint32_t>(raw_changed_at + KEYDBDELAY);
                gesture_state = WAITING_FOR_RELEASE;
            }
            break;

        case WAITING_FOR_RELEASE:
            if (debounced_pressed) {
                if (!long_reported && elapsed(now, press_started_at, KEYLONGDELAY)) {
                    long_reported = 1;
                    last_long_report_at = now;
                    return long_gesture();
                }

                if (long_reported && long_press_repeat() && elapsed(now, last_long_report_at, long_repeat_delay)) {
                    last_long_report_at = now;
                    return long_gesture();
                }
            } else if (long_reported) {
                reset_gesture_state();
            } else if (press_count >= 3) {
                const uint8_t gesture = short_gesture();
                reset_gesture_state();
                return gesture;
            } else {
                released_at = now;
                gesture_state = WAITING_FOR_NEXT_PRESS;
            }
            break;

        case WAITING_FOR_NEXT_PRESS:
            if (debounced_pressed) {
                if (raw_press_started_inside_multipress_window()) {
                    if (press_count < 3) {
                        ++press_count;
                    }
                    long_reported = 0;
                    press_started_at = static_cast<uint32_t>(raw_changed_at + KEYDBDELAY);
                    gesture_state = WAITING_FOR_RELEASE;
                } else {
                    const uint8_t gesture = short_gesture();
                    reset_gesture_state();
                    return gesture;
                }
            } else if (raw_pressed && raw_press_started_inside_multipress_window()) {
                /* Wait for debounce when the next tap started before the timeout. */
            } else if (elapsed(now, released_at, ALLOWED_MULTIPRESS_DELAY)) {
                const uint8_t gesture = short_gesture();
                reset_gesture_state();
                return gesture;
            }
            break;

        default:
            reset_gesture_state();
            break;
    }

    return NOT_PRESSED;
}

uint8_t ButtonGestures::check_button() {
    const uint8_t newstate = check_button_gesture();
    state = newstate;

    if (NONE != state) {
        callback(state);
    }

    return state;
}
