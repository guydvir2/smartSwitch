#ifndef swmartSW_h
#define smartSW_h
#include <Arduino.h>
#include <Chrono.h>
#include <ez_switch_lib.h>

#include "defs.h" /* Libraries complementary definitions */

class smartSwitch
{
public:
    bool useDebug = false;
    bool OUTPUT_ON = HIGH;     /* configurable */
    bool BUTTON_PRESSED = LOW; /* configurable */

    const char *ver = "smartSwitch_v0.75a";
    char name[MAX_TOPIC_SIZE];
    SW_act_telem telemtryMSG;

    smartSwitch(bool use_debug=false);
    void set_id(uint8_t i);
    void set_timeout(int t = 0);
    void set_name(const char *Name = "smartSW");
    void set_additional_timeout(int t, uint8_t type);
    void set_indiction(uint8_t pin = UNDEF_PIN, bool dir = 0);
    void set_input(uint8_t inpin = UNDEF_PIN, uint8_t t = 0, bool dir = LOW);
    void set_output(uint8_t outpin = UNDEF_PIN, uint8_t intense = 0, bool dir = HIGH);

    void set_lockSW();
    void set_unlockSW();

    void set_useLockdown(bool t = true);
    void init_lockdown();
    void release_lockdown();

    void turnON_cb(uint8_t type, unsigned int temp_TO = 0, uint8_t intense = 255);
    void turnOFF_cb(uint8_t type);
    void clear_newMSG();
    bool loop();

    uint8_t get_SWstate();
    unsigned long get_remain_time();
    unsigned long get_elapsed();
    unsigned long get_timeout();
    void get_SW_props(SW_props &props);
    void print_preferences();

    bool useTimeout();
    bool is_virtCMD();
    bool is_useButton();

private:
    uint8_t _ez_sw_id = 0;
    uint8_t _DEFAULT_PWM_INTENSITY = 0;
    uint8_t _button_type = 255;
    uint8_t _outputPin = UNDEF_PIN;
    uint8_t _indicPin = UNDEF_PIN;
    uint8_t _multiPress_counter = 0;

    uint8_t _id = 0;
    static uint8_t _next_id; /* Instance counter */

    bool _virtCMD = false;
    bool _useButton = false;
    bool _guessState = false;
    bool _use_timeout = false;
    bool _use_lockdown = false;
    bool _use_indic = false;
    bool _in_lockdown = false;
    bool _indic_on = false;
    bool _PWM_ison = false;
    bool _output_pwm = false;

    Switches _inSW;
    Chrono _timeout_clk;

    /* inputs only */
    unsigned long _DEFAULT_TIMEOUT_DUARION = 1; // in seconds
    unsigned long _adHoc_timeout_duration = 0;  // in seconds
    unsigned long _last_button_press = 0;

private:
    bool _isOUTPUT_ON();
    void _setOUTPUT_ON(uint8_t val = 255);
    void _setOUTPUT_OFF();
    void _button_loop();
    void _indic_loop();
    void _timeout_loop();
    void _stop_timeout();
    void _turn_indic_on();
    void _turn_indic_off();
    void _start_timeout_clock();
    void _update_telemetry(uint8_t state, uint8_t type, unsigned long te = 0, uint8_t pwm = 255);
};

#endif
