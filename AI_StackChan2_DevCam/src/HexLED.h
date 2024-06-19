#ifndef _HEX_LED_H_
#define _HEX_LED_H_

#include <FastLED.h>
#define NUM_LEDS 37
//#define LED_DATA_PIN 9    //PORTB
#define LED_DATA_PIN 2    //CoreS3 PORTA
#define LED_BRIGHTNESS 8

void hex_led_init(void);
void hex_led_ptn_off(void);
void hex_led_ptn_wake(void);
void hex_led_ptn_accept(void);
void hex_led_ptn_notification(void);
void hex_led_ptn_boot(void);
void hex_led_ptn_thinking_start(void);
void hex_led_ptn_thinking_end(void);
//電源OFF時のLED点灯シーケンス
void hex_led_ptn_pow_off_1of5(void);
void hex_led_ptn_pow_off_2of5(void);
void hex_led_ptn_pow_off_3of5(void);
void hex_led_ptn_pow_off_4of5(void);
void hex_led_ptn_pow_off_5of5(void);
void hex_led_ptn_test(void);
//LED点灯パターン
void hex_led_ptn_Np(int patternNum);

#endif //_HEX_LED_H_