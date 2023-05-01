#include <smartSwitch.h>

smartSwitch SWitch;

int inPin = D3;
int outPin = D1;
int indicPin = D5;

void setup()
{
  Serial.begin(115200);
  Serial.println("\nStart");

  SWitch.set_id(1);                     /* Instances counter- generally don't need to interfere */
  SWitch.set_timeout(5);                /* timeout is optional */
  SWitch.set_name("SW_1");              /* Optional */
  SWitch.set_input(inPin, ON_OFF_SW);   /* Optional, 2-Toggle; 1-Button */
  SWitch.set_output(outPin);            /* Optional, Can be Relay, PWM, or Virual */
  SWitch.set_indiction(indicPin, HIGH); /* Optional */

  SWitch.print_preferences();
}

void loop()
{
  SWitch.loop();
  int a = SWitch.get_remain_time();
  if (a > 0)
  {
    Serial.print("rem: ");
    Serial.println((double)a / 1000, 3);
  }
  delay(50);
}
