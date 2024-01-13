#include <Arduino.h>
#include "smartSwitch.h"

smartSwitch::smartSwitch(bool use_debug) : _inSW(1), _timeout_clk(Chrono::MILLIS)
{
    _id = _next_id++;
    useDebug = use_debug;
}
void smartSwitch::set_id(uint8_t i)
{
    _id = i;
}
void smartSwitch::set_timeout(int t)
{
    if (t > 0)
    {
        _use_timeout = true;
        _DEFAULT_TIMEOUT_DUARION = t * TimeFactor; /* default timeout */
        _stop_timeout();
    }
    else
    {
        _use_timeout = false;
    }
}
void smartSwitch::set_additional_timeout(int t, uint8_t type)
{
    if (_use_timeout)
    {
        if (get_remain_time() > 0) /* adding time in case timeout is ON */
        {
            _adHoc_timeout_duration += t * TimeFactor;
            telemtryMSG.clk_end = _adHoc_timeout_duration;
        }
        else
        {
            _adHoc_timeout_duration = t;
            turnON_cb(type, _adHoc_timeout_duration);
        }
    }
}
void smartSwitch::set_name(const char *Name)
{
    strlcpy(name, Name, MAX_TOPIC_SIZE);
}
void smartSwitch::set_output(uint8_t outpin, uint8_t intense, bool dir, bool onBoot)
{
    OUTPUT_ON = dir;
    _onBoot = onBoot;
    if (outpin == UNDEF_PIN) // VirtCmd
    {
        _virtCMD = true;
    }
    else
    {
        _outputPin = outpin;
        pinMode(outpin, OUTPUT);
        if (intense > 0 && intense <= 100) /* PWM OUTOUT defined by intense >0 */
        {
            _output_pwm = true;
            _DEFAULT_PWM_INTENSITY = intense;
            telemtryMSG.pwm = _DEFAULT_PWM_INTENSITY;
            telemtryMSG.state = 0;

#if defined(ESP8266)
            analogWriteRange(1023);
#endif
        }
        else /* Relay Output*/
        {
            _output_pwm = false;
            telemtryMSG.pwm = _DEFAULT_PWM_INTENSITY;
            digitalRead(_outputPin) == OUTPUT_ON ? telemtryMSG.state = SW_ON : telemtryMSG.state = SW_OFF;
        }
    }
}
void smartSwitch::set_input(uint8_t inpin, uint8_t t, bool dir)
{
    _button_type = t;
    BUTTON_PRESSED = dir;

    if (_button_type == NO_INPUT || inpin == UNDEF_PIN) /* No Input */
    {
        _useButton = false;
    }
    else if (inpin != UNDEF_PIN && _button_type > NO_INPUT) /* Use input */
    {
        _useButton = true;
        _inSW.set_debounce(50);
        if (_button_type == MULTI_PRESS_BUTTON) /* Multi Press */
        {
            // MultiPress Button is set up as Momentary SW
            _inSW.add_switch(MOMENTARY_SW, inpin, circuit_C2); /* pullup input */
        }
        else /* Button or Toggle */
        {
            _inSW.add_switch(_button_type, inpin, circuit_C2); /* pullup input */
        }
    }
    else
    {
        DBGL("Input definition Error");
        yield();
    }
}
void smartSwitch::set_lockSW()
{
    _in_lockdown = true;
}
void smartSwitch::set_unlockSW()
{
    _in_lockdown = false;
}
void smartSwitch::set_useLockdown(bool t)
{
    _use_lockdown = t;
}
void smartSwitch::set_indiction(uint8_t pin, bool dir)
{
    if (pin != UNDEF_PIN)
    {
        _use_indic = true;
        _indicPin = pin;
        _indic_on = dir;
        pinMode(_indicPin, OUTPUT);
    }
}
void smartSwitch::init_lockdown()
{
    if (_use_lockdown)
    {
        _in_lockdown = true;
    }
}
void smartSwitch::release_lockdown()
{
    if (_use_lockdown)
    {
        _in_lockdown = false;
    }
}

void smartSwitch::turnON_cb(uint8_t type, unsigned int temp_TO, uint8_t intense)
{
    if (!_in_lockdown)
    {
        if (!_virtCMD)
        {
            unsigned long _t = 0;
            if (!_isOUTPUT_ON())
            {
                telemtryMSG.clk_start = millis();
                _setOUTPUT_ON(intense == 255 ? _DEFAULT_PWM_INTENSITY : intense); /* Both PWM and Switch */
                if (_use_timeout)
                {
                    _t = _calc_timeout(temp_TO); /* defualt or adhoc*/
                    _start_timeout_clock();
                }
                telemtryMSG.clk_end = _t;
                _update_telemetry(SW_ON, type, intense == 255 ? _DEFAULT_PWM_INTENSITY : intense);
            }
        }
    }
    else
    {
        if (_guessState == SW_OFF)
        {
            _start_timeout_clock();
            _guessState = !_guessState;
            telemtryMSG.clk_end = get_remain_time();
            _update_telemetry(SW_ON, type);
        }
        else
        {
            yield();
        }
    }
}
void smartSwitch::turnOFF_cb(uint8_t type)
{
    if (!_in_lockdown)
    {
        if (!_virtCMD)
        {
            if (_isOUTPUT_ON())
            {
                _setOUTPUT_OFF();
                _stop_timeout();
                _update_telemetry(SW_OFF, type, 0);
            }
            else
            {
                DBG(F("SW#:"));
                DBG(_id);
                DBGL(F(": Already off"));
            }
        }
        else
        {
            if (_guessState == SW_ON)
            {
                _stop_timeout();
                _guessState = !_guessState;
                _update_telemetry(SW_OFF, type, 0);
            }
            else
            {
                yield();
            }
        }
    }
}
unsigned long smartSwitch::get_remain_time()
{
    if (_timeout_clk.isRunning() && _use_timeout)
    {
        return _adHoc_timeout_duration == 0 ? _DEFAULT_TIMEOUT_DUARION - _timeout_clk.elapsed() : _adHoc_timeout_duration - _timeout_clk.elapsed();
    }
    else
    {
        return 0;
    }
}
unsigned long smartSwitch::get_elapsed()
{
    return _timeout_clk.elapsed();
}
unsigned long smartSwitch::get_timeout()
{
    return _adHoc_timeout_duration == 0 ? _DEFAULT_TIMEOUT_DUARION : _adHoc_timeout_duration;
}
uint8_t smartSwitch::get_SWstate() /* get output state */
{
    if (!_virtCMD)
    {
        return _isOUTPUT_ON();
    }
    else
    {
        return 255;
    }
}
void smartSwitch::get_SW_props(SW_props &props)
{
    props.id = _id;
    props.type = _button_type;
    props.inpin = _inSW.switches[0].switch_pin;
    props.outpin = _outputPin;
    props.indicpin = _indicPin;
    props.TO_dur = _DEFAULT_TIMEOUT_DUARION;
    props.timeout = _use_timeout;
    props.virtCMD = _virtCMD;
    props.lockdown = _use_lockdown;
    props.PWM_intense = _DEFAULT_PWM_INTENSITY;
    props.name = name;
    props.onBoot = _onBoot;
}
void smartSwitch::print_preferences()
{
    DBG(F("\n >>>>>> Switch #"));
    DBG(_id);
    DBGL(F(" <<<<<< "));

    DBG(F("Output Type :\t"));
    DBGL(_virtCMD ? "Virtual" : "Real-Switch");
    DBG(F("Name:\t"));
    DBGL(name);

    DBG(F("input type:\t"));
    DBG(_button_type);
    DBGL(F(" ; 0:None; 1:Button, 2:Toggle"));
    DBG(F("input_pin:\t"));
    if (_button_type != 0)
    {
        DBGL(_inSW.switches[0].switch_pin);
    }
    else
    {
        DBGL(F("None"));
    }
    DBG(F("outout_pin:\t"));
    DBGL(_outputPin);
    DBG(F("isPWM:\t\t"));
    DBGL(_output_pwm == 0 ? "No" : "Yes");
    DBG(F("use indic:\t"));
    DBGL(_use_indic ? "Yes" : "No");
    if (_use_indic)
    {
        DBG(F("indic_Pin:\t"));
        DBGL(_indicPin);
    }

    DBG(F("use timeout:\t"));
    DBGL(_use_timeout ? "Yes" : "No");

    if (_DEFAULT_TIMEOUT_DUARION > 0)
    {
        DBG(F("timeout [sec]:\t"));
        DBGL(_DEFAULT_TIMEOUT_DUARION / TimeFactor);
    }

    DBG(F("use lockdown:\t"));
    DBGL(_use_lockdown ? "YES" : "NO");

    DBGL(F(">>>>>>>> END <<<<<<<< \n"));
}

bool smartSwitch::loop()
{
    bool not_in_lockdown = (_use_lockdown && !_in_lockdown) || (!_use_lockdown);

    if (_useButton && not_in_lockdown && _inSW.read_switch(0) == switched) /* Input change*/
    {
        _button_loop();
    }

    if (_use_timeout && not_in_lockdown)
    {
        _timeout_loop();
    }

    if (_use_indic)
    {
        _indic_loop();
    }

    return telemtryMSG.newMSG;
}
void smartSwitch::clear_newMSG()
{
    telemtryMSG.newMSG = false;
}

bool smartSwitch::useTimeout()
{
    return _use_timeout;
}
bool smartSwitch::is_virtCMD()
{
    return _virtCMD;
}
bool smartSwitch::is_useButton()
{
    return _useButton;
}

uint8_t smartSwitch::_next_id = 0;

bool smartSwitch::_isOUTPUT_ON()
{
    if (_output_pwm)
    {
        return _PWM_ison;
    }
    else
    {
        return (digitalRead(_outputPin) == OUTPUT_ON);
    }
}
void smartSwitch::_setOUTPUT_OFF()
{
    if (_output_pwm)
    {
        analogWrite(_outputPin, 0);
        _PWM_ison = false;
    }
    else
    {
        digitalWrite(_outputPin, !OUTPUT_ON);
    }
    DBG(F("SW#:"));
    DBG(_id);
    DBGL(F(": OUTPUT_OFF"));
}
void smartSwitch::_setOUTPUT_ON(uint8_t val)
{
    if (_output_pwm)
    {
        int res = 0;
#if defined(ESP8266)
        res = 255;
#elif defined(ESP32)
        res = 4095;
#endif
        analogWrite(_outputPin, (res * val) / 100);
        _PWM_ison = true;
        DBG(F("SW#:"));
        DBG(_id);
        DBGL(F("PWM_ON"));
        DBG(F("PWM_Value: "));
        DBGL((res * val) / 100);
    }
    else
    {
        digitalWrite(_outputPin, OUTPUT_ON);
        DBG(F("SW#:"));
        DBG(_id);
        DBGL(F(": OUTPUT_ON"));
    }
}
void smartSwitch::_button_loop()
{
    if (_inSW.switches[0].switch_type == toggle_switch) /* Toggle only */
    {
        DBG(F("SW#:"));
        DBG(_id);
        DBGL(F(": TOGGLE"));

        if (_inSW.switches[0].switch_status == !on && (get_SWstate() == 1 || (get_SWstate() == 255 && _guessState == SW_ON))) /* Toggle Off */
        {
            turnOFF_cb(BUTTON_INPUT);
        }
        else if (_inSW.switches[0].switch_status == on && (get_SWstate() == 0 || (get_SWstate() == 255 && _guessState == SW_OFF))) /* Toggle On */
        {
            turnON_cb(BUTTON_INPUT);
        }
        else if (_inSW.switches[0].switch_status == !on && (get_SWstate() == 0)) /* Toggled Off - but was Off by timeout */
        {
            yield();
            DBG(F("SW#:"));
            DBG(_id);
            DBGL(F(": WAS ALREADY OFF (PROB_TIMER)"));
        }
        else
        {
            yield();
            DBG(F("SW#:"));
            DBG(_id);
            DBGL(F(": ERR1"));
        }
    }
    else /* Button - single & multiPress types */
    {
        const int _time_between_presses = 2000;
        DBG(F("SW#:"));
        DBG(_id);
        DBGL(F(": BUTTON_PRESS"));

        if (get_SWstate()) /* Is output ON ? */
        {
            if (_button_type == MOMENTARY_SW)
            {
                turnOFF_cb(BUTTON_INPUT);
            }
            else if (_button_type == MULTI_PRESS_BUTTON)
            {
                if (_last_button_press != 0 && millis() - _last_button_press > _time_between_presses) /* press after time- turns off*/
                {
                    _multiPress_counter = 0;
                    _last_button_press = 0;
                    turnOFF_cb(BUTTON_INPUT);
                }
                else if (_last_button_press != 0 && millis() - _last_button_press < _time_between_presses) /* inc counter */
                {
                    _multiPress_counter++;
                    _last_button_press = millis();

                    _update_telemetry(SW_ON, BUTTON_INPUT, telemtryMSG.pwm);
                }
                else
                {
                    /* any error ?*/
                    yield();
                    DBG(F("SW#:"));
                    DBG(_id);
                    DBGL(F(" ERR2"));
                }
            }
        }
        else
        {
            _multiPress_counter = 1;
            _last_button_press = millis();
            turnON_cb(BUTTON_INPUT); /* Momentary & MultiPress */
        }
    }
}
void smartSwitch::_indic_loop()
{
    if (_isOUTPUT_ON())
    {
        _turn_indic_on();
    }
    else
    {
        _turn_indic_off();
    }
}
void smartSwitch::_timeout_loop()
{
    if (_timeout_clk.isRunning())
    {
        if (_adHoc_timeout_duration != 0 && _timeout_clk.hasPassed(_adHoc_timeout_duration)) /* ad-hoc timeout*/
        {
            turnOFF_cb(SW_TIMEOUT);
        }
        else if (_adHoc_timeout_duration == 0 && _timeout_clk.hasPassed(_DEFAULT_TIMEOUT_DUARION)) /* preset timeout */
        {
            turnOFF_cb(SW_TIMEOUT);
        }
    }
}
void smartSwitch::_turn_indic_on()
{
    digitalWrite(_indicPin, _indic_on);
    telemtryMSG.indic_state = _indic_on;
}
void smartSwitch::_turn_indic_off()
{
    digitalWrite(_indicPin, !_indic_on);
    telemtryMSG.indic_state = !_indic_on;
}
void smartSwitch::_stop_timeout()
{
    if (_use_timeout)
    {
        _timeout_clk.stop();
        _adHoc_timeout_duration = 0;
        DBG(F("SW#:"));
        DBG(_id);
        DBGL(F(" TIMEOUT_STOPPED"));
    }
}
void smartSwitch::_start_timeout_clock()
{
    if (_use_timeout)
    {
        _timeout_clk.stop();
        _timeout_clk.start();
        telemtryMSG.clk_start = millis();

        DBG(F("SW#:"));
        DBG(_id);
        DBGL(F(" TIMEOUT_START"));
    }
}
unsigned long smartSwitch::_calc_timeout(int t)
{
    if (t != 0) /* timeout was defined - not using default timeout */
    {
        _adHoc_timeout_duration = t * TimeFactor; /* Define timeout in millis */
        return _adHoc_timeout_duration;
    }
    else
    {
        return _DEFAULT_TIMEOUT_DUARION;
    }
}
void smartSwitch::_update_telemetry(uint8_t state, uint8_t type, uint8_t pwm)
{
    telemtryMSG.newMSG = true;
    telemtryMSG.state = state;
    telemtryMSG.reason = type;
    telemtryMSG.pressCount = _multiPress_counter;
    telemtryMSG.pwm = pwm;

    DBG(F("SW#:"));
    DBG(_id);
    DBGL(F(" TELEMETRY_UPDATE"));
}
