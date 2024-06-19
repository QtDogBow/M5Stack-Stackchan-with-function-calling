#include "HexLED.h"
#include <math.h>

CRGB leds[NUM_LEDS];
CRGB leds_rotate_tmp[NUM_LEDS];

#define C0 (CRGB::Black)
#define C1 (CRGB::Blue)
#define C2 (CRGB::Green)
#define C3 (CRGB::DarkOrange)
#define C4 (CRGB::Red)
#define C5 (CRGB::WhiteSmoke)
#define C6 (CRGB::DimGray)
#define C7 (0x202020)


const CRGB led_pattern00[NUM_LEDS] = {
         C0,C0,C0,C0,
       C0,C0,C0,C0,C0,
      C0,C0,C0,C0,C0,C0,
    C0,C0,C0,C0,C0,C0,C0,
      C0,C0,C0,C0,C0,C0,
       C0,C0,C0,C0,C0,
         C0,C0,C0,C0
};

const CRGB led_pattern01[NUM_LEDS] = {
         C1,C2,C1,C2,
       C2,C0,C0,C0,C1,
      C1,C0,C0,C0,C0,C2,
    C2,C0,C0,C0,C0,C0,C1,
      C1,C0,C0,C0,C0,C2,
       C2,C0,C0,C0,C1,
         C1,C2,C1,C2
};

const CRGB led_pattern02[NUM_LEDS] = {
         C2,C1,C2,C1,
       C1,C0,C0,C0,C2,
      C2,C0,C0,C0,C0,C1,
    C1,C0,C0,C0,C0,C0,C2,
      C2,C0,C0,C0,C0,C1,
       C1,C0,C0,C0,C2,
         C2,C1,C2,C1
};

const CRGB led_pattern03[NUM_LEDS] = {
         C3,C3,C3,C3,
       C3,C0,C0,C0,C3,
      C3,C0,C0,C0,C0,C3,
    C3,C0,C0,C0,C0,C0,C3,
      C3,C0,C0,C0,C0,C3,
       C3,C0,C0,C0,C3,
         C3,C3,C3,C3
};
//2024.05.23 電源ボタン 〇〇〇〇
const CRGB led_pattern04[NUM_LEDS] = {
    C1,C1,C1,C1,
   C1,C0,C0,C0,C1,
 C0,C0,C0,C0,C0,C1,
C1,C1,C1,C1,C0,C0,C1,
  C0,C0,C0,C0,C0,C1,
   C1,C0,C0,C0,C1,
     C1,C1,C1,C1
};
//2024.05.23 電源ボタン ◎〇〇〇
const CRGB led_pattern05[NUM_LEDS] = {
    C1,C1,C1,C1,
   C1,C0,C0,C0,C1,
 C0,C0,C0,C0,C0,C1,
C4,C1,C1,C1,C0,C0,C1,
  C0,C0,C0,C0,C0,C1,
   C1,C0,C0,C0,C1,
     C1,C1,C1,C1
};
//2024.05.23 電源ボタン 〇◎〇〇
const CRGB led_pattern06[NUM_LEDS] = {
    C1,C1,C1,C1,
   C1,C0,C0,C0,C1,
 C0,C0,C0,C0,C0,C1,
C1,C4,C1,C1,C0,C0,C1,
  C0,C0,C0,C0,C0,C1,
   C1,C0,C0,C0,C1,
     C1,C1,C1,C1
};
//2024.05.23 電源ボタン 〇〇◎〇
const CRGB led_pattern07[NUM_LEDS] = {
    C1,C1,C1,C1,
   C1,C0,C0,C0,C1,
 C0,C0,C0,C0,C0,C1,
C1,C1,C4,C1,C0,C0,C1,
  C0,C0,C0,C0,C0,C1,
   C1,C0,C0,C0,C1,
     C1,C1,C1,C1
};
//2024.05.23 電源ボタン 〇〇〇◎
const CRGB led_pattern08[NUM_LEDS] = {
    C1,C1,C1,C1,
   C1,C0,C0,C0,C1,
 C0,C0,C0,C0,C0,C1,
C1,C1,C1,C4,C0,C0,C1,
  C0,C0,C0,C0,C0,C1,
   C1,C0,C0,C0,C1,
     C1,C1,C1,C1
};
//2024.05.24 テスト
const CRGB led_pattern99[NUM_LEDS] = {
    C4,C4,C4,C4,
   C0,C4,C4,C4,C0,
 C0,C0,C4,C4,C0,C0,
C0,C0,C0,C0,C0,C0,C0,
  C0,C0,C0,C0,C0,C0,
   C0,C0,C0,C0,C0,
     C0,C0,C0,C0
};
//2024.06.13 数字の1
const CRGB led_patternN1[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C0,C0,C2,C2,
 C0,C0,C0,C0,C2,C0,
C0,C0,C0,C0,C2,C0,C0,
  C0,C0,C0,C2,C0,C0,
   C0,C0,C2,C0,C0,
     C2,C2,C2,C0
};
//2024.06.13 数字の2
const CRGB led_patternN2[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C2,C2,C2,C0,
 C0,C2,C0,C0,C2,C0,
C0,C0,C0,C0,C2,C0,C0,
  C0,C0,C0,C2,C0,C0,
   C0,C0,C2,C0,C0,
     C2,C2,C2,C2
};
//2024.06.13 数字の3
const CRGB led_patternN3[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C2,C2,C2,C0,
 C0,C2,C0,C0,C2,C0,
C0,C0,C0,C2,C2,C0,C0,
  C0,C0,C0,C0,C2,C0,
   C2,C0,C0,C2,C0,
     C2,C2,C2,C0
};
//2024.06.13 数字の4
const CRGB led_patternN4[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C2,C0,C0,C2,
 C0,C2,C0,C0,C2,C0,
C0,C2,C2,C2,C2,C2,C2,
  C0,C0,C0,C2,C0,C0,
   C0,C0,C2,C0,C0,
     C0,C2,C0,C0};
//2024.06.13 数字の5
const CRGB led_patternN5[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C2,C2,C2,C2,
 C0,C2,C0,C0,C0,C0,
C0,C2,C2,C2,C2,C0,C0,
  C0,C0,C0,C0,C2,C0,
   C0,C0,C0,C2,C0,
     C2,C2,C2,C0
};
//2024.06.13 数字の6
const CRGB led_patternN6[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C2,C2,C2,C0,
 C0,C2,C0,C0,C0,C0,
C0,C2,C2,C2,C2,C0,C0,
  C2,C0,C0,C0,C2,C0,
   C2,C0,C0,C2,C0,
     C2,C2,C2,C0
};
//2024.06.13 数字の7
const CRGB led_patternN7[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C2,C2,C2,C2,
 C0,C2,C0,C0,C2,C0,
C0,C0,C0,C0,C2,C0,C0,
  C0,C0,C0,C2,C0,C0,
   C0,C0,C2,C0,C0,
     C0,C2,C0,C0
};
//2024.06.13 数字の8
const CRGB led_patternN8[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C0,C2,C2,C2,
 C0,C0,C2,C0,C0,C2,
C0,C0,C2,C2,C2,C2,C0,
  C0,C2,C0,C0,C2,C0,
   C2,C0,C0,C2,C0,
     C2,C2,C2,C0
};
//2024.06.13 数字の9
const CRGB led_patternN9[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C0,C2,C2,C2,
 C0,C0,C2,C0,C0,C2,
C0,C0,C2,C2,C2,C2,C0,
  C0,C0,C0,C0,C2,C0,
   C2,C0,C0,C2,C0,
     C2,C2,C2,C0
};
//2024.06.13 数字の0
const CRGB led_patternN0[NUM_LEDS] = {
    C0,C0,C0,C0,
   C0,C0,C2,C2,C2,
 C0,C0,C2,C0,C0,C2,
C0,C0,C2,C0,C0,C2,C0,
  C0,C2,C0,C0,C2,C0,
   C2,C0,C0,C2,C0,
     C2,C2,C2,C0
};

// ※回転させたいパターンはconstにしない
CRGB led_rotate_pattern01[NUM_LEDS] = {
         C0,C0,C0,C0,
       C0,C5,C5,C5,C0,
      C0,C5,C0,C0,C0,C0,
    C0,C6,C0,C0,C0,C0,C0,
      C0,C6,C0,C0,C7,C0,
       C0,C6,C7,C7,C0,
         C0,C0,C0,C0
};


#define SCROLL_LINES  (7)
#define SCROLL_COLUMNS (52)
#define SCROLL_MARGIN (7)

const CRGB led_scroll_pattern01[SCROLL_LINES][SCROLL_COLUMNS] = {
  // ↓↓↓ LED_Scroll_Pattern.xlsm で変換したコードをここに貼る ↓↓↓
	{		0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	},
	{	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000		},
	{		0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0x305496	,	0x000000	,	0x305496	,	0x000000	,	0xFF0000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x548235	,	0x548235	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	},
	{	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x305496	,	0x305496	,	0x000000	,	0xFF0000	,	0x000000	,	0x000000	,	0x548235	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x548235	,	0x000000	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000		},
	{		0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x000000	,	0x548235	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	},
	{	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x548235	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000		},
	{		0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0xFF0000	,	0xFF0000	,	0x000000	,	0xFF0000	,	0xFF0000	,	0xFF0000	,	0x000000	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x000000	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x548235	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0x548235	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0xBF8F00	,	0x000000	,	0xBF8F00	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	,	0x000000	},

  // ↑↑↑ LED_Scroll_Pattern.xlsm で変換したコードをここに貼る ↑↑↑
};


/// 関数プロトタイプ ///
void hex_led_animation01_callback(TimerHandle_t _xTimer);
void start_hex_led_animation_timer(TimerCallbackFunction_t pxCallbackFunction);


// 次のようにパターンを回転させるための変換テーブル
//
//       　 0  →  1  →  2  →  3
//        ↑ 　                  ↓ 
//       4     5  →  6  →  7 　  8
//      ↑     ↑             ↓     ↓ 
//     9    10   11  →  12   13   14
//    ↑    ↑    ↑         ↓    ↓    ↓ 
//  15   16   17    18     19   20   21 
//    ↑    ↑    ↑         ↓    ↓    ↓ 
//    22   23    24  ←  25   26   27
//      ↑     ↑             ↓    ↓ 
//      28     29  ← 30  ← 31   32
//        ↑ 　                 ↓ 
//         33  ←  34　← 35 ← 36
                    // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19  
int rotate_next[37] = {3, 8,14,21, 2, 7,13,20,27, 1, 6,12,19,26,32, 0, 5,11,18,25,
                    //20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 
                      31,36, 4,10,17,24,30,35, 9,16,23,29,34,15,22,28,33};


void hex_led_init(void){
  FastLED.addLeds<SK6812, LED_DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical
  FastLED.setBrightness( LED_BRIGHTNESS );
}

void hex_led_ptn_set(const CRGB* ptn){
  memcpy(leds, ptn, NUM_LEDS * sizeof(CRGB));
  FastLED.show();
}

void hex_led_ptn_rotate_set(CRGB* ptn, int step){
  for(int s=0; s<step; s++){
    for(int i=0; i<NUM_LEDS; i++){
    leds_rotate_tmp[rotate_next[i]] = ptn[i];
  }
  memcpy(ptn, leds_rotate_tmp, NUM_LEDS * sizeof(CRGB));

  }
  hex_led_ptn_set(ptn);
}

bool hex_led_animation_enable = false;
TimerHandle_t xTimerHexAnimation;
#define HEX_ANIMATION_PERIOD (200)     // 0.2s

void hex_led_animation01_callback(TimerHandle_t _xTimer){

  hex_led_ptn_rotate_set(led_rotate_pattern01, 1);

  if(hex_led_animation_enable){
    start_hex_led_animation_timer(hex_led_animation01_callback);
  }
  else{
    hex_led_ptn_off();
  }
}

void start_hex_led_animation_timer(TimerCallbackFunction_t pxCallbackFunction){
  xTimerHexAnimation = xTimerCreate("Timer HEX Animation", HEX_ANIMATION_PERIOD, pdFALSE, 0, pxCallbackFunction);
  
  if(xTimerHexAnimation != NULL){
    xTimerStart(xTimerHexAnimation, 0);
    //Serial.println("Start hex animation timer.");
  }
  else{
    Serial.println("Failed to start hex animation timer.");
  }

}

void hex_led_ptn_thinking_start(void){
  hex_led_animation_enable = true;
  start_hex_led_animation_timer(hex_led_animation01_callback);
}

void hex_led_ptn_thinking_end(void){
  hex_led_animation_enable = false;
}

void hex_led_scroll_ptn_set(const CRGB ptn[][SCROLL_COLUMNS]){
  FastLED.setBrightness( LED_BRIGHTNESS );
  
  for(int frame=0; frame<(SCROLL_COLUMNS-SCROLL_MARGIN+1); frame++){

    memcpy(&leds[0], &ptn[0][frame + 1], sizeof(CRGB)*4);
    memcpy(&leds[4], &ptn[1][frame + 1], sizeof(CRGB)*5);
    memcpy(&leds[9], &ptn[2][frame], sizeof(CRGB)*6);
    memcpy(&leds[15], &ptn[3][frame], sizeof(CRGB)*7);
    memcpy(&leds[22], &ptn[4][frame], sizeof(CRGB)*6);
    memcpy(&leds[28], &ptn[5][frame + 1], sizeof(CRGB)*5);
    memcpy(&leds[33], &ptn[6][frame + 1], sizeof(CRGB)*4);

    FastLED.show();
    delay(200);

  }

}


void hex_led_ptn_off(void){
  hex_led_ptn_set(led_pattern00);
}

void hex_led_ptn_wake(void){
  FastLED.setBrightness( LED_BRIGHTNESS );
  hex_led_ptn_set(led_pattern01);
}


void hex_led_ptn_accept(void){
  FastLED.setBrightness( LED_BRIGHTNESS );
  for(int i=0; i<5; i++){
    // Turn the LED on, then pause
    hex_led_ptn_set(led_pattern01);
    delay(200);
    // Now turn the LED off, then pause
    hex_led_ptn_set(led_pattern02);
    delay(200);
  }
  hex_led_ptn_set(led_pattern00);
}


#define OMEGA   (M_PI * 2 / 6000)      //点滅の周期6秒
void hex_led_ptn_notification(void){
  unsigned int tick_ms;

  tick_ms = xTaskGetTickCount() * portTICK_RATE_MS;

  FastLED.setBrightness( LED_BRIGHTNESS * (sin(OMEGA * tick_ms) + 1.0) / 2.0);
  hex_led_ptn_set(led_pattern03);

}


void hex_led_ptn_boot(void){

  hex_led_scroll_ptn_set(led_scroll_pattern01);

}

//2024.05.23
void hex_led_ptn_pow_off_1of5(void){
  hex_led_ptn_set(led_pattern04);
}
void hex_led_ptn_pow_off_2of5(void){
  hex_led_ptn_set(led_pattern05);
}
void hex_led_ptn_pow_off_3of5(void){
  hex_led_ptn_set(led_pattern06);
}
void hex_led_ptn_pow_off_4of5(void){
  hex_led_ptn_set(led_pattern07);
}
void hex_led_ptn_pow_off_5of5(void){
  hex_led_ptn_set(led_pattern08);
}
void hex_led_ptn_test(void){
  hex_led_ptn_set(led_pattern99);
}

//2024.06.13
void hex_led_ptn_Np(int patternNum){
  Serial.print("hex_led_ptn_Np patternNum = ");
  Serial.println(patternNum);
  FastLED.setBrightness( LED_BRIGHTNESS );
  switch(patternNum)
  {
    case 1: hex_led_ptn_set(led_patternN1); break;
    case 2: hex_led_ptn_set(led_patternN2); break;
    case 3: hex_led_ptn_set(led_patternN3); break;
    case 4: hex_led_ptn_set(led_patternN4); break;
    case 5: hex_led_ptn_set(led_patternN5); break;
    case 6: hex_led_ptn_set(led_patternN6); break;
    case 7: hex_led_ptn_set(led_patternN7); break;
    case 8: hex_led_ptn_set(led_patternN8); break;
    case 9: hex_led_ptn_set(led_patternN9); break;
    case 0: hex_led_ptn_set(led_patternN0); break;
    default:hex_led_ptn_off(); break;
  }
}

