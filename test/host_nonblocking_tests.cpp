#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BUTTONGESTURES_SKIP_ARDUINO_INCLUDE

#define LOW 0
#define HIGH 1
#define INPUT 0
#define INPUT_PULLUP 2

static uint32_t g_now = 0;
static int g_pin_state[256];
static uint8_t g_pin_mode[256];
static uint8_t g_callback_count = 0;
static uint8_t g_last_callback_pin = 0;
static uint8_t g_last_callback_state = 0;

uint32_t millis() {
    return g_now;
}

void pinMode(uint8_t pin, uint8_t mode) {
    g_pin_mode[pin] = mode;
}

int digitalRead(uint8_t pin) {
    return g_pin_state[pin];
}

#include "../src/ButtonGestures.cpp"

static const uint8_t BUTTON_PIN = 2;

static void fail(const char *test, const char *message, int line) {
    fprintf(stderr, "FAIL %s:%d: %s\n", test, line, message);
    exit(1);
}

#define CHECK_TRUE(test_name, expr) do { if (!(expr)) { fail((test_name), #expr, __LINE__); } } while (0)
#define CHECK_EQ(test_name, expected, actual) do { \
    unsigned long e_ = static_cast<unsigned long>(expected); \
    unsigned long a_ = static_cast<unsigned long>(actual); \
    if (e_ != a_) { \
        fprintf(stderr, "FAIL %s:%d: expected %lu got %lu\n", (test_name), __LINE__, e_, a_); \
        exit(1); \
    } \
} while (0)

static void set_pressed(bool pressed) {
    g_pin_state[BUTTON_PIN] = pressed ? LOW : HIGH;
}

static void reset_world() {
    g_now = 0;
    for (uint16_t i = 0; i < 256; ++i) {
        g_pin_state[i] = HIGH;
        g_pin_mode[i] = 0;
    }
    g_callback_count = 0;
    g_last_callback_pin = 0;
    g_last_callback_state = NOT_PRESSED;
}

static uint8_t tick(ButtonGestures &button, uint16_t ms) {
    uint8_t observed = NOT_PRESSED;
    for (uint16_t i = 0; i < ms; ++i) {
        ++g_now;
        const uint8_t state = button.check_button();
        if (state != NOT_PRESSED && observed == NOT_PRESSED) {
            observed = state;
        }
    }
    return observed;
}

static uint8_t tick_until_event(ButtonGestures &button, uint16_t limit_ms) {
    for (uint16_t i = 0; i < limit_ms; ++i) {
        ++g_now;
        const uint8_t state = button.check_button();
        if (state != NOT_PRESSED) {
            return state;
        }
    }
    return NOT_PRESSED;
}

static uint8_t poll_now(ButtonGestures &button) {
    return button.check_button();
}

static void callback_recorder(const uint8_t pin, const uint8_t state) {
    ++g_callback_count;
    g_last_callback_pin = pin;
    g_last_callback_state = state;
}

static uint8_t do_short_tap(ButtonGestures &button, uint16_t held_ms) {
    set_pressed(true);
    CHECK_EQ("short tap press", NOT_PRESSED, tick(button, static_cast<uint16_t>(KEYDBDELAY + held_ms)));
    set_pressed(false);
    return poll_now(button);
}

static void test_button_pressed_is_nonblocking_and_press_debounced() {
    const char *test = "button_pressed";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, INPUT_PULLUP, g_pin_mode[BUTTON_PIN]);
    CHECK_TRUE(test, !button.button_pressed());

    set_pressed(true);
    CHECK_TRUE(test, !button.button_pressed());
    CHECK_TRUE(test, !tick(button, KEYDBDELAY - 1));
    ++g_now;
    CHECK_TRUE(test, button.button_pressed());

    set_pressed(false);
    CHECK_TRUE(test, !button.button_pressed());
}

static void test_single_short_is_reported_after_multipress_window() {
    const char *test = "single short";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 25));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY - 1));
    CHECK_EQ(test, SINGLE_PRESS_SHORT, tick_until_event(button, 2));
    CHECK_EQ(test, NOT_PRESSED, tick(button, 10));
}

static void test_double_short_is_reported_after_second_window() {
    const char *test = "double short";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY / 2));
    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY - 1));
    CHECK_EQ(test, DOUBLE_PRESS_SHORT, tick_until_event(button, 2));
}

static void test_triple_short_is_reported_on_third_release() {
    const char *test = "triple short";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY / 2));
    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY / 2));
    CHECK_EQ(test, TRIPLE_PRESS_SHORT, do_short_tap(button, 20));
}

static void test_late_started_second_tap_is_still_accepted() {
    const char *test = "late second tap";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY - 1));

    set_pressed(true);
    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
    CHECK_EQ(test, NOT_PRESSED, tick(button, KEYDBDELAY));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
    CHECK_EQ(test, DOUBLE_PRESS_SHORT, tick_until_event(button, ALLOWED_MULTIPRESS_DELAY + 2));
}

static void test_late_started_tap_after_window_starts_new_gesture() {
    const char *test = "late new tap";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, SINGLE_PRESS_SHORT, tick_until_event(button, ALLOWED_MULTIPRESS_DELAY + 2));

    set_pressed(true);
    CHECK_EQ(test, NOT_PRESSED, tick(button, KEYDBDELAY + 20));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
    CHECK_EQ(test, SINGLE_PRESS_SHORT, tick_until_event(button, ALLOWED_MULTIPRESS_DELAY + 2));
}

static void test_single_long_repeats_by_default() {
    const char *test = "long repeat default";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_TRUE(test, button.long_press_repeat());
    CHECK_EQ(test, DEFAULT_LONG_REPEAT_DELAY, button.long_press_repeat_delay());

    set_pressed(true);
    CHECK_EQ(test, SINGLE_PRESS_LONG, tick_until_event(button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, SINGLE_PRESS_LONG, tick_until_event(button, DEFAULT_LONG_REPEAT_DELAY + 2));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
}

static void test_single_long_can_be_single_shot() {
    const char *test = "long single shot";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);
    button.set_long_press_repeat(false);

    CHECK_TRUE(test, !button.long_press_repeat());
    set_pressed(true);
    CHECK_EQ(test, SINGLE_PRESS_LONG, tick_until_event(button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, NOT_PRESSED, tick(button, DEFAULT_LONG_REPEAT_DELAY * 2));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
}

static void test_double_long_repeats_as_double_long() {
    const char *test = "double long repeat";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY / 2));
    set_pressed(true);
    CHECK_EQ(test, DOUBLE_PRESS_LONG, tick_until_event(button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, DOUBLE_PRESS_LONG, tick_until_event(button, DEFAULT_LONG_REPEAT_DELAY + 2));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
}

static void test_triple_long_repeat_and_single_shot_modes_are_explicit() {
    const char *test = "triple long mode";
    reset_world();
    ButtonGestures repeat_button(BUTTON_PIN, LOW, INPUT_PULLUP);
    CHECK_TRUE(test, repeat_button.set_long_press_mode(LONG_PRESS_REPEAT));

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(repeat_button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(repeat_button, ALLOWED_MULTIPRESS_DELAY / 2));
    CHECK_EQ(test, NOT_PRESSED, do_short_tap(repeat_button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(repeat_button, ALLOWED_MULTIPRESS_DELAY / 2));
    set_pressed(true);
    CHECK_EQ(test, TRIPLE_PRESS_LONG, tick_until_event(repeat_button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, TRIPLE_PRESS_LONG, tick_until_event(repeat_button, DEFAULT_LONG_REPEAT_DELAY + 2));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(repeat_button));

    reset_world();
    ButtonGestures once_button(BUTTON_PIN, LOW, INPUT_PULLUP);
    CHECK_TRUE(test, once_button.set_long_press_mode(LONG_PRESS_SINGLE_SHOT));

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(once_button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(once_button, ALLOWED_MULTIPRESS_DELAY / 2));
    CHECK_EQ(test, NOT_PRESSED, do_short_tap(once_button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(once_button, ALLOWED_MULTIPRESS_DELAY / 2));
    set_pressed(true);
    CHECK_EQ(test, TRIPLE_PRESS_LONG, tick_until_event(once_button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, NOT_PRESSED, tick(once_button, DEFAULT_LONG_REPEAT_DELAY * 2));
    set_pressed(false);
    CHECK_EQ(test, NOT_PRESSED, poll_now(once_button));
}

static void test_invalid_long_press_mode_is_rejected_without_changing_mode() {
    const char *test = "invalid long mode";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    CHECK_TRUE(test, button.long_press_repeat());
    CHECK_TRUE(test, !button.set_long_press_mode(42));
    CHECK_TRUE(test, button.long_press_repeat());

    CHECK_TRUE(test, button.set_long_press_mode(LONG_PRESS_SINGLE_SHOT));
    CHECK_TRUE(test, !button.long_press_repeat());
    CHECK_TRUE(test, !button.set_long_press_mode(255));
    CHECK_TRUE(test, !button.long_press_repeat());
}

static void test_custom_repeat_delay() {
    const char *test = "custom repeat delay";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);
    button.set_long_press_repeat_delay(100);

    CHECK_EQ(test, 100, button.long_press_repeat_delay());
    set_pressed(true);
    CHECK_EQ(test, SINGLE_PRESS_LONG, tick_until_event(button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, NOT_PRESSED, tick(button, 99));
    CHECK_EQ(test, SINGLE_PRESS_LONG, tick_until_event(button, 2));
}

static void test_complete_press_between_polls_is_missed() {
    const char *test = "coarse polling";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

    set_pressed(true);
    g_now += KEYDBDELAY + 20;
    set_pressed(false);
    g_now += ALLOWED_MULTIPRESS_DELAY + 1;

    CHECK_EQ(test, NOT_PRESSED, poll_now(button));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY + KEYDBDELAY + 2));
}

static void test_callbacks_fire_once_per_reported_gesture() {
    const char *test = "callbacks";
    reset_world();
    ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);
    button.set_long_press_repeat(false);
    CHECK_TRUE(test, button.set_callback(SHORT1, callback_recorder));
    CHECK_TRUE(test, button.set_callback(LONG3, callback_recorder));
    CHECK_TRUE(test, !button.set_callback(NOT_PRESSED, callback_recorder));

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, SINGLE_PRESS_SHORT, tick_until_event(button, ALLOWED_MULTIPRESS_DELAY + 2));
    CHECK_EQ(test, 1, g_callback_count);
    CHECK_EQ(test, BUTTON_PIN, g_last_callback_pin);
    CHECK_EQ(test, SINGLE_PRESS_SHORT, g_last_callback_state);

    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY / 2));
    CHECK_EQ(test, NOT_PRESSED, do_short_tap(button, 20));
    CHECK_EQ(test, NOT_PRESSED, tick(button, ALLOWED_MULTIPRESS_DELAY / 2));
    set_pressed(true);
    CHECK_EQ(test, TRIPLE_PRESS_LONG, tick_until_event(button, KEYDBDELAY + KEYLONGDELAY + 2));
    CHECK_EQ(test, 2, g_callback_count);
    CHECK_EQ(test, TRIPLE_PRESS_LONG, g_last_callback_state);
}

int main() {
    test_button_pressed_is_nonblocking_and_press_debounced();
    test_single_short_is_reported_after_multipress_window();
    test_double_short_is_reported_after_second_window();
    test_triple_short_is_reported_on_third_release();
    test_late_started_second_tap_is_still_accepted();
    test_late_started_tap_after_window_starts_new_gesture();
    test_single_long_repeats_by_default();
    test_single_long_can_be_single_shot();
    test_double_long_repeats_as_double_long();
    test_triple_long_repeat_and_single_shot_modes_are_explicit();
    test_invalid_long_press_mode_is_rejected_without_changing_mode();
    test_custom_repeat_delay();
    test_complete_press_between_polls_is_missed();
    test_callbacks_fire_once_per_reported_gesture();

    printf("host_nonblocking_tests passed\n");
    return 0;
}
