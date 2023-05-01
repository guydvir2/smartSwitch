#include <Arduino.h>
#include "smartSwitch.h"

smartSwitch::smartSwitch() : _inSW(1),
                             _timeout_clk(Chrono::MILLIS)
{
    _id = _next_id++;
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
        if (_adHoc_timeout_duration != 0)
        {
            _adHoc_timeout_duration += t * TimeFactor;
        }
        else if (_adHoc_timeout_duration == 0 && _DEFAULT_TIMEOUT_DUARION != 0)
        {
            _adHoc_timeout_duration += t * TimeFactor + _DEFAULT_TIMEOUT_DUARION;
        }
        if (!get_remain_time())
        {
            turnON_cb(type, _adHoc_timeout_duration / 1000);
        }
        else
        {
            _update_telemetry(SW_ON, type, _adHoc_timeout_duration, telemtryMSG.pwm);
        }
    }
}
void smartSwitch::set_name(const char *Name)
{
    strlcpy(name, Name, MAX_TOPIC_SIZE);
}
void smartSwitch::set_output(uint8_t outpin, uint8_t intense, bool dir)
{
    OUTPUT_ON = dir;
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
            telemtryMSG.pwm = 0;
            telemtryMSG.state = digitalRead(_outputPin);
        }
    }
}

void smartSwitch::set_input(uint8_t inpin, uint8_t t, bool dir)
{
    _button_type = t;
    BUTTON_PRESSED = dir;

    if (_button_type == NO_INPUT || inpin == UNDEF_PIN)
    {
        _useButton = false;
    }
    else if (inpin != UNDEF_PIN && _button_type > NO_INPUT)
    {
        _useButton = true;
        _inSW.set_debounce(50);
        if (_button_type == MULTI_PRESS_BUTTON)
        {
            // MultiPress Button is set up as Momentary SW
            _ez_sw_id = _inSW.add_switch(MOMENTARY_SW, inpin, circuit_C2); /* pullup input */
        }
        else
        {
            _ez_sw_id = _inSW.add_switch(_button_type, inpin, circuit_C2); /* pullup input */
        }
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
            /* Turn ON */
            if (!_isOUTPUT_ON())
            {
                _setOUTPUT_ON(intense); /* Both PWM and Switch */
            }
            else
            {
                DBGL(F("Already on"));
                return;
            }
            /* Timeout */
            unsigned long _t = 0;
            if (_use_timeout)
            {
                if (temp_TO != 0) /* timeout was defined - not using default timeout */
                {
                    _adHoc_timeout_duration = temp_TO * TimeFactor; /* Define timeout */
                    _t = _adHoc_timeout_duration;
                }
                else
                {
                    _t = _DEFAULT_TIMEOUT_DUARION;
                }
                _start_timeout_clock(); /* start clock - no matter if it is default or not */
            }
            _update_telemetry(SW_ON, type, _t, intense == 255 ? _DEFAULT_PWM_INTENSITY : intense);
        }
        else
        {
            if (_guessState == SW_OFF)
            {
                _start_timeout_clock();
                _guessState = !_guessState;
                _update_telemetry(SW_ON, type, get_remain_time());
            }
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
                _update_telemetry(SW_OFF, type, 0, 0);
            }
            else
            {
                DBGL(F("Already off"));
            }
        }
        else
        {
            if (_guessState == SW_ON)
            {
                _stop_timeout();
                _guessState = !_guessState;
                _update_telemetry(SW_OFF, type, 0, 0);
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
uint8_t smartSwitch::get_SWstate()
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
    props.inpin = _inSW.switches[_ez_sw_id].switch_pin;
    props.outpin = _outputPin;
    props.timeout = _use_timeout;
    props.virtCMD = _virtCMD;
    props.lockdown = _use_lockdown;
    props.PWM = _output_pwm;
    props.name = name;
}
void smartSwitch::print_preferences()
{
    DBG(F("\n >>>>>> Switch #"));
    DBGL(_id);
    DBGL(F(" <<<<<< "));

    DBG(F("Output Type :\t"));
    DBGL(_virtCMD ? "Virtual" : "Real-Switch");
    DBG(F("Name:\t"));
    DBGL(name);

    DBG(F("input type:\t"));
    DBGL(_button_type);
    DBGL(F(" ; 0:None; 1:Button, 2:Toggle"));
    DBG(F("input_pin:\t"));
    DBGL(_inSW.switches[_ez_sw_id].switch_pin);
    DBG(F("outout_pin:\t"));
    DBGL(_outputPin);
    DBG(F("isPWM:\t"));
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

    DBGL(F(" >>>>>>>> END <<<<<<<< \n"));
}

bool smartSwitch::loop()
{
    bool not_in_lockdown = (_use_lockdown && !_in_lockdown) || (!_use_lockdown);

    if (_useButton && not_in_lockdown && _inSW.read_switch(_ez_sw_id) == switched) /* Input change*/
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
    DBGL(F("OUTPUT_OFF"));
}
void smartSwitch::_setOUTPUT_ON(uint8_t val)
{
    if (_output_pwm)
    {
        int res = 0;
#if defined(ESP8266)
        res = 1023;
#elif defined(ESP32)
        res = 4097;
#endif
        int _val = val == 255 ? (res * _DEFAULT_PWM_INTENSITY) / 100 : (res * val) / 100;
        analogWrite(_outputPin, _val);
        _PWM_ison = true;
        DBGL(F("PWM_ON"));
    }
    else
    {
        digitalWrite(_outputPin, OUTPUT_ON);
        DBGL(F("OUTPUT_ON"));
    }
}
void smartSwitch::_button_loop()
{
    /* For Toggle only */
    if (_inSW.switches[_ez_sw_id].switch_type == toggle_switch)
    {
        DBGL(F("TOGGLE"));

        if (_inSW.switches[_ez_sw_id].switch_status == !on && (get_SWstate() == 1 || (get_SWstate() == 255 && _guessState == SW_ON)))
        {
            turnOFF_cb(BUTTON_INPUT);
        }
        else if (_inSW.switches[_ez_sw_id].switch_status == on && (get_SWstate() == 0 || (get_SWstate() == 255 && _guessState == SW_OFF)))
        {
            turnON_cb(BUTTON_INPUT);
        }
        else
        {
            yield();
            DBGL(F("ERR1"));
        }
    }
    /* For Button only */
    else
    {
        const int _time_between_presses = 2000;
        DBGL(F("BUTTON_PRESS"));
        /* Is output ON ? */
        if (get_SWstate())
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
                    _update_telemetry(SW_ON, BUTTON_INPUT, telemtryMSG.clk_end, telemtryMSG.pwm);
                }
                else
                {
                    /* any error ?*/
                    yield();
                    DBGL(F("ERR2"));
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
}
void smartSwitch::_turn_indic_off()
{
    digitalWrite(_indicPin, !_indic_on);
}
void smartSwitch::_stop_timeout()
{
    if (_use_timeout)
    {
        _timeout_clk.stop();
        _adHoc_timeout_duration = 0;
        DBGL(F("TIMEOUT_STOPPED"));
    }
}
void smartSwitch::_start_timeout_clock()
{
    if (_use_timeout)
    {
        _timeout_clk.stop();
        _timeout_clk.start();
        DBGL(F("TIMEOUT_START"));
    }
}
void smartSwitch::_update_telemetry(uint8_t state, uint8_t type, unsigned long te, uint8_t pwm)
{
    telemtryMSG.newMSG = true;
    telemtryMSG.state = state;
    telemtryMSG.reason = type;
    telemtryMSG.clk_end = te;
    telemtryMSG.pressCount = _multiPress_counter;
    telemtryMSG.pwm = pwm;
    DBGL(F("TELEMETRY_UPDATE"));
}
