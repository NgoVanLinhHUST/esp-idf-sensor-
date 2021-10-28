#ifndef LEDC_APP_H
#define LEDC_APP_H

void LedC_Init(void);
void LedC_Add_Pin(int pin, int channel);
void LedC_Set_Duty(int channel ,int duty);
#endif
