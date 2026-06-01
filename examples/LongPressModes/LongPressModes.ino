/*\
|*| ButtonGestures long-press mode example
|*|
|*| This sketch cycles through the repeat, single-shot, and custom
|*| repeat-delay modes. Watch Serial output and hold the button through
|*| each mode to see the difference.
|*|
|*| One momentary pushbutton is wired from BUTTON_PIN to GND.
|*|
|*| (c) 2026 trent m. wyatt
|*|
\*/

#include <ButtonGestures.h>

#define BUTTON_PIN 2

ButtonGestures button(BUTTON_PIN, LOW, INPUT_PULLUP);

static const uint32_t MODE_WINDOW_MS = 15000UL;
static const uint16_t FAST_REPEAT_MS = 250;

enum demo_mode_t {
    DEMO_DEFAULT_REPEAT = 0,
    DEMO_SINGLE_SHOT,
    DEMO_FAST_REPEAT,
    DEMO_MODE_COUNT
};

static uint8_t currentMode = DEMO_MODE_COUNT;
static uint32_t modeStartedAt = 0;

static bool elapsed(const uint32_t now, const uint32_t since, const uint32_t interval) {
    return static_cast<uint32_t>(now - since) >= interval;
}

static void printGesture(const uint8_t state) {
    switch (state) {
        case SINGLE_PRESS_SHORT: Serial.println(F("single short")); break;
        case SINGLE_PRESS_LONG:  Serial.println(F("single long"));  break;
        case DOUBLE_PRESS_SHORT: Serial.println(F("double short")); break;
        case DOUBLE_PRESS_LONG:  Serial.println(F("double long"));  break;
        case TRIPLE_PRESS_SHORT: Serial.println(F("triple short")); break;
        case TRIPLE_PRESS_LONG:  Serial.println(F("triple long"));  break;
        case NOT_PRESSED:
        default:
            break;
    }
}

static void applyMode(const uint8_t mode) {
    currentMode = mode;
    modeStartedAt = millis();

    Serial.println();
    switch (mode) {
        case DEMO_DEFAULT_REPEAT:
            button.set_long_press_mode(LONG_PRESS_REPEAT);
            button.set_long_press_repeat_delay(DEFAULT_LONG_REPEAT_DELAY);
            Serial.println(F("Mode: default long-repeat"));
            Serial.println(F("Hold the button: long gestures repeat while held."));
            break;

        case DEMO_SINGLE_SHOT:
            button.set_long_press_mode(LONG_PRESS_SINGLE_SHOT);
            button.set_long_press_repeat_delay(DEFAULT_LONG_REPEAT_DELAY);
            Serial.println(F("Mode: single-shot long press"));
            Serial.println(F("Hold the button: one long gesture is reported per hold."));
            break;

        case DEMO_FAST_REPEAT:
        default:
            button.set_long_press_mode(LONG_PRESS_REPEAT);
            button.set_long_press_repeat_delay(FAST_REPEAT_MS);
            Serial.print(F("Mode: long-repeat every "));
            Serial.print(FAST_REPEAT_MS);
            Serial.println(F(" ms"));
            Serial.println(F("Hold the button: long gestures repeat faster."));
            break;
    }

    Serial.println(F("Short, double, and triple clicks still work in every mode."));
    button.reset();
}

void setup() {
    Serial.begin(115200);
    uint32_t timer = millis() + 2000;
    while (!Serial && millis() < timer);
    Serial.flush();

    Serial.println(F("\n\nButtonGestures LongPressModes example"));
    Serial.println(F("Wire one button from pin 2 to GND."));
    Serial.println(F("This loop never calls delay(); check_button() must be polled often."));

    applyMode(DEMO_DEFAULT_REPEAT);
}

void loop() {
    const uint32_t now = millis();

    if (elapsed(now, modeStartedAt, MODE_WINDOW_MS)) {
        applyMode(static_cast<uint8_t>((currentMode + 1) % DEMO_MODE_COUNT));
    }

    const uint8_t gesture = button.check_button();
    if (gesture != NOT_PRESSED) {
        printGesture(gesture);
    }
}
