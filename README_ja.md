# M5Stack-Stackchan with  function calling

[![Powered By PlatformIO](https://img.shields.io/badge/powered-PlatformIO-brightgreen)](https://platformio.org/)

![alt text](/images/IMG2-1.jpg)

[ENGLISH](README.md)

## 機能

* OpenAIのfunction callingを使いHexLEDと天気予報を利用できます。
  function callingのコーディングは手習い的に単純にしています。
* 元ソースはronron-ghさんの下記作品です。但し顔認識は無効です。
  [AI_StackChan2_DevCam](https://github.com/ronron-gh/AI_StackChan2_DevCam)
* 無言のときの吹き出しに時刻を表示します。
* HexLED制御はronron-ghさんの下記作品のコードです。
  [AI_StackChan2_FuncCall](https://github.com/ronron-gh/AI_StackChan2_FuncCall)
* 独り言はソースコードで有効に固定しています。
* 無操作が続くと独り言が減り、最終的にはシャットダウンします。
* HexLEDの表示パターンは下記ツールで作成しています。
  [HexLED-Editor-for-ronronsan-AI_StackChan2_FuncCall](https://github.com/QtDogBow/HexLED-Editor-for-ronronsan-AI_StackChan2_FuncCall)
* HexLEDの固定にはTakao-Baseのshellを改造しています。
 ![alt text](/images/IMG4.jpg)
* M5Stack CoreS3で動作確認。

## インストール

### 前提

* VSCode 1.90.1 , Platform IOで動作確認しています。

### 準備

* SDのwifi.txtに下記情報を記述してください。
  
```sh
WIFI_SSID
WIFI_PASS
```

* SDのapikey.txtに下記情報を記述してください。天気情報はOpenWeatherのフリーアカウントにより取得しています。
  [OpenWeather](https://openweathermap.org/)

```sh
OPENAI_API_KEY
VOICEVOX_APIKEY
STT_APIKEY
OPENWEATHER_APIKEY
```

## 使い方

```sh

HexLEDを点灯させるには、額の辺りを押してから「LEDパネルをパターン７で点灯させて」などのように言います。
パターンは０から９まで登録してあります。

天気を聞くには、額の辺りを押してから「東京の天気は？」のように言います。
OpenWeatherのフリーアカウントでは主要都市の現在の天気ぐらいしか答えてくれません。
```

## 独り言を止めるには

main.cppのloop()関数の先頭から5行目辺りにある下記行をコメントアウトしてください。

```cpp

random_time = MILLIS_MONOLOGUE_SHORT_INTERVAL;//独り言モードになる。
```

## 注意

* シャットダウン後の復帰には電源ボタンの長押しが要るようです。
* 滑りやすい机の上で独り言を言い続けると、スタックチャンが落下する事
  があります。
* 独り言を喋っているときにHexLED点灯を提案または実行する事がありま
  す。これは想定外でした。
