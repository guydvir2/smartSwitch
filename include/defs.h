#include <Arduino.h>

#ifndef UNDEF_PIN
#define UNDEF_PIN 255
#endif

#ifndef MAX_TOPIC_SIZE
#define MAX_TOPIC_SIZE 40
#endif

#define SECONDS 1000
#define MINUTES (60 * SECONDS)
#define TimeFactor MINUTES

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
    uint8_t pwm = 255;   /* PWM precentage */
    uint8_t state = 255; /* Up/Down/ Off */
    uint8_t reason = 3;  /* What triggered the button */
    uint8_t pressCount = 0;
    unsigned long clk_end = 0;
};
struct SW_props
{
    uint8_t id = 0;
    uint8_t type = 0;
    uint8_t inpin = UNDEF_PIN;
    uint8_t outpin = UNDEF_PIN;
    uint8_t indicpin = UNDEF_PIN;

    bool PWM = false;
    bool timeout = false;
    bool virtCMD = false;
    bool lockdown = false;
    const char *name;
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

/* "Virtcmd" is defined when output is not defined
   "useTimeout" is defined when set_timeout is set to t !=0
   "useButton" is defined when set_input is defined !=0;
*/