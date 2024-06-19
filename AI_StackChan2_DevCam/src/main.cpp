#include <Arduino.h>
//#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <M5Unified.h>
#include <nvs.h>
#include <Avatar.h>
#include <AudioOutput.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include "AudioFileSourceHTTPSStream.h"
#include "AudioOutputM5Speaker.h"
#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing       
#include "WebVoiceVoxTTS.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include "rootCAgoogle.h"
#include <ArduinoJson.h>
#include <ESP32WebServer.h>
#include <ESPmDNS.h>
#include <deque>
#include "AudioWhisper.h"
#include "Whisper.h"
#include "Audio.h"
#include "CloudSpeechClient.h"

#include "time.h" //2024.05.09
#include "rtc.h"  //2024.05.09
#if defined( ENABLE_HEX_LED ) //2024.05.15 追加
#include "HexLED.h"
#endif
#include <M5GFX.h>

//2024.05.20 Button unit [U027]のテスト
//#define PB_IN0  8

#if defined( ENABLE_FACE_DETECT )
#include <esp_camera.h>
#include <fb_gfx.h>
#include <vector>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#define TWO_STAGE 1 /*<! 1: detect by two-stage which is more accurate but slower(with keypoints). */
                    /*<! 0: detect by one-stage which is less accurate but faster(without keypoints). */

#define FACE_COLOR_WHITE  0x00FFFFFF
#define FACE_COLOR_BLACK  0x00000000
#define FACE_COLOR_RED    0x000000FF
#define FACE_COLOR_GREEN  0x0000FF00
#define FACE_COLOR_BLUE   0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN   (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

static camera_config_t camera_config = {
    .pin_pwdn     = -1,
    .pin_reset    = -1,
    .pin_xclk     = 2,
    .pin_sscb_sda = 12,
    .pin_sscb_scl = 11,

    .pin_d7 = 47,
    .pin_d6 = 48,
    .pin_d5 = 16,
    .pin_d4 = 15,
    .pin_d3 = 42,
    .pin_d2 = 41,
    .pin_d1 = 40,
    .pin_d0 = 39,

    .pin_vsync = 46,
    .pin_href  = 38,
    .pin_pclk  = 45,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,
    //.pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_QVGA,   // QVGA(320x240)
    .jpeg_quality = 0,
    //.fb_count     = 2,
    .fb_count     = 1,
    .fb_location  = CAMERA_FB_IN_PSRAM,
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

bool isSubWindowON = true;
//bool isSilentMode = false;

#endif    //ENABLE_FACE_DETECT
bool isSilentMode = false;



// 保存する質問と回答の最大数
const int MAX_HISTORY = 5;

// 過去の質問と回答を保存するデータ構造
std::deque<String> chatHistory;

#define USE_SDCARD
#define WIFI_SSID "SET YOUR WIFI SSID"
#define WIFI_PASS "SET YOUR WIFI PASS"
#define OPENAI_APIKEY "SET YOUR OPENAI APIKEY"
#define VOICEVOX_APIKEY "SET YOUR VOICEVOX APIKEY"
#define STT_APIKEY "SET YOUR STT APIKEY"
#define OPENWEATHER_APIKEY "SET YOUR OPENWEATHER  APIKEY"

#define USE_SERVO
#ifdef USE_SERVO
#if defined(ARDUINO_M5STACK_Core2)
  // #define SERVO_PIN_X 13  //Core2 PORT C
  // #define SERVO_PIN_Y 14
  #define SERVO_PIN_X 33  //Core2 PORT A
  #define SERVO_PIN_Y 32
#elif defined( ARDUINO_M5STACK_FIRE )
  #define SERVO_PIN_X 21
  #define SERVO_PIN_Y 22
#elif defined( ARDUINO_M5Stack_Core_ESP32 )
  #define SERVO_PIN_X 21
  #define SERVO_PIN_Y 22
#elif defined( ARDUINO_M5STACK_CORES3 )
  #define SERVO_PIN_X 18  //CoreS3 PORT C
  #define SERVO_PIN_Y 17
#endif
#endif

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;

using namespace m5avatar;
Avatar avatar;

//2024.05.06  face color change
ColorPalette* cps;

const Expression expressions_table[] = {
  Expression::Neutral,
  Expression::Happy,
  Expression::Sleepy,
  Expression::Doubt,
  Expression::Sad,
  Expression::Angry
};

ESP32WebServer server(80);

//---------------------------------------------
String OPENAI_API_KEY = "";
String VOICEVOX_API_KEY = "";
String STT_API_KEY = "";
String OPENWEATHER_API_KEY = "";
String TTS_SPEAKER_NO = "3";
String TTS_SPEAKER = "&speaker=";
String TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;

//---------------------------------------------
// C++11 multiline string constants are neato...
static const char HEAD[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>AIｽﾀｯｸﾁｬﾝ</title>
</head>)KEWL";

static const char APIKEY_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>APIキー設定</title>
  </head>
  <body>
    <h1>APIキー設定</h1>
    <form>
      <label for="role1">OpenAI API Key</label>
      <input type="text" id="openai" name="openai" oninput="adjustSize(this)"><br>
      <label for="role2">VoiceVox API Key</label>
      <input type="text" id="voicevox" name="voicevox" oninput="adjustSize(this)"><br>
      <label for="role3">Speech to Text API Key</label>
      <input type="text" id="sttapikey" name="sttapikey" oninput="adjustSize(this)"><br>
      <label for="role4">OpenWeahter API Key</label>
      <input type="text" id="openweather" name="openweather" oninput="adjustSize(this)"><br>
      <button type="button" onclick="sendData()">送信する</button>
    </form>
    <script>
      function adjustSize(input) {
        input.style.width = ((input.value.length + 1) * 8) + 'px';
      }
      function sendData() {
        // FormDataオブジェクトを作成
        const formData = new FormData();

        // 各ロールの値をFormDataオブジェクトに追加
        const openaiValue = document.getElementById("openai").value;
        if (openaiValue !== "") formData.append("openai", openaiValue);

        const voicevoxValue = document.getElementById("voicevox").value;
        if (voicevoxValue !== "") formData.append("voicevox", voicevoxValue);

        const sttapikeyValue = document.getElementById("sttapikey").value;
        if (sttapikeyValue !== "") formData.append("sttapikey", sttapikeyValue);

        const openweatherValue = document.getElementById("openweather").value;
        if (openweatherValue !== "") formData.append("openweather", openweatherValue);

	    // POSTリクエストを送信
	    const xhr = new XMLHttpRequest();
	    xhr.open("POST", "/apikey_set");
	    xhr.onload = function() {
	      if (xhr.status === 200) {
	        alert("データを送信しました！");
	      } else {
	        alert("送信に失敗しました。");
	      }
	    };
	    xhr.send(formData);
	  }
	</script>
  </body>
</html>)KEWL";

static const char ROLE_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
<head>
	<title>ロール設定</title>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<style>
		textarea {
			width: 80%;
			height: 200px;
			resize: both;
		}
	</style>
</head>
<body>
	<h1>ロール設定</h1>
	<form onsubmit="postData(event)">
		<label for="textarea">ここにロールを記述してください。:</label><br>
		<textarea id="textarea" name="textarea"></textarea><br><br>
		<input type="submit" value="Submit">
	</form>
	<script>
		function postData(event) {
			event.preventDefault();
			const textAreaContent = document.getElementById("textarea").value.trim();
//			if (textAreaContent.length > 0) {
				const xhr = new XMLHttpRequest();
				xhr.open("POST", "/role_set", true);
				xhr.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
			// xhr.onload = () => {
			// 	location.reload(); // 送信後にページをリロード
			// };
			xhr.onload = () => {
				document.open();
				document.write(xhr.responseText);
				document.close();
			};
				xhr.send(textAreaContent);
//        document.getElementById("textarea").value = "";
				alert("Data sent successfully!");
//			} else {
//				alert("Please enter some text before submitting.");
//			}
		}
	</script>
</body>
</html>)KEWL";

String speech_text = "";
String speech_text_buffer = "";
DynamicJsonDocument chat_doc(1024*10);
//String json_ChatString = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";
//String json_ChatString = payload1;
String Role_JSON = "";

//2024.05.09
m5::RTC8563_Class rtc;
tm timeInfo;

#if defined( ENABLE_HEX_LED ) //2024.05.15 追加
#pragma region //LEDパネル
//2024.06.14
int task_ledPanel_param1 = 0;//LED点灯パターン
void task_ledPanel(void *args)
{
  delay(3000);//音声応答終了終了時にHexLEDが光る（回転）処理がある。この終了を待つ。
  hex_led_ptn_Np(task_ledPanel_param1);//LEDパネル(HexLED)をパターン番号（引数１）に従った点灯パターンで光らせる。
  vTaskDelete(NULL);//このタスク（task_ledPanel）を終了する。
}

void LedPanel(int pattern)
{
  Serial.print("LedPanel Pattern -> pattern = ");
  Serial.println(pattern);
  if(pattern >= 0 and pattern <10)
  {
    //喋らせる。喋っている途中でLEDも点灯する。
    char buff[100];
    sprintf(buff,"LEDパネルをパターン%dで点灯しました。",pattern);
    Voicevox_tts(buff, (char*)TTS_PARMS.c_str());
    //task_ledPanelをタスクとして起動する。
    task_ledPanel_param1 = pattern;//タスクに依頼する点灯パターン
    avatar.addTask(task_ledPanel,"task_ledPanel");
   }
  else return;
}
#pragma endregion

#endif

#pragma region //天気予報
String task_city_name = "東京";  //Open Weatherでは漢字で都市名を指定する。
void task_weathers(void *args)
{
  Serial.print("task_weathers city_name = ");
  Serial.println(task_city_name);

  //delay(3000);//音声応答終了終了時にHexLEDが光る（回転）処理がある。この終了を待つ。
  String apiKey = OPENWEATHER_API_KEY;//"OpenWeatherMapのAPIキー";
  // 天気情報取得
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + task_city_name + ",jp&appid=" + apiKey+ "&units=metric&lang=ja";
  HTTPClient http;
  http.begin(url);
#if true

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      // JSONパース
      StaticJsonDocument<256> doc;
      deserializeJson(doc, payload);
      
      //Serial.print("openweather > payload = ");
      //Serial.println(payload);

      // ここから天気情報を表示する処理を追加
      String weather_description = doc["weather"][0]["description"];
      
      //Serial.print("openweather > weather_description = ");
      //Serial.println(weather_description);
      
      String sTemperature = payload.substring(payload.indexOf("\"temp\":"));
      sTemperature = sTemperature.substring(strlen("\"temp\":"),sTemperature.indexOf(","));
      float fTemperature = atoi(sTemperature.c_str());

      //喋らせる。
      char buff[200];
      sprintf(buff,"%sの現在の天気 %s、気温は%4.1fです。", task_city_name, weather_description, fTemperature);
      Voicevox_tts(buff, (char*)TTS_PARMS.c_str());

  }
#endif
  http.end();
  vTaskDelete(NULL);//このタスク（task_weathers）を終了する。
}

void get_weathers(String city_name)
{
  Serial.print("get_weathers city_name = ");
  Serial.println(city_name);
  
  if((city_name != NULL) && (city_name != "")) task_city_name = city_name;//タスクに依頼する点灯パターン
  //task_weathersをタスクとして起動する。
  avatar.addTask(task_weathers,"task_weathers");
}
#pragma endregion

bool init_chat_doc(const char *data)
{
  DeserializationError error = deserializeJson(chat_doc, data);
  if (error) {
    Serial.println("DeserializationError");
    return false;
  }
  String json_str; //= JSON.stringify(chat_doc);
  serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
//  Serial.println(json_str);
    return true;
}

void handleRoot() {
  server.send(200, "text/plain", "hello from m5stack!");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
//  server.send(404, "text/plain", message);
  server.send(404, "text/html", String(HEAD) + String("<body>") + message + String("</body>"));
}

void handle_speech() {
  String message = server.arg("say");
  String speaker = server.arg("voice");
  if(speaker != "") {
    TTS_PARMS = TTS_SPEAKER + speaker;
  }
  Serial.println(message);
  ////////////////////////////////////////
  // 音声の発声
  ////////////////////////////////////////
  avatar.setExpression(Expression::Happy);
  Voicevox_tts((char*)message.c_str(), (char*)TTS_PARMS.c_str());
  server.send(200, "text/plain", String("OK"));
}
//---------------------------------------- begin
String prompt = "You are helpful assistant.";
String inputText = "明日の天気を教えて下さい。";  //fanction callingで関数を選択するように質問を変更する。
String model = "gpt-3.5-turbo";
int maxTokens = 100;
float temperature = 1;
float top_p = 1;

//単体の単純プログラムで成功したfunction calling用のコードを編集していく。
String payload1 = "{\"messages\": [{\"role\": \"system\", \"content\": \"" + prompt  + "\"},"
  + "{\"role\": \"user\", \"content\": \"" + inputText  + "\"}], " 
  + "\"max_tokens\":" + String(maxTokens) + ", \"temperature\":" + float(temperature) + ", \"top_p\":" + float(top_p) + ", \"model\": \"" + model // + "\"}"//;
  + "\", " 
  + "\"functions\": ["
#if defined( ENABLE_HEX_LED ) //2024.05.15 追加
        "{"
            "\"name\": \"LedPanel\","
            "\"description\": \"指定したパターン番号に従ってLEDパネルを点灯しました\","
            "\"parameters\": {"
              "\"type\":\"object\","
              "\"properties\": {"
                  "\"pattern\":{"
                    "\"type\": \"number\","
                    "\"description\": \"パターン番号\""
                  "}"
              "},"
              "\"required\": [\"pattern\"]"
            "}"
        "},"
#endif
        "{"
            "\"name\": \"get_weathers\","
            "\"description\": \"指定された都市の天気予報を取得します。\","
            "\"parameters\": {"
              "\"type\":\"object\","
              "\"properties\": {"
                  "\"city_name\":{"
                    "\"type\": \"string\","
                    "\"description\": \"都市名\""
                  "}"
              "},"
              "\"required\": []" //<== デフォルトを自分の都市名にしておく。
            "}"
        "}"
    "],"
    "\"function_call\":\"auto\""
"}";
String json_ChatString = payload1;

String https_post_json(const char* url, const char* json_string, const char* root_ca) {
  {//2024.06.07
    Serial.println("-----------------------------");
    Serial.println("https_post_json(..json_string..)");
    Serial.print("json_string = ");
    Serial.println(json_string);
    Serial.println("-----------------------------");
  }

  String payload = "";
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(root_ca);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
      https.setTimeout( 65000 ); 
  
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, url)) {  // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
        int httpCode = https.POST((uint8_t *)json_string, strlen(json_string));
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            payload = https.getString();
            Serial.println("//////////////");
            Serial.print("payload = ");
            Serial.println(payload);
            Serial.println("//////////////");
          }
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      // End extra scoping block
    }  
    delete client;
  } else {
    Serial.println("Unable to create client");
  }
  return payload;
}

String chatGpt(String json_string) {
  String response = "";;
  avatar.setExpression(Expression::Doubt);
  avatar.setSpeechText("考え中…");
  {
    #pragma region //json_ChatStringにユーザーの言葉を入れたjson_stringを作成する
    DynamicJsonDocument doc(2000);
    DeserializationError error = deserializeJson(doc, json_string);
    String text = doc["messages"][1]["content"];
    doc.clear();
    error = deserializeJson(doc, json_ChatString);
    doc["messages"][1]["content"] = text;
    json_string ="";
    serializeJson(doc, json_string);

    Serial.println("-----------------------------");
    Serial.print("chatGpt (in) json_string = ");
    Serial.println(json_string);
    Serial.println("-----------------------------");
    #pragma endregion
  }
  
  String ret = https_post_json("https://api.openai.com/v1/chat/completions", json_string.c_str(), root_ca_openai);

  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");

#pragma region //chatGTPの出力をデバッグ表示する  
  Serial.println("-----------------------------");
  Serial.print("chatGpt (out) https_post_json() = ");
  Serial.println(ret);
  Serial.println("-----------------------------");
#pragma endregion

  if(ret != ""){
    DynamicJsonDocument doc(2000);
    DeserializationError error = deserializeJson(doc, ret.c_str());
    if (error) {
      #pragma region //エラー処理
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      avatar.setExpression(Expression::Sad);
      avatar.setSpeechText("エラーです");
      response = "エラーです";
      delay(1000);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
      #pragma endregion //エラー処理
    }else{
      const char* data = doc["choices"][0]["message"]["content"];//chatGPTからの回答文言
      if(data == NULL)//function callingの場合は"content"には何もない(null)
      {
        #pragma region //ユーザーの言葉からfunction Calling機能によりローカル・プログラムを起動するときの処理
        String func_name = doc["choices"][0]["message"]["function_call"]["name"];
        Serial.print("function name = ");
        Serial.println(func_name);
        const char* params;
        params = doc["choices"][0]["message"]["function_call"]["arguments"];
        Serial.print("params = ");
        Serial.println(params);
        if(func_name == "LedPanel") {
#if defined( ENABLE_HEX_LED ) //2024.05.15 追加
          String Params = params;     //{"pattern":3}
          Params = Params.substring(Params.indexOf(":")+1);
          Params.replace("/[^0-9]/","");
          Serial.print("chatGpt function name = LedPanel, Pattern = ");
          Serial.println(Params);// 3
          int iParam1 = atoi(Params.c_str());
          Serial.print("chatGpt function Pattern -> iParam1 = ");
          Serial.println(iParam1);;// 数値3
          if(iParam1 >= 0 && iParam1 < 10) LedPanel(iParam1);//HexLEDを点灯させる。hex_led_ptn_Np
#endif
        }
        else if(func_name == "get_weathers") {
          String Params = params;     //{"city_name":"Tokyo"}
          Params = Params.substring(Params.indexOf(":")+1);
          Params.replace("\"","");
          Params.replace("}","");
          Serial.print("function name = ");
          Serial.println(func_name);
          Serial.print("chatGpt function city_name = ");
          Serial.println(Params);// 東京
          get_weathers(Params);// 天気予報を取得して喋らせる。
        }
        response = "";//"少々お待ちください。";//function callingの場合
        #pragma endregion
      }
      else {
        #pragma region //ユーザーの言葉に対してchatGTPが言葉のみを返すときの処理
        Serial.println(data);
        response = String(data);
        std::replace(response.begin(),response.end(),'\n',' ');
        #pragma endregion
      }
    }
  } else {
    #pragma region //エラー処理
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("わかりません");
    response = "わかりません";
    delay(1000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
    #pragma endregion //エラー処理
  }
  return response;
}

String InitBuffer = "";

void handle_chat() {
  static String response = "";
  // tts_parms_no = 1;
  String text = server.arg("text");
    //デバッグ出力
    Serial.println("====================");
    Serial.print("handle_chat  text = ");
    Serial.println(text);
    Serial.println("====================");


  String speaker = server.arg("voice");
  if(speaker != "") {
    TTS_PARMS = TTS_SPEAKER + speaker;
  }
  Serial.println(InitBuffer);
  init_chat_doc(InitBuffer.c_str());
  // 質問をチャット履歴に追加
  chatHistory.push_back(text);
   // チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (chatHistory.size() > MAX_HISTORY * 2)
  {
    chatHistory.pop_front();
    chatHistory.pop_front();
  }

  for (int i = 0; i < chatHistory.size(); i++)
  {
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    if(i % 2 == 0) {
      systemMessage1["role"] = "user";
    } else {
      systemMessage1["role"] = "assistant";
    }
    systemMessage1["content"] = chatHistory[i];
  }

  String json_string;
  serializeJson(chat_doc, json_string);
    //デバッグ出力
    Serial.println("====================");
    Serial.println("handle_chat");
    Serial.println("serializeJson(chat_doc, json_string);");
    Serial.println(json_string);
    Serial.println("====================");

  if(speech_text=="" && speech_text_buffer == "") {
    Serial.println("handle_chat --> chatGpt(json_string)");
    response = chatGpt(json_string);
    speech_text = response;
    // 返答をチャット履歴に追加
    chatHistory.push_back(response);
  } else {
    response = "busy";
  }
  // Serial.printf("chatHistory.max_size %d \n",chatHistory.max_size());
  // Serial.printf("chatHistory.size %d \n",chatHistory.size());
  // for (int i = 0; i < chatHistory.size(); i++)
  // {
  //   Serial.print(i);
  //   Serial.println("= "+chatHistory[i]);
  // }
  serializeJsonPretty(chat_doc, json_string);

    //デバッグ出力
    Serial.println("====================");
    Serial.println("serializeJsonPretty(chat_doc, json_string);");
    Serial.println(json_string);
    Serial.println("====================");

  server.send(200, "text/html", String(HEAD)+String("<body>")+response+String("</body>"));
}

void exec_chatGPT(String text) {
  static String response = "";
  Serial.println(InitBuffer);
  init_chat_doc(InitBuffer.c_str());

  chat_doc["messages"][1]["content"] = text;//2024.06.08 debug

  // 質問をチャット履歴に追加
  chatHistory.push_back(text);
   // チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (chatHistory.size() > MAX_HISTORY * 2)
  {
    chatHistory.pop_front();
    chatHistory.pop_front();
  }

  for (int i = 0; i < chatHistory.size(); i++)
  {
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    if(i % 2 == 0) {
      systemMessage1["role"] = "user";
    } else {
      systemMessage1["role"] = "assistant";
    }
    systemMessage1["content"] = chatHistory[i];
  }
    //2024.06.08 debug
    chat_doc["messages"][1]["content"] = text;
    Serial.println("====================");
    Serial.print("exec_chatGPT  chat_doc[\"messages\"][1][\"content\"] = ");
    String s = chat_doc["messages"][1]["content"];
    Serial.println(s.c_str());
    Serial.println("====================");

  String json_string;
  serializeJson(chat_doc, json_string);

    //2024.06.08 debug
    Serial.println("====================");
    Serial.println("exec_chatGPT  serializeJson(chat_doc, json_string)");
    Serial.print("exec_chatGPT  json_string = ");
    Serial.println(json_string);
    Serial.println("====================");

  if(speech_text=="" && speech_text_buffer == "") {
    Serial.println("exec_chatGpt --> chatGpt(json_string)");
    response = chatGpt(json_string);
    speech_text = response;
    // 返答をチャット履歴に追加
    chatHistory.push_back(response);
  } else {
    response = "busy";
  }
  // Serial.printf("chatHistory.max_size %d \n",chatHistory.max_size());
  // Serial.printf("chatHistory.size %d \n",chatHistory.size());
  // for (int i = 0; i < chatHistory.size(); i++)
  // {
  //   Serial.print(i);
  //   Serial.println("= "+chatHistory[i]);
  // }
  
  //2024.06.08 debug
  serializeJsonPretty(chat_doc, json_string);
  Serial.println("====================");
  Serial.println("serializeJsonPretty(chat_doc, json_string);");
  Serial.println(json_string);
  Serial.println("====================");

}
/*
String Role_JSON = "";
void exec_chatGPT1(String text) {
  static String response = "";
  init_chat_doc(Role_JSON.c_str());

  String role = chat_doc["messages"][0]["role"];
  if(role == "user") {chat_doc["messages"][0]["content"] = text;}
  String json_string;
  serializeJson(chat_doc, json_string);

  response = chatGpt(json_string);
  speech_text = response;
//  server.send(200, "text/html", String(HEAD)+String("<body>")+response+String("</body>"));
}
*/
void handle_apikey() {
  // ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", APIKEY_HTML);
}

void handle_apikey_set() {
  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  // openai
  String openai = server.arg("openai");
  // voicetxt
  String voicevox = server.arg("voicevox");
  // voicetxt
  String sttapikey = server.arg("sttapikey");
  // openWeather
  String openweather = server.arg("openweather");

  OPENAI_API_KEY = openai;
  VOICEVOX_API_KEY = voicevox;
  STT_API_KEY = sttapikey;
  OPENWEATHER_API_KEY = openweather;
  Serial.println(openai);
  Serial.println(voicevox);
  Serial.println(sttapikey);
  Serial.println(openweather);

  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
    nvs_set_str(nvs_handle, "openai", openai.c_str());
    nvs_set_str(nvs_handle, "voicevox", voicevox.c_str());
    nvs_set_str(nvs_handle, "sttapikey", sttapikey.c_str());
    nvs_set_str(nvs_handle, "openweather", openweather.c_str());
    nvs_close(nvs_handle);
  }
  server.send(200, "text/plain", String("OK"));
}

void handle_role() {
  // ファイルを読み込み、クライアントに送信する
  server.send(200, "text/html", ROLE_HTML);
}

bool save_json(){
  // SPIFFSをマウントする
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }

  // JSONファイルを作成または開く
  File file = SPIFFS.open("/data.json", "w");
  if(!file){
    Serial.println("Failed to open file for writing");
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJson(chat_doc, file);
  file.close();
  return true;
}


/**
 * アプリからテキスト(文字列)と共にRoll情報が配列でPOSTされてくることを想定してJSONを扱いやすい形に変更
 * 出力形式をJSONに変更
*/
void handle_role_set() {
  // POST以外は拒否
  if (server.method() != HTTP_POST) {
    return;
  }
  String role = server.arg("plain");
  if (role != "") {
//    init_chat_doc(InitBuffer.c_str());
    if(!init_chat_doc(json_ChatString.c_str()))
    {
      //エラー表示
      Serial.println("handle_role_set  Line=" + String(__LINE__) + "  init_chat_doc error");
      Serial.println("json_ChatString = ");
      Serial.println(json_ChatString);
    }
    JsonArray messages = chat_doc["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    systemMessage1["role"] = "system";
    systemMessage1["content"] = role;
//    serializeJson(chat_doc, InitBuffer);
  } else {
    if(!init_chat_doc(json_ChatString.c_str()))
    {
      //エラー表示
      Serial.println("handle_role_set  Line=" + String(__LINE__) + "  init_chat_doc error");
      Serial.println("json_ChatString = ");
      Serial.println(json_ChatString);
    }
  }
  //会話履歴をクリア
  chatHistory.clear();

  InitBuffer="";
  serializeJson(chat_doc, InitBuffer);
  Serial.println("InitBuffer = " + InitBuffer);
  Role_JSON = InitBuffer;

  // JSONデータをspiffsへ出力する
  save_json();

  // 整形したJSONデータを出力するHTMLデータを作成する
  String html = "<html><body><pre>";
  serializeJsonPretty(chat_doc, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", html);
//  server.send(200, "text/plain", String("OK"));
};

// 整形したJSONデータを出力するHTMLデータを作成する
void handle_role_get() {

  String html = "<html><body><pre>";
  serializeJsonPretty(chat_doc, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", String(HEAD) + html);
};

void handle_face() {
  String expression = server.arg("expression");
  expression = expression + "\n";
  Serial.println(expression);
  switch (expression.toInt())
  {
    case 0: avatar.setExpression(Expression::Neutral); break;
    case 1: avatar.setExpression(Expression::Happy); break;
    case 2: avatar.setExpression(Expression::Sleepy); break;
    case 3: avatar.setExpression(Expression::Doubt); break;
    case 4: avatar.setExpression(Expression::Sad); break;
    case 5: avatar.setExpression(Expression::Angry); break;  
  } 
  server.send(200, "text/plain", String("OK"));
}

void handle_setting() {
  String value = server.arg("volume");
  String led = server.arg("led");
  String speaker = server.arg("speaker");
//  volume = volume + "\n";
  Serial.println(speaker);
  Serial.println(value);
  size_t speaker_no;
  if(speaker != ""){
    speaker_no = speaker.toInt();
    if(speaker_no > 60) {
      speaker_no = 60;
    }
    TTS_SPEAKER_NO = String(speaker_no);
    TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
  }

  if(value == "") value = "180";
  size_t volume = value.toInt();
  uint8_t led_onoff = 0;
  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("setting", NVS_READWRITE, &nvs_handle)) {
    if(volume > 255) volume = 255;
    nvs_set_u32(nvs_handle, "volume", volume);
    if(led != "") {
      if(led == "on") led_onoff = 1;
      else  led_onoff = 0;
      nvs_set_u8(nvs_handle, "led", led_onoff);
    }
    nvs_set_u8(nvs_handle, "speaker", speaker_no);

    nvs_close(nvs_handle);
  }
  M5.Speaker.setVolume(volume);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
  server.send(200, "text/plain", String("OK"));
}

AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;
AudioFileSourceBuffer *buff = nullptr;
int preallocateBufferSize = 30*1024;
uint8_t *preallocateBuffer;
AudioFileSourceHTTPSStream *file = nullptr;

void playMP3(AudioFileSourceBuffer *buff){
  mp3->begin(buff, &out);
}

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

#ifdef USE_SERVO
#define START_DEGREE_VALUE_X 90
//#define START_DEGREE_VALUE_Y 90
//#define START_DEGREE_VALUE_Y 85 
#define START_DEGREE_VALUE_Y 80 
ServoEasing servo_x;
ServoEasing servo_y;
#endif

void lipSync(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    level = abs(*out.getBuffer());
    if(level<100) level = 0;
    if(level > 15000)
    {
      level = 15000;
    }
    float open = (float)level/15000.0;
    avatar->setMouthOpenRatio(open);
    avatar->getGaze(&gazeY, &gazeX);
    avatar->setRotation(gazeX * 5);
    delay(50);
  }
}

bool servo_home = false;
const int CENTER_X = 160;
const int CENTER_Y = 120;
int g_face_centroid_x = CENTER_X;
int g_face_centroid_y = CENTER_Y;
int g_angle_x = 0;
int g_angle_y = 0;

void servo(void *args)
{
  float gazeX, gazeY;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
#ifdef USE_SERVO
    if(!servo_home)
    {
      if(!isSilentMode){
        avatar->getGaze(&gazeY, &gazeX);
        servo_x.setEaseTo(START_DEGREE_VALUE_X + (int)(15.0 * gazeX));
        if(gazeY < 0) {
          int tmp = (int)(10.0 * gazeY);
          if(tmp > 10) tmp = 10;
          servo_y.setEaseTo(START_DEGREE_VALUE_Y + tmp);
        } else {
          servo_y.setEaseTo(START_DEGREE_VALUE_Y + (int)(10.0 * gazeY));
        }
      }
      else{
        int diff_x = g_face_centroid_x - CENTER_X;
        int diff_y = g_face_centroid_y - CENTER_Y;
        g_angle_x += 0.05 * diff_x;
        g_angle_y += 0.05 * diff_y;
        
        if(g_angle_x > 30) g_angle_x = 30;
        else if(g_angle_x < -30) g_angle_x = -30;

        if(g_angle_y > 5) g_angle_y = 5;
        else if(g_angle_y < -15) g_angle_y = -15;

        servo_x.setEaseTo(START_DEGREE_VALUE_X + g_angle_x);
        servo_y.setEaseTo(START_DEGREE_VALUE_Y + g_angle_y);
      }
    } else {
//     avatar->setRotation(gazeX * 5);
//     float b = avatar->getBreath();
       servo_x.setEaseTo(START_DEGREE_VALUE_X); 
//     servo_y.setEaseTo(START_DEGREE_VALUE_Y + b * 5);
       servo_y.setEaseTo(START_DEGREE_VALUE_Y);
    }
    synchronizeAllServosStartAndWaitForAllServosToStop();
#endif
    delay(50);
  }
}

//2024.04.23
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.nict.jp", 32400, 60000);
const char* ntpServer = "ntp.nict.jp";
//const char* ntpServer = "210.173.160.27";
const long  gmtOffset_sec = 3600 * 9;
const int   daylightOffset_sec = 0;


void Servo_setup() {
#ifdef USE_SERVO
  if (servo_x.attach(SERVO_PIN_X, START_DEGREE_VALUE_X, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo x");
  }
  if (servo_y.attach(SERVO_PIN_Y, START_DEGREE_VALUE_Y, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo y");
  }
  servo_x.setEasingType(EASE_QUADRATIC_IN_OUT);
  servo_y.setEasingType(EASE_QUADRATIC_IN_OUT);
  setSpeedForAllServos(30);

  servo_x.setEaseTo(START_DEGREE_VALUE_X); 
  servo_y.setEaseTo(START_DEGREE_VALUE_Y);
  synchronizeAllServosStartAndWaitForAllServosToStop();
#endif
}

struct box_t
{
  int x;
  int y;
  int w;
  int h;
  int touch_id = -1;

  void setupBox(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
  bool contain(int x, int y)
  {
    return this->x <= x && x < (this->x + this->w)
        && this->y <= y && y < (this->y + this->h);
  }
};
static box_t box_servo;
static box_t box_stt;
static box_t box_BtnA;
static box_t box_BtnC;
#if defined(ENABLE_FACE_DETECT)
static box_t box_subWindow;
#endif

void Wifi_setup() {
  // 前回接続時情報で接続する
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    Serial.print(".");
    delay(500);
    // 10秒以上接続できなかったら抜ける
    if ( 10000 < millis() ) {
      break;
    }
  }
  M5.Display.println("");
  Serial.println("");
  // 未接続の場合にはSmartConfig待受
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    M5.Display.println("Waiting for SmartConfig");
    Serial.println("Waiting for SmartConfig");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      M5.Display.print("#");
      Serial.print("#");
      // 30秒以上接続できなかったら抜ける
      if ( 30000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    // Wi-fi接続
    M5.Display.println("");
    Serial.println("");
    M5.Display.println("Waiting for WiFi");
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Display.print(".");
      Serial.print(".");
      // 60秒以上接続できなかったら抜ける
      if ( 60000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
  }
}

String SpeechToText(bool isGoogle){
  Serial.println("\r\nRecord start!\r\n");

  String ret = "";
  if( isGoogle) {
    Audio* audio = new Audio();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    avatar.setSpeechText("わかりました");  
    CloudSpeechClient* cloudSpeechClient = new CloudSpeechClient(root_ca_google, STT_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  } else {
    AudioWhisper* audio = new AudioWhisper();
    audio->Record();  
    Serial.println("Record end\r\n");
    Serial.println("音声認識開始");
    avatar.setSpeechText("わかりました");  
    Whisper* cloudSpeechClient = new Whisper(root_ca_openai, OPENAI_API_KEY.c_str());
    ret = cloudSpeechClient->Transcribe(audio);
    delete cloudSpeechClient;
    delete audio;
  }
  return ret;
}


#if defined(ENABLE_FACE_DETECT)
esp_err_t camera_init(){

    //initialize the camera
    M5.In_I2C.release();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        //Serial.println("Camera Init Failed");
        M5.Display.println("Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

static void draw_face_boxes(fb_data_t *fb, std::list<dl::detect::result_t> *results, int face_id)
{
    int x, y, w, h;
    uint32_t color = FACE_COLOR_YELLOW;
    if (face_id < 0)
    {
        color = FACE_COLOR_RED;
    }
    else if (face_id > 0)
    {
        color = FACE_COLOR_GREEN;
    }
    if(fb->bytes_per_pixel == 2){
        //color = ((color >> 8) & 0xF800) | ((color >> 3) & 0x07E0) | (color & 0x001F);
        color = ((color >> 16) & 0x001F) | ((color >> 3) & 0x07E0) | ((color << 8) & 0xF800);
    }
    int i = 0;
    for (std::list<dl::detect::result_t>::iterator prediction = results->begin(); prediction != results->end(); prediction++, i++)
    {
        // rectangle box
        x = (int)prediction->box[0];
        y = (int)prediction->box[1];

        // yが負の数のときにfb_gfx_drawFastHLine()でメモリ破壊してリセットする不具合の対策
        if(y < 0){
           y = 0;
        }

        w = (int)prediction->box[2] - x + 1;
        h = (int)prediction->box[3] - y + 1;

        //Serial.printf("x:%d y:%d w:%d h:%d\n", x, y, w, h);

        if((x + w) > fb->width){
            w = fb->width - x;
        }
        if((y + h) > fb->height){
            h = fb->height - y;
        }

        //サイレントモード時の顔追尾用
        g_face_centroid_x = x + w * 0.5;
        g_face_centroid_y = y + h * 0.5;

        //Serial.printf("x:%d y:%d w:%d h:%d\n", x, y, w, h);

        //fb_gfx_fillRect(fb, x+10, y+10, w-20, h-20, FACE_COLOR_RED);  //モザイク
        fb_gfx_drawFastHLine(fb, x, y, w, color);
        fb_gfx_drawFastHLine(fb, x, y + h - 1, w, color);
        fb_gfx_drawFastVLine(fb, x, y, h, color);
        fb_gfx_drawFastVLine(fb, x + w - 1, y, h, color);

#if TWO_STAGE
        // landmarks (left eye, mouth left, nose, right eye, mouth right)
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)prediction->keypoint[j];
            y0 = (int)prediction->keypoint[j+1];
            fb_gfx_fillRect(fb, x0, y0, 3, 3, color);
        }
#endif
    }
}


void debug_check_heap_free_size(void){
  Serial.printf("===============================================================\n");
  Serial.printf("Mem Test\n");
  Serial.printf("===============================================================\n");
  Serial.printf("esp_get_free_heap_size()                              : %6d\n", esp_get_free_heap_size() );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DMA)               : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DMA) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_SPIRAM)            : %6d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_INTERNAL)          : %6d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DEFAULT)           : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT) );

}


bool camera_capture_and_face_detect(){
  bool isDetected = false;

  //acquire a frame
  M5.In_I2C.release();
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    //Serial.println("Camera Capture Failed");
    M5.Display.println("Camera Capture Failed");
    return ESP_FAIL;
  }


  int face_id = 0;

#if TWO_STAGE
  HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
  HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
  std::list<dl::detect::result_t> &candidates = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
  std::list<dl::detect::result_t> &results = s2.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3}, candidates);
#else
  HumanFaceDetectMSR01 s1(0.3F, 0.5F, 10, 0.2F);
  std::list<dl::detect::result_t> &results = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
#endif


  
  if (results.size() > 0) {
    //Serial.printf("Face detected : %d\n", results.size());

    isDetected = true;

    fb_data_t rfb;
    rfb.width = fb->width;
    rfb.height = fb->height;
    rfb.data = fb->buf;
    rfb.bytes_per_pixel = 2;
    rfb.format = FB_RGB565;

    draw_face_boxes(&rfb, &results, face_id);

  }

  if(isSubWindowON){
    avatar.updateSubWindow(fb->buf);
  }

  //Serial.println("\n<<< heap size before fb return >>>");  
  //debug_check_heap_free_size();

  //return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  //Serial.println("<<< heap size after fb return >>>");  
  //debug_check_heap_free_size();

  return isDetected;
}
#endif  //ENABLE_FACE_DETECT

String sDateTime(tm* timeinfo){
  char cDT[64];
  printf(cDT,64,"%04d-%02d-%02d %02d:%02d:%02d" 
                ,timeinfo->tm_year + 1900
                ,timeinfo->tm_mon + 1
                ,timeinfo->tm_mday
                ,timeinfo->tm_hour
                ,timeinfo->tm_min
                ,timeinfo->tm_sec
                );
 return String(cDT);
}

void setClock() {
  //configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  configTime(3 * 3600, 0, "ntp.nict.jp", "jp.pool.ntp.org","ntp.jst.mfeed.ad.jp");//2024.04.23

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

//2024.04.23 独り言モード
unsigned long millis_monologue_stop;  //独り言を停止した時刻
#define MILLIS_MONOLOGUE_SHORT_INTERVAL 40000 //2024.06.17
#define MILLIS_MONOLOGUE_INTERVAL 5*60*1000 //会話後の独り言を停止する期間[ms]
unsigned long millis_monologue_interval;
int count_monologue;
bool prev_servo_home_monologue;
unsigned long millis_period_of_inactivity;//無操作時間[ms]
#define MILLIS_INACTIVITY_TO_SHUTDOWN 2*60*60*1000  //無操作からシャットダウンまでの時間[ms]

//2024.04.23 時刻表示
unsigned long millis_time_disp_last = 0;
unsigned long millis_time_disp_interval = 30*1000; 
static char speechTextBuff[64];   //2024.06.17 64 ->128

void setup()
{
  json_ChatString = payload1;


  millis_monologue_stop = millis();       //独り言を停止した時刻
  millis_monologue_interval = (int)MILLIS_MONOLOGUE_INTERVAL;
  count_monologue = 0;
  prev_servo_home_monologue = false;
  millis_period_of_inactivity = 0;
  auto cfg = M5.config();
  //2024.04.18 下記一行でStack-chan_Takao_Baseの背面USB TypeC ケーブルから電源供給してみる。
  cfg.output_power = false;  // Groveポートの5VをINに設定します。この設定で背面のUSB-C端子に接続するとM5Stackの電源とバッテリーの充電ができます。


  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT
//  cfg.output_power = true;
  M5.begin(cfg);

  preallocateBuffer = (uint8_t *)malloc(preallocateBufferSize);
  if (!preallocateBuffer) {
    M5.Display.printf("FATAL ERROR:  Unable to preallocate %d bytes for app\n", preallocateBufferSize);
    for (;;) { delay(1000); }
  }

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5.Speaker.config(spk_cfg);
  }
  M5.Speaker.begin();

  Servo_setup();
  delay(1000);

  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("setting", NVS_READONLY, &nvs_handle)) {
      size_t volume;
      uint8_t led_onoff;
      uint8_t speaker_no;
      nvs_get_u32(nvs_handle, "volume", &volume);
      if(volume > 255) volume = 255;
      M5.Speaker.setVolume(volume);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
      nvs_get_u8(nvs_handle, "led", &led_onoff);
      // if(led_onoff == 1) Use_LED = true;
      // else  Use_LED = false;
      nvs_get_u8(nvs_handle, "speaker", &speaker_no);
      if(speaker_no > 60) speaker_no = 3;
      TTS_SPEAKER_NO = String(speaker_no);
      TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
      nvs_close(nvs_handle);
    } else {
      if (ESP_OK == nvs_open("setting", NVS_READWRITE, &nvs_handle)) {
        size_t volume = 180;
        uint8_t led_onoff = 0;
        uint8_t speaker_no = 3;
        nvs_set_u32(nvs_handle, "volume", volume);
        nvs_set_u8(nvs_handle, "led", led_onoff);
        nvs_set_u8(nvs_handle, "speaker", speaker_no);
        nvs_close(nvs_handle);
        M5.Speaker.setVolume(volume);
        M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
        // Use_LED = false;
        TTS_SPEAKER_NO = String(speaker_no);
        TTS_PARMS = TTS_SPEAKER + TTS_SPEAKER_NO;
      }
    }
  }

  M5.Lcd.setTextSize(2);
  Serial.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
#ifndef USE_SDCARD
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  OPENAI_API_KEY = String(OPENAI_APIKEY);
  VOICEVOX_API_KEY = String(VOICEVOX_APIKEY);
  STT_API_KEY = String(STT_APIKEY);
  OPENWEATHER_API_KEY = String(OPENWEATHER_APIKEY);
#else
  /// settings
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    /// wifi
    auto fs = SD.open("/wifi.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      int y = 0;
      for(int x = 0; x < sz; x++) {
        if(buf[x] == 0x0a || buf[x] == 0x0d)
          buf[x] = 0;
        else if (!y && x > 0 && !buf[x - 1] && buf[x])
          y = x;
      }
      WiFi.begin(buf, &buf[y]);
    } else {
       WiFi.begin();
    }

    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle)) {
      /// radiko-premium
      fs = SD.open("/apikey.txt", FILE_READ);
      if(fs) {
        size_t sz = fs.size();
        char buf[sz + 1];
        fs.read((uint8_t*)buf, sz);
        buf[sz] = 0;
        fs.close();
  
        int y = 0;
        int z = 0;
        int zz = 0;
        for(int x = 0; x < sz; x++) {
          if(buf[x] == 0x0a || buf[x] == 0x0d)
            buf[x] = 0;
          else if (!y && x > 0 && !buf[x - 1] && buf[x])
            y = x;
          else if (!z && x > 0 && !buf[x - 1] && buf[x])
            z = x;
          else if (!zz && x > 0 && !buf[x - 1] && buf[x])
            zz = x;
        }

        nvs_set_str(nvs_handle, "openai", buf);
        nvs_set_str(nvs_handle, "voicevox", &buf[y]);
        nvs_set_str(nvs_handle, "sttapikey", &buf[z]);
        nvs_set_str(nvs_handle, "openweather", &buf[zz]);
        Serial.println("------------------------");
        Serial.println(buf);
        Serial.println(&buf[y]);
        Serial.println(&buf[z]);
        Serial.println(&buf[zz]);
        Serial.println("------------------------");
      }
      
      nvs_close(nvs_handle);
    }
    SD.end();
  } else {
    WiFi.begin();
  }

  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("apikey", NVS_READONLY, &nvs_handle)) {
      Serial.println("nvs_open");

      size_t length1;
      size_t length2;
      size_t length3;
      size_t length4;
      if(ESP_OK == nvs_get_str(nvs_handle, "openai", nullptr, &length1) && 
         ESP_OK == nvs_get_str(nvs_handle, "voicevox", nullptr, &length2) && 
         ESP_OK == nvs_get_str(nvs_handle, "sttapikey", nullptr, &length3) && 
         ESP_OK == nvs_get_str(nvs_handle, "openweather",nullptr, &length4) &&
        length1 && length2 && length3 && length4) {
        Serial.println("nvs_get_str");
        char openai_apikey[length1 + 1];
        char voicevox_apikey[length2 + 1];
        char stt_apikey[length3 + 1];
        char openweather_apikey[length4 + 1];
        if(ESP_OK == nvs_get_str(nvs_handle, "openai", openai_apikey, &length1) && 
           ESP_OK == nvs_get_str(nvs_handle, "voicevox", voicevox_apikey, &length2) &&
           ESP_OK == nvs_get_str(nvs_handle, "sttapikey", stt_apikey, &length3) &&
           ESP_OK == nvs_get_str(nvs_handle, "openweather", openweather_apikey, &length4)) {
          OPENAI_API_KEY = String(openai_apikey);
          VOICEVOX_API_KEY = String(voicevox_apikey);
          STT_API_KEY = String(stt_apikey);
          OPENWEATHER_API_KEY = String(openweather_apikey);
          // Serial.println(OPENAI_API_KEY);
          // Serial.println(VOICEVOX_API_KEY);
          // Serial.println(STT_API_KEY);
        }
      }
      nvs_close(nvs_handle);
    }
  }
  
#endif

  M5.Lcd.print("Connecting");
  Wifi_setup();
  M5.Lcd.println("\nConnected");
  Serial.printf_P(PSTR("Go to http://"));
  M5.Lcd.print("Go to http://");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());

  if (MDNS.begin("m5stack")) {
    Serial.println("MDNS responder started");
    M5.Lcd.println("MDNS responder started");
  }
  delay(1000);

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  // And as regular external functions:
  server.on("/speech", handle_speech);
  server.on("/face", handle_face);
  server.on("/chat", handle_chat);
  server.on("/apikey", handle_apikey);
  server.on("/setting", handle_setting);
  server.on("/apikey_set", HTTP_POST, handle_apikey_set);
  server.on("/role", handle_role);
  server.on("/role_set", HTTP_POST, handle_role_set);
  server.on("/role_get", handle_role_get);
  server.onNotFound(handleNotFound);

  if(!init_chat_doc(json_ChatString.c_str()))
  {
    //エラー表示
    Serial.println("setup Line=" + String(__LINE__) + "  init_chat_doc error");
    Serial.println("json_ChatString = ");
    Serial.println(json_ChatString);
  }
  // SPIFFSをマウントする
  if(SPIFFS.begin(true)){
    // JSONファイルを開く
    File file = SPIFFS.open("/data.json", "r");
    if(file){
      DeserializationError error = deserializeJson(chat_doc, file);
      if(error){
        Serial.println("Failed to deserialize JSON");
        if(!init_chat_doc(json_ChatString.c_str()))
        {
          //エラー表示
          Serial.println("setup  Line=" + String(__LINE__) + "  init_chat_doc error");
          Serial.println("json_ChatString = ");
          Serial.println(json_ChatString);
        }
      }
      serializeJson(chat_doc, InitBuffer);
      Role_JSON = InitBuffer;
      String json_str; 
      serializeJsonPretty(chat_doc, json_str);  // 文字列をシリアルポートに出力する
      Serial.println(json_str);
    } else {
      Serial.println("Failed to open file for reading");
      if(!init_chat_doc(json_ChatString.c_str()))
      {
        //エラー表示
        Serial.println("setup  Line=" + String(__LINE__) + "  init_chat_doc error");
        Serial.println("json_ChatString = ");
        Serial.println(json_ChatString);
      }
    }
  } else {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

  server.begin();
  Serial.println("HTTP server started");
  M5.Lcd.println("HTTP server started");  
  
  Serial.printf_P(PSTR("/ to control the chatGpt Server.\n"));
  M5.Lcd.print("/ to control the chatGpt Server.\n");
  delay(3000);

  audioLogger = &Serial;
  mp3 = new AudioGeneratorMP3();
//  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");

#if defined(ENABLE_FACE_DETECT)
  avatar.init(16);
#else
  //avatar.init();
  avatar.init(16);//2024.05.13
#endif
  avatar.addTask(lipSync, "lipSync");
  avatar.addTask(servo, "servo");
  avatar.setSpeechFont(&fonts::efontJA_16);

//2024.05.06  face color change
cps = new ColorPalette();
cps->set(COLOR_PRIMARY,TFT_BLACK);//目、口などの色
//cps->set(COLOR_BACKGROUND, (uint16_t)0xdab300);//背景色（顔色）
cps->set(COLOR_BACKGROUND, TFT_WHITE);//背景色（顔色）
avatar.setColorPalette(*cps);


//  M5.Speaker.setVolume(200);
  box_servo.setupBox(80, 120, 80, 80);

#if defined(ENABLE_FACE_DETECT)
  box_stt.setupBox(107, 0, M5.Display.width()-107, 80);
  box_subWindow.setupBox(0, 0, 107, 80);
#else
  box_stt.setupBox(0, 0, M5.Display.width(), 60);
#endif
  
  box_BtnA.setupBox(0, 100, 40, 60);
  box_BtnC.setupBox(280, 100, 40, 60);

#if defined(ENABLE_FACE_DETECT)
  camera_init();
  avatar.set_isSubWindowEnable(true);
  //avatar.setBatteryIcon(true);
  //avatar.setBatteryStatus(M5.Power.isCharging(), M5.Power.getBatteryLevel());
#endif

  //2024.04.23
  setClock();  // Required for X.509 validation
  timeClient.begin();
  millis_time_disp_last = millis();
  timeClient.update();

  //char s[64];
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
  String timeStr = hoursStr + ":" + minuteStr ;
  sprintf(speechTextBuff,"%s",timeStr.c_str());  // <===  例 17:03

  //2024.05.09 Set RTC time
  m5::rtc_time_t t;
  t.hours = timeInfo.tm_hour;
  t.minutes = timeInfo.tm_min;
  t.seconds = timeInfo.tm_sec;
  M5.Rtc.setTime(t);
  m5::rtc_date_t d;
  d.year = timeInfo.tm_year;
  d.month = timeInfo.tm_mon;
  d.date = timeInfo.tm_mday;
  M5.Rtc.setDate(d);

  avatar.setSpeechText(speechTextBuff);
  //2024.05.09  不安定のための再起動が多く、独り言が同じになりやすいことへの対策
  randomSeed(t.seconds);

  //2024.05.15 追加
#if defined( ENABLE_HEX_LED )
  hex_led_init();
  hex_led_ptn_boot();
#endif 

  Voicevox_tts((char*)"起動しました", (char*)TTS_PARMS.c_str());

  //2024.05.20 Button unit [U027]のテスト
  //pinMode(PB_IN0, INPUT);//INPUT_PULLUPでもダメ
 
}

//String random_words[18] = {"あなたは誰","楽しい","怒った","可愛い","悲しい","眠い","ジョークを言って","泣きたい","怒ったぞ","こんにちは","お疲れ様","詩を書いて","疲れた","お腹空いた","嫌いだ","苦しい","俳句を作って","歌をうたって"};
//2024.04.25
#define ELEMENTS_OF_RANDOM_WORDS 30
String random_words[ELEMENTS_OF_RANDOM_WORDS] = {
  "あなたは誰", //1
  "興味深い深海生物を一つ挙げて", //2
  "宇宙に関する面白い話を聞かせて", //3
  "自然は美しい", //4
   "少し眠い", //5
  "ジョークを言って", //6
  "本艦の状況を報告せよ", //7
  "お勧めの本を一冊挙げて", //8
  "人間の未来を予想して", //9
  "コーヒーブレイクしよう", //10
  "諺を一つ説明して", //11
  "銀河系内で興味深い星を挙げて", //12
  "生物に関する蘊蓄を語って", //13
  "ワープ７で発進せよ", //14
  "お勧めのワインを紹介して", //15
  "ヴァルカンの習慣を語って", //16
  "科学トピックスを語って", //17
  "生命の神秘を語って",  //18
  "楽しい", //19
  "思い付き",  //20
  "元気か",  //21
  "美味しい野草",//22
  "毒を持つ魚",//23
  "きのこの話をして",//24
  "蟹の話をして",//25
  "旧車の話をして",//26
  "最新の車の話をして",//27
  "F1の話をして",//28
  "魔法の話をして",//29
  "星座の話をして"//30
 };
int random_time = -1;
bool random_speak = true;
static int lastms1 = 0;

void report_batt_level(){
  char buff[100];
   int level = M5.Power.getBatteryLevel();
  if(M5.Power.isCharging())
    sprintf(buff,"充電中、バッテリーのレベルは%d％です。",level);
  else
    sprintf(buff,"バッテリーのレベルは%d％です。",level);
  avatar.setExpression(Expression::Happy);
  Voicevox_tts(buff, (char*)TTS_PARMS.c_str());
  avatar.setExpression(Expression::Neutral);
  Serial.println("mp3 begin");
}

void switch_monologue_mode(){
    String tmp;
    if(random_speak) {
      tmp = "独り言始めます。";
      lastms1 = millis();
      random_time = MILLIS_MONOLOGUE_SHORT_INTERVAL + 1000 * random(30);
    } else {
      tmp = "独り言やめます。";
      random_time = -1;
    }
    random_speak = !random_speak;
    avatar.setExpression(Expression::Happy);
    Voicevox_tts((char*)tmp.c_str(), (char*)TTS_PARMS.c_str());
    avatar.setExpression(Expression::Neutral);
    Serial.println("mp3 begin");
}

// robo8080さんの音声認識を関数化
void voice_recognition(void)
{
  M5.Speaker.tone(1000, 100);
  delay(200);
  M5.Speaker.end();
  bool prev_servo_home = servo_home;
  random_speak = true;
  random_time = -1;
#ifdef USE_SERVO
  servo_home = true;
#endif
 
#if defined( ENABLE_HEX_LED )
          hex_led_ptn_wake();
#endif

 avatar.setExpression(Expression::Happy);
  avatar.setSpeechText("御用でしょうか？");
  M5.Speaker.end();
  String ret;
  if(OPENAI_API_KEY != STT_API_KEY){
    Serial.println("Google STT");
    ret = SpeechToText(true);
  } else {
    Serial.println("Whisper STT");
    ret = SpeechToText(false);
  }
#ifdef USE_SERVO
  servo_home = prev_servo_home;
#endif
  Serial.println("音声認識終了");
  Serial.println("音声認識結果");
  if(ret != "") {
    Serial.println(ret);
    if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
      exec_chatGPT(ret);
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_accept();
#endif
    }
  } else {
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_off();
#endif
    Serial.println("音声認識失敗");
    avatar.setExpression(Expression::Sad);
    avatar.setSpeechText("聞き取れませんでした");
    delay(2000);
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);
  } 
  M5.Speaker.begin();

}

  //2024.05.20 Button unit [U027]でシャットダウン
  //uint PB_IN_pressed_millis = 0;
  //int PB_IN_prev_state = HIGH;  //press = LOW (0V)
  //const uint PRESS_TIME_TO_SHUTDOWN = 5000;

#if defined( ENABLE_HEX_LED ) //2024.05.15 追加
//無操作時の自動電源OFFを知らせる  
void hex_led_ptn_PowerOff(void)
{
#if defined( ENABLE_HEX_LED )
  hex_led_ptn_pow_off_1of5();
  delay(1000);
  hex_led_ptn_pow_off_2of5();
  delay(1000);
  hex_led_ptn_pow_off_3of5();
  delay(1000);
  hex_led_ptn_pow_off_4of5();
  delay(1000);
  hex_led_ptn_pow_off_5of5();
  delay(1000);
  hex_led_ptn_off();
#endif
}

//2024.05.24 点灯パターン確認用
void hex_led_ptn_test_blinking(void)
{
#if defined( ENABLE_HEX_LED )
  hex_led_ptn_test();
  delay(1000);
  hex_led_ptn_off();
  delay(1000);
  hex_led_ptn_test();
  delay(1000);
  hex_led_ptn_off();
  delay(1000);
  hex_led_ptn_test();
  delay(1000);
  hex_led_ptn_off();
  delay(1000);
#endif
}
#endif

void loop()
{
  static int lastms = 0;
  //2024.04.23 独り言長期インターバル制御
  if(millis() - millis_monologue_stop > millis_monologue_interval)
  {
    random_time = MILLIS_MONOLOGUE_SHORT_INTERVAL;//独り言モードになる。
    if(count_monologue >= 10) {//喋りすぎなので一旦停止
      random_time = -1;   //通常の一問一答
      count_monologue = 0;
      millis_monologue_stop = millis();	//独り言を停止した時刻
      
      //2024.05.21
      millis_period_of_inactivity += millis_monologue_interval;
      if(millis_period_of_inactivity > MILLIS_INACTIVITY_TO_SHUTDOWN)
      {//シャットダウン
        Voicevox_tts((char*)"おやすみなさい", (char*)TTS_PARMS.c_str());
        delay(5000);
#if defined( ENABLE_HEX_LED )
        hex_led_ptn_PowerOff();//2024.05.23
#endif
        M5.Power.powerOff();
      }

      millis_monologue_interval *= 2; //停止時間を延ばす
      if(millis_monologue_interval > MILLIS_MONOLOGUE_INTERVAL*3) {
        //サーボ停止
        prev_servo_home_monologue = false;
        servo_home = true;
      }
      //2024.05.14
      avatar.setExpression(Expression::Sleepy);

    }
  }
  else
  {
    random_time = -1;   //通常の一問一答
  }
  //2024.04.23 独り言短期繰り返し実行
  if (random_time >= 0 && millis() - lastms1 > random_time)
  {
    //count_monologue++;  //独り言 　連続実行回数カウント
    lastms1 = millis();
    random_time = MILLIS_MONOLOGUE_SHORT_INTERVAL + 1000 * random(30);
    if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
      //exec_chatGPT(random_words[random(18)]);
      Serial.println("exec_chatGPT -1-独り言");
      count_monologue++; 
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_notification();
#endif
      /////////////////////
      exec_chatGPT(random_words[random(ELEMENTS_OF_RANDOM_WORDS)]);//2024.05.10 <==== random_words[]の要素数
      /////////////////////
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_off();
#endif
    }
  }


#if defined(ENABLE_FACE_DETECT)
  //しゃべっていないときに顔検出を実行し、顔が検出されれば音声認識を開始。
  if (!mp3->isRunning()) {
    bool isFaceDetected;
    isFaceDetected = camera_capture_and_face_detect();
    
    //2024.04.23 時刻表示
    if(!isFaceDetected) 
    {
      // NTPサーバへの頻繁なアクセスは減らすべき
      unsigned long millis_now = millis();
      if(millis_now - millis_time_disp_last > millis_time_disp_interval)
      {
        millis_time_disp_last = millis_now;
        timeClient.update();
        String sFmtDT = timeClient.getFormattedTime();
        Serial.println(sFmtDT);  // <===  例 17:03:16
        char s[64];
        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();
        String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
        String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
        String timeStr = hoursStr + ":" + minuteStr ;
        sprintf(speechTextBuff,"%s",timeStr.c_str());  // <===  例 17:03
        avatar.setSpeechText(speechTextBuff);
      }
    }
  
    if(!isSilentMode){
      //顔検出してたら　音声認識とexec_chatGPT
      if(isFaceDetected){

        avatar.set_isSubWindowEnable(false);
        Serial.println("voice_recognition -1-顔検出");

        /////////////////////
        voice_recognition();                    //音声認識
        /////////////////////
        //2024.04.23 独り言モード

        millis_monologue_stop = millis();       //独り言を停止した時刻
        millis_monologue_interval = MILLIS_MONOLOGUE_INTERVAL;
        count_monologue = 0;
        servo_home = prev_servo_home_monologue;
        millis_period_of_inactivity = 0;

        //exec_chatGPT(random_words[random(18)]);   //独り言

        // フレームバッファを読み捨てる（ｽﾀｯｸﾁｬﾝが応答した後に、過去のフレームで顔検出してしまうのを防ぐため）
        M5.In_I2C.release();
        camera_fb_t *fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);

      }

    }
    else{
      //サイレントモードでは音声認識等をせず、表情のみ(^_^)
      if(isFaceDetected){
        avatar.setExpression(Expression::Happy);
      }
      else{
        avatar.setExpression(Expression::Neutral);
      }
    }
  }

#else
  //2024.04.23 時刻表示
  if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
  {
    // NTPサーバへの頻繁なアクセスは減らすべき
    unsigned long millis_now = millis();
    if(millis_now - millis_time_disp_last > millis_time_disp_interval)
    {
      //2024.05.13
      millis_time_disp_last = millis_now;
      timeClient.update();
      String sFmtDT = timeClient.getFormattedTime();
      //Serial.println(sFmtDT);  // <===  例 17:03:16
      //char s[64];
      int hours = timeClient.getHours();
      int minutes = timeClient.getMinutes();
      String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
      String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
      String timeStr = hoursStr + ":" + minuteStr ;
      sprintf(speechTextBuff,"%s",timeStr.c_str());  // <===  例 17:03
      avatar.setSpeechText(speechTextBuff);
    }
  }
  }
#endif

#if false //2024.05.12 CoreS3
  if (M5.BtnA.wasPressed())
  {
    M5.Speaker.tone(1000, 100);
    switch_monologue_mode();
  }
#endif
  // if (Serial.available()) {
  //   char kstr[256];
  //   size_t len = Serial.readBytesUntil('\r', kstr, 256);
  //   kstr[len]=0;
  //   avatar.setExpression(Expression::Happy);
  //   VoiceText_tts(kstr, tts_parms2);
  //   avatar.setExpression(Expression::Neutral);
	// }

  M5.update();
 
 #if 0
  if(!mp3->isRunning())
  {
    M5.Lcd.setCursor(160, 60);
    M5.Lcd.print(digitalRead(PB_IN));
    delay(200);
  }


  //2024.05.20 Button unit [U027]のテスト
  if(digitalRead(PB_IN)==LOW)
  {//ボタンが押されている
    uint millis_now = millis();
    if(PB_IN_prev_state == HIGH) {//今押された
      PB_IN_pressed_millis = millis_now;
      PB_IN_prev_state = LOW;
      Serial.println("<---");
    }
    else if(millis_now - PB_IN_pressed_millis > PRESS_TIME_TO_SHUTDOWN){//長押し
      //シャットダウンを実行する
      Serial.println("L*");
    }
    else{
      Serial.println("L");
    }
  } else {
    PB_IN_prev_state = HIGH;
    Serial.println("     H");
  }
#endif


#if defined(ARDUINO_M5STACK_Core2) || defined( ARDUINO_M5STACK_CORES3 )
  //Serial.println("in : #if defined(ARDU...");
  auto count = M5.Touch.getCount();
  if (count)
  {
    //Serial.println("in : if(count){}");
    auto t = M5.Touch.getDetail();
    if (t.wasPressed())
    {          
      //Serial.println("in : if (t.wasPressed()){}");
      if (box_stt.contain(t.x, t.y)&&(!mp3->isRunning()))
      {//画面上のボタンを押して音声認識させる
        //Serial.println("in : if (box_stt.contain{ voice_recognition }");
#if defined(ENABLE_FACE_DETECT)
        avatar.set_isSubWindowEnable(false);
#endif

        Serial.println("voice_recognition -2-ボタン操作");
        
        /////////////////////
        voice_recognition();
        /////////////////////

        //Serial.println("after : voice_recognition");
        //2024.04.23 独り言モード
        millis_monologue_stop = millis();       //独り言を停止した時刻
        millis_monologue_interval = MILLIS_MONOLOGUE_INTERVAL;
        millis_period_of_inactivity = 0;
        count_monologue = 0;
        servo_home = prev_servo_home_monologue;
 #if 0
        M5.Speaker.tone(1000, 100);
        delay(200);
        M5.Speaker.end();
        bool prev_servo_home = servo_home;
        random_speak = true;
        random_time = -1;
#ifdef USE_SERVO
        servo_home = true;
#endif

#if defined( ENABLE_HEX_LED )
          hex_led_ptn_wake();
#endif

        avatar.setExpression(Expression::Happy);
        avatar.setSpeechText("御用でしょうか？");
        M5.Speaker.end();
        String ret;
        if(OPENAI_API_KEY != STT_API_KEY){
          Serial.println("Google STT");
          ret = SpeechToText(true);
        } else {
          Serial.println("Whisper STT");
          ret = SpeechToText(false);
        }
#ifdef USE_SERVO
        servo_home = prev_servo_home;
#endif
        Serial.println("音声認識終了");
        Serial.println("音声認識結果");
        if(ret != "") {
          Serial.println(ret);
          if (!mp3->isRunning() && speech_text=="" && speech_text_buffer == "") {
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_accept();
#endif
            exec_chatGPT(ret);
          }
        } else {
#if defined( ENABLE_HEX_LED )
    hex_led_ptn_off();
#endif
          Serial.println("音声認識失敗");
          avatar.setExpression(Expression::Sad);
          avatar.setSpeechText("聞き取れませんでした");
          delay(2000);
          avatar.setSpeechText("");
          avatar.setExpression(Expression::Neutral);
        } 
        M5.Speaker.begin();
#endif
      }

      
#ifdef USE_SERVO
      if (box_servo.contain(t.x, t.y))
      {
        servo_home = !servo_home;
        g_face_centroid_x = CENTER_X;
        g_face_centroid_y = CENTER_Y;
        g_angle_x = 0;
        g_angle_y = 0;
        M5.Speaker.tone(1000, 100);
      }
#endif
      //画面上のタッチ操作
      if (box_BtnA.contain(t.x, t.y))
      {//顔認識時はサイレントモードON/OFF、そうでないときは独り言ON/OFF
#if defined(ENABLE_FACE_DETECT)
        isSilentMode = !isSilentMode;
        if(isSilentMode){
          avatar.setSpeechText("サイレントモード");
        }
        else{
          avatar.setSpeechText("サイレントモード解除");
        }
        delay(2000);
        avatar.setSpeechText("");
#else
        M5.Speaker.tone(1000, 100);
        switch_monologue_mode();
#endif
      }
      if (box_BtnC.contain(t.x, t.y))
      {//バッテリー残量発話
#if defined(ENABLE_FACE_DETECT)
        avatar.set_isSubWindowEnable(false);
#endif
        M5.Speaker.tone(1000, 100);
        report_batt_level();
      }
      //カメラ映像の表示／非表示
#if defined(ENABLE_FACE_DETECT)
      if (box_subWindow.contain(t.x, t.y))
      {
        isSubWindowON = !isSubWindowON;
        avatar.set_isSubWindowEnable(isSubWindowON);
      }
#endif //ENABLE_FACE_DETECT
    }
  }
#endif

  if (M5.BtnC.wasPressed())
  {//バッテリー残量発話
    M5.Speaker.tone(1000, 100);
    report_batt_level();
  }
  //文言があれば発話
  if(speech_text != ""){
    if(millis_monologue_interval <= 3*MILLIS_MONOLOGUE_INTERVAL) avatar.setExpression(Expression::Happy);//2024.05.14 条件を付ける
    speech_text_buffer = speech_text;
    speech_text = "";

    Voicevox_tts((char*)speech_text_buffer.c_str(), (char*)TTS_PARMS.c_str());
  }
  //発話終了したのでMP3停止
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      if(file != nullptr){delete file; file = nullptr;}
      Serial.println("mp3 stop");
      if(millis_monologue_interval <= 3*MILLIS_MONOLOGUE_INTERVAL) avatar.setExpression(Expression::Neutral);//2o24.05.14 条件を付ける
      speech_text_buffer = "";

#if defined(ENABLE_FACE_DETECT)
      if(isSubWindowON){
        avatar.set_isSubWindowEnable(true);
      }
#endif  //ENABLE_FACE_DETECT
    }
    delay(1);
  } else {
  server.handleClient();
  }
//delay(100);
}
 
