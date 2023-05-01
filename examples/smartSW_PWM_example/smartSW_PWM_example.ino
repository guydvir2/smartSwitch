
#include <smartSwitch.h>

smartSwitch SWitch;

int inPin = D3;
int outPin = D5;
int indicPin = D1;
uint8_t PWM_intense = 30; /* 0-100 */

void setup()
{
  Serial.begin(115200);
  Serial.println("\nStart");

  SWitch.set_id(0); /* Instances counter- generally don't need to interfere */
  // SWitch.set_timeout(5);                  /* timeout is optional */
  SWitch.set_name("pwmSW_1");             /* Optional */
  SWitch.set_input(inPin, 2);             /* Optional, 1-Toggle; 2-Button */
  SWitch.set_output(outPin, PWM_intense); /* Optional, Can be Relay, PWM, or Virual */
  SWitch.set_indiction(indicPin, HIGH);   /* Optional */

  SWitch.print_preferences();
}

void loop()
{
  // SWitch.loop();
  delay(100);
  SWitch.turnOFF_cb(2);
  int a = random(0, 100);
  Serial.println(a);
  SWitch.turnON_cb(2, 0, a);
}
