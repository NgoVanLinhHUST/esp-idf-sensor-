#ifndef __WS2812B_H
#define __WS2812B_H

void WS2812B_Init(int tx_pin, int number_led);
void WS2812B_Set_Color(int index, int r, int g, int b);
void WS2812B_Update_Color(void);
#endif