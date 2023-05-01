#include <myIOT2.h>
#include "myIOT_settings.h"
#include <smartSwitch.h>

#define MAX_SW 2
smartSwitch *sw_array[MAX_SW]{};

uint8_t total_sw_counter = 0;
uint8_t input_io_usage_counter = 0;
uint8_t output_io_usage_counter = 0;
const uint8_t input_io_bank[4] = {D3, D2};          /* Wemos Button Shield D3/ IO0*/
const uint8_t output_io_bank[4] = {D1, D4, D5, D7}; /*Wemos Relay Shield D1*/

void init_sw(bool use_input, uint8_t inType,
             bool use_output, uint8_t pwm_intense, bool output_dir,
             bool use_indic = false, bool indic_dir = HIGH,
             int timeout = 0, const char *name = "SW")
{

  sw_array[total_sw_counter] = new smartSwitch;

  sw_array[total_sw_counter]->set_name(name);
  if (timeout > 0)
  {
    sw_array[total_sw_counter]->set_timeout(timeout);
  }
  if (use_input)
  {
    sw_array[total_sw_counter]->set_input(input_io_bank[input_io_usage_counter], inType);
    input_io_usage_counter++;
  }
  if (use_output)
  {
    sw_array[total_sw_counter]->set_output(output_io_bank[output_io_usage_counter], pwm_intense, output_dir);
    output_io_usage_counter++;
  }
  if (use_indic)
  {
    sw_array[total_sw_counter]->set_indiction(output_io_bank[output_io_usage_counter], indic_dir);
    output_io_usage_counter++;
  }

  sw_array[total_sw_counter]->print_preferences();
  total_sw_counter++;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\nStart");

  init_sw(true /*use_input*/, ON_OFF_SW /*input_type*/,
          true /*use_output*/, 0 /*pwm_intense*/, HIGH /* output_on_dir*/,
          true /*use_indic*/, LOW /*indic_dir*/,
          10 /*timeout_sec*/, "SW_0" /*Switch_name MQTT topic*/);

  init_sw(true /*use_input*/, MOMENTARY_SW /*input_type*/,
          true /*use_output*/, 80 /*pwm_intense*/, HIGH /* output_on_dir*/,
          true /*use_indic*/, HIGH /*indic_dir*/,
          0 /*timeout_sec*/, "SW_1" /*Switch_name MQTT topic*/);

  startIOTservices();
}

void loop()
{
  iot.looper();

  for (uint8_t i = 0; i < total_sw_counter; i++)
  {
    if (sw_array[i]->loop())
    {
      const char *trigs[] = {"Button", "Timeout", "MQTT"};
      const char *state[] = {"off", "on"};
      char newmsg[100];
      sprintf(newmsg, "[%s]: Switch[#%d][%s] turned [%s]", trigs[sw_array[i]->telemtryMSG.reason], i, sw_array[i]->name, state[sw_array[i]->telemtryMSG.state]);
      sw_array[i]->clear_newMSG();
      Serial.println(newmsg);
    }
  }
}
