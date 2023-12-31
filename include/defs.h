#include <Arduino.h>

#ifndef UNDEF_PIN
#define UNDEF_PIN 255
#endif

#ifndef MAX_TOPIC_SIZE
#define MAX_TOPIC_SIZE 40
#endif

#define SECONDS 1000
#define MINUTES (60 * SECONDS)
#define TimeFactor SECONDS

#ifndef DBG
#define DBG(a)           \
    if (useDebug)        \
    {                    \
        Serial.print(a); \
    }
#endif
#ifndef DBGL
#define DBGL(a)            \
    if (useDebug)          \
    {                      \
        Serial.println(a); \
    }
#endif

struct SW_act_telem
{
    bool newMSG = false;
    bool lockdown = false;
    bool indic_state = false;
    uint8_t pwm = 255;   /* PWM precentage */
    uint8_t state = 255; /* Up/Down/ Off */
    uint8_t reason = 3;  /* What triggered the button */
    uint8_t pressCount = 0;
    unsigned long clk_end = 0;
    unsigned long clk_start = 0;
};

enum SWTypes : const uint8_t
{
    NO_INPUT,
    MOMENTARY_SW,
    ON_OFF_SW,
    MULTI_PRESS_BUTTON
};
enum InputTypes : const uint8_t
{
    BUTTON_INPUT,
    SW_TIMEOUT,
    EXT_0,
    EXT_1
};
enum SWstates : const uint8_t
{
    SW_OFF,
    SW_ON
};
struct SW_props
{
    uint8_t id = 0;
    uint8_t type = NO_INPUT;
    uint8_t inpin = UNDEF_PIN;
    uint8_t outpin = UNDEF_PIN;
    uint8_t indicpin = UNDEF_PIN;
    uint8_t PWM_intense = 0;

    int TO_dur = 0;
    const char *name;

    bool timeout = false;
    bool virtCMD = false;
    bool lockdown = false;
    bool outputON = HIGH;
    bool inputPressed = LOW;
};

/* "Virtcmd" is defined when output is not defined
   "useTimeout" is defined when set_timeout is set to t !=0
   "useButton" is defined when set_input is defined !=0;
*/