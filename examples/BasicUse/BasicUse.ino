/*\
|*| Example use of the ButtonGestures library
|*|
|*| This example shows how to check the button for any gestures.
|*| If a gesture has been entered it will be displayed to the
|*| serial port.
|*|
|*| (c) 2022 trent m. wyatt
|*|
\*/

#include <ButtonGestures.h>

// NOTE: change/define the following pin(s) based on your project/connections
#define   BUTTON_PIN    2

ButtonGestures  button(BUTTON_PIN, LOW, INPUT_PULLUP);

//
// This function will report the passed button state along with a label
//
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
    Serial.begin(115200);
    uint32_t timer = millis() + 2000;
    while (!Serial && millis() < timer);
    Serial.flush();
    Serial.println(F("\n\nArduino Core Library - ButtonGestures Library Test"));
}


void loop(void) {
    report_button(button.check_button(), "push button 1");
}
