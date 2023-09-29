---
title: "WA-MIKANを単体で使う（その2）"
---

引き続き、WA-MIKANを単体で使う方法を紹介していきます。

この章では以下のようなものを作成していきます。

* NTPで時刻合わせ
* Slackメッセージ送受信
* Slackを介してエアコンを制御


## Wi-Fiを使ってみる

次はWi-Fiを使ったプログラムをいくつか試してみましょう。

### NTPでESP8266のRTCを設定する

まずはWi-Fiを使った簡単な例として、NTPを使ってESP8266のRTC (Real Time Clock)を設定してみましょう。

```CPP
#include <ESP8266WiFi.h>
#include <time.h>

#define SSID    "**************"
#define PASSWD  "*************"

// Set the clock using NTP.
void setClock() {
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  Serial.print("Waiting for NTP time sync: ");
  time_t now;
  while ((now = time(nullptr)) < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  Serial.printf("Current time (JST): %s", asctime(&timeinfo));
}

void setup() {
  Serial.begin(115200);

  WiFi.setOutputPower(0.0);   // Set to minimum power. (0.0 - 20.5 dBm)
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWD);

  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  setClock();
}

void loop() {
}
```

`setup()` 関数内の `WiFi.setOutputPower()` でWi-Fiの出力強度を設定しています。0.0 ~ 20.5 dBm の範囲で強度を設定できます。この設定は必須ではありませんが、通信に問題がない範囲で小さい値を指定することでESP8266の消費電力を下げることができます。ESP8266を電池で動かしたいときには有効でしょう。
`WiFi.mode()` でSTA(ステーション)モードに設定し、`WiFi.begin()` でSSIDとパスワードを指定することでWi-Fi接続が開始されます。
`WiFi.status()` で接続状況が確認できます。
`WiFi.localIP()` でIPアドレスを知ることができます。

`setClock()` 関数でNTPサーバーとの同期を行います。この関数は[ESP8266のサンプルコード](https://github.com/esp8266/Arduino/blob/7f2deb14a254d0d1b0903e0bb15a87815833f579/libraries/ESP8266WiFi/examples/BearSSL_Validation/BearSSL_Validation.ino#L27-L43)を元にしています。
`configTzTime()` でタイムゾーンとNTPサーバーを指定すると、NTPサーバーへの接続が試みられます。第1引数でタイムゾーンを指定し、第2引数以降(最大3つまで)でNTPサーバーを指定します。
RTCが正しく設定されれば、`time()` で現在の時刻を取得することができます。取得した時間は `localtime_r()` でローカルタイムに変換して、`asctime()` で表示可能な文字列に変換することができます。

実行例:
```
........
WiFi connected
IP address: 192.168.0.4
Waiting for NTP time sync: .
Current time (JST): Mon Sep 11 23:58:20 2023
```


### Slackにメッセージを送信する

次はWA-MIKANからSlackにメッセージを送信してみましょう。


#### HTTPS接続

Slackにメッセージを送信するにはHTTPS接続が必要です。ESP8266でHTTPS接続を行うためには[BearSSL WiFi Classes](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html)を使うことになります。
ここで出てくる[BearSSL](https://bearssl.org/)とはSSL/TLSライブラリーの1つで、サイズが小さいのが特徴です。(他の有名なSSL/TLSライブラリーとしては[OpenSSL](https://www.openssl.org/)やそのフォークの[LibraSSL](https://www.libressl.org/)などがあります。)

ヘッダーファイルには [`WiFiClientSecure.h`](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h)と [`WiFiClientSecureBearSSL.h`](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecureBearSSL.h) の2つがありますが、`WiFiClientSecure.h` の実体は

```CPP
#include "WiFiClientSecureBearSSL.h"

using namespace BearSSL;
```

となっているだけですので、実質的には同じものです。`WiFiClientSecure.h` を使った場合には、シンボルの頭に `BearSSL::` を付ける必要がなくなるという点だけが異なります。

BearSSL WiFi Classesを使ったHTTPS接続の例は [`BearSSL_Validation.ino`](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_Validation/BearSSL_Validation.ino) にあります。

`BearSSL::WiFiClientSecure` は、サーバーの証明書の確認方法としていくつかの方法を提供しています。

* `setInsecure()`  
  証明書のチェックを行いません。安全ではありません。
* `setKnownKey()`  
  証明書のチェックの代わりに、サーバーの公開鍵のチェックを行います。サーバーが公開鍵を更新した場合は、通信に失敗します。
* `setFingerprint()`  
  証明書のチェックの代わりに、証明書のフィンガープリントを使ってチェックを行います。サーバーが証明書を更新した場合は、通信に失敗します。簡易的なチェックであるため、安全性は劣りますが、消費電力は少ないです。
* `setTrustAnchors()`  
  指定されたルート証明書を使ってサーバー証明書のチェックを行います。

電力や性能的な問題がなければ、`setTrustAnchors()` を使うやり方が良いでしょう。

これらの関数を使うために必要なデータは、ESP8266用Arduinoコアライブラリのリポジトリの中にある [`tools/cert.py`](https://github.com/esp8266/Arduino/blob/master/tools/cert.py) というスクリプトで取得できます。
`-s` オプションで、接続したいサーバーを指定します。今回は `slack.com` です。

```shell-session
$ cert.py -s slack.com > certs.h
```

スクリプトを実行する際に、cryptographyが見つからないというエラーが出た時は、以下のコマンドでインストールできます。

```shell-session
$ pip install cryptography
```

生成された [`certs.h`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/certs.h) の中身は以下のようになっています。

```CPP:certs.h
...

////////////////////////////////////////////////////////////
// certificate chain for slack.com:443

// CN: slack.com => name: slack_com
// not valid before: 2023-07-16 04:41:07
// not valid after:  2023-10-14 04:41:06
const char fingerprint_slack_com [] PROGMEM = "...";
const char pubkey_slack_com [] PROGMEM = R"PUBKEY(
-----BEGIN PUBLIC KEY-----
...
-----END PUBLIC KEY-----
)PUBKEY";

// http://r3.i.lencr.org/
// CN: R3 => name: R3
// not valid before: 2020-09-04 00:00:00
// not valid after:  2025-09-15 16:00:00
const char cert_R3 [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)CERT";

// http://x1.i.lencr.org/
// CN: ISRG Root X1 => name: ISRG_Root_X1
// not valid before: 2015-06-04 11:04:38
// not valid after:  2035-06-04 11:04:38
const char cert_ISRG_Root_X1 [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)CERT";
```

フィンガープリント (`fingerprint_slack_com`) やサーバーの公開鍵 (`pubkey_slack_com`) を使う場合は、有効期限が3か月ほどしかないことが分かります。期限が切れた場合は `certs.h` を再生成する必要があります。
ルート証明書を使う場合は、`cert_ISRG_Root_X1` を指定します。

```CPP
X509List cert(cert_ISRG_Root_X1);

void loop() {
  WiFiClientSecure client;
  HTTPClient https;

  client.setTrustAnchors(&cert);

  ...
}
```

1つのサーバーに接続するだけならば以上の方法で十分ですが、事前に接続先サーバーのルート証明書を特定できない場合や何十個ものルート証明書を使う必要がある場合は、サンプルの [`BearSSL_CertStore.ino`](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_CertStore/BearSSL_CertStore.ino) にあるように `BearSSL::CertStore` を使うことでメモリ使用量を削減できます。


#### Slackへのアプリの登録

まずは、WA-MIKANとやり取りするためのワークスペースを用意します。
[Slackのトップページ](https://slack.com/intl/ja-jp/)の「ワークスペースを新規作成」ボタンから新しいワークスペースを作成できます。

次に[SlackのAPIページ](https://api.slack.com/)からアプリを登録します。
ページの右上の[Your apps](https://api.slack.com/apps)をクリックし、"Create New App" ボタンをクリックすると、"Create an app" ダイアログボックスが開きます。
"From scratch" を選択すると、アプリ名の入力欄とどのワークスペースにアプリを登録するかを選択するドロップダウンリストが表示されます。
今回はApp Nameはiot-botとし、ワークスペースは今回作成した個人用ワークスペースを指定しました。
"Create App" ボタンを押すとアプリ（の設定）が作成されて、画面が切り替わります。

次に、アプリのスコープを設定してトークンを発行します。
左側のリストから "OAuth & Permissions" を選択するか、右側の "Permissions" パネルを選択します。
ページが切り替わったら、少し下の方に行くと Scope 欄があります。その中に "Bot Token Scopes" と "User Token Scopes" がありますが、"Bot Token Scopes" の "Add an OAuth Scope" ボタンをクリックし、"chat:write" を選択します。(メッセージを送信するには "chat:write" スコープが必要です。)
ページの上の方に戻り、"Install to Workspace" ボタンをクリックします。「許可する」ボタンをクリックしてワークスペースにアプリをインストールします。
ワークスペースへのインストールが成功すると、`xoxb-` で始まる Bot User OAuth Token というものが発行されます。APIを使うにはこのトークンが必要となります。

さて、Slackにアプリが正しく登録できたかを確かめるために、curlコマンドを使ってメッセージを送信してみましょう。
Slackにメッセージを送信するには、[chat.postMessage](https://api.slack.com/methods/chat.postMessage) APIを使います。
`token=` の部分には先ほど発行されたトークンを指定します。`channel=` の部分にはメッセージを送信する先のチャンネルのIDを指定します。WebブラウザでSlackのチャンネルを開いた際、URLの末尾の "C" で始まるディレクトリ部分がチャンネルのIDになります。

```shell-session
$ curl -X POST 'https://slack.com/api/chat.postMessage' -d 'token=xoxb-*************-*************-************************' -d 'channel=C**********' -d 'text=Hello+World'
```

うまくいけば、指定したチャンネルに "Hello World" と表示されます。
メッセージはx-www-form-urlencoded形式で送信しますので、`+` はスペースとして表示される点に注意してください。

curlコマンドで `-d` オプションを複数指定した場合、文字列を `&` でつないで並べたことと同じになります。つまり、以下のように指定しても同じ結果になります。

```shell-session
$ curl -X POST 'https://slack.com/api/chat.postMessage' -d 'token=xoxb-*************-*************-************************&channel=C**********&text=Hello+World'
```


#### ESP8266からメッセージを送信する

curlコマンドでのメッセージ送信が確認できたら、同じことをESP8266からやってみましょう。

以下のコードを実行すると、1分ごとに指定したチャンネルに "Hello World" というメッセージを送信します。

```CPP
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// https://github.com/esp8266/Arduino/blob/master/tools/cert.py
// $ cert.py -s slack.com > certs.h
#include "certs.h"

X509List cert(cert_ISRG_Root_X1);

#define SSID    "**************"
#define PASSWD  "*************"

const char* slack_host = "slack.com";
const uint16_t slack_port = 443;

const char* slack_token = "xoxb-*************-*************-************************";
const char* slack_channel_id = "C**********";


// Set the clock using NTP.
void setClock() {
  ...
}

void setup() {
  ...
}

String urlencode(String s) {
  const char *t = "0123456789ABCDEF";
  String e = "";
  for (unsigned int i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (isalnum(c) /* || c == '*' || c == '-' || c == '.' || c == '_' */) {
      e += c;
    } else if (c == ' ') {
      e += '+';
    } else {
      e += '%';
      e += t[(c >> 4) & 0xf];
      e += t[c & 0xf];
    }
  }
  return e;
}

// Post a message to the slack channel.
void post_message(String text) {
  WiFiClientSecure client;
  HTTPClient https;

  client.setTrustAnchors(&cert);

  if (https.begin(client, slack_host, slack_port, "/api/chat.postMessage")) {  // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = String("token=") + slack_token + "&channel=" + slack_channel_id + "&text=" + urlencode(text);
    int httpCode = https.POST(body);

    if (httpCode > 0) {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
}

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    post_message("Hello World");
  }

  Serial.println("Wait 1min before next round...");
  delay(60*1000);
}
```

`setClock()` と `loop()` は、前述のNTPのコードと同じですので省略してあります。X.509による証明書の確認のためには現在時刻が正しい必要があるため、NTPによる時刻合わせを行っておく必要があります。

`urlencode()` は送信メッセージをx-www-form-urlencoded形式でエンコードするための関数です。URLエンコードを行うArduino用のライブラリも存在するようですが、大した処理ではないので自前で実装しています。

`post_message()` がメッセージをSlackに送信するための関数です。`https://slack.com:443/api/chat.postMessage` に対して、POSTを発行することでメッセージを送信します。curlを使った場合の2つ目の例と同様に、`token=`, `channel=`, `text=` を `&` でつなげた文字列をPOSTしています。

ソースコード全体は [`sketch_slack_hello.ino`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/sketch_slack_hello.ino) から取得できます。


### Slackからメッセージを受信する

Slackからメッセージを受信するにはいくつか方法がありますが、今回は[conversations.history](https://api.slack.com/methods/conversations.history) APIを使用することにします。
このAPIを使うには "channels:history" スコープが必要です。[SlackのAPIページ](https://api.slack.com/)の[Your apps](https://api.slack.com/apps)ボタンをクリックすると、先ほど作成したアプリが見つかるはずです。そのアプリをクリックし、"Add features and functionality" をクリックすると "Permissions" パネルが出てくるのでそれを選択します。先ほどと同じように "Bot Token Scopes" の "Add an OAuth Scope" ボタンをクリックし、"channels:history" を追加します。アプリを再インストールするように促されますので、再インストールするとconversations.history APIが使えるようになります。

conversations.history APIはいくつかのオプションを持っていますが、`limit` オプションで取得するメッセージ数を指定できます。今回は `limit=1` とすることで、指定したチャンネルの最新のメッセージを1つだけ取得するようにしてみます。

以下のコードを実行すると、1分ごとに最新のメッセージを取得して、Slack APIのレスポンスをそのままシリアルに表示します。
`setup()` 関数、`setClock()` 関数やグローバル変数などはSlackへのメッセージ送信と同じなので省略します。

```CPP
// Get the latest message from the slack channel.
String get_slack_message() {
  WiFiClientSecure client;
  HTTPClient https;
  String payload;

  client.setTrustAnchors(&cert);

  if (https.begin(client, slack_host, slack_port, "/api/conversations.history")) {  // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = String("token=") + slack_token + "&channel=" + slack_channel_id + "&limit=1";
    int httpCode = https.POST(body);

    if (httpCode > 0) {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        payload = https.getString();
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return payload;
}

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    String payload = get_slack_message();
    if (!payload.isEmpty()) {
      Serial.println(payload);
    }
  }

  Serial.println("Wait 1min before next round...");
  delay(60*1000);
}
```

ソースコード全体は [`sketch_slack_get_message.ino`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/sketch_slack_get_message.ino) から取得できます。

実行結果は以下のようになります。

```
....
Current time (JST): Wed Sep 27 02:03:07 2023
[HTTPS] POST... code: 200
{"ok":true,"messages":[{"bot_id":"B05RG1XH3B2","type":"message","text":"Hello World","user":"U05QKM8J006","ts":"1695747523.687079","app_id":"A05QS7N8K9R","blocks":[{"type":"rich_text","block_id":"B3p","elements":[{"type":"rich_text_section","elements":[{"type":"text","text":"Hello World"}]}]}],"team":"T05PWGWLT1B","bot_profile":{"id":"B05RG1XH3B2","deleted":false,"name":"iot-bot","updated":1693763129,"app_id":"A05QS7N8K9R","icons":{"image_36":"https:\/\/a.slack-edge.com\/80588\/img\/plugins\/app\/bot_36.png","image_48":"https:\/\/a.slack-edge.com\/80588\/img\/plugins\/app\/bot_48.png","image_72":"https:\/\/a.slack-edge.com\/80588\/img\/plugins\/app\/service_72.png"},"team_id":"T05PWGWLT1B"}}],"has_more":true,"pin_count":0,"channel_actions_ts":null,"channel_actions_count":0,"response_metadata":{"next_cursor":"bmV4dF90czoxNjk1NzQ3NDYyMDc0NTI5"}}
Wait 1min before next round...
```

このように、SlackからのデータはJSON形式で返ってきます。
このSlackからのレスポンスを `slack_response.json` というファイルに保存してしたうえでjqコマンドを使えば、以下のようにしてメッセージのテキストを抽出することができます。

```shell-session
$ cat slack_response.json | jq '.messages[0].text'
"Hello World"
```

同様なことをWA-MIKANでやってみましょう。
ArduinoでJSONを解析するには、ArduinoJSONというライブラリが使えます。

```CPP
#include <ArduinoJson.h>

// Parse a slack response.
void parse_response(String payload) {
  DynamicJsonDocument doc(2048);

  if (deserializeJson(doc, payload)) {
    Serial.println("JSON parse error.");
    return;
  }
  if (!doc["ok"].as<bool>()) {
    Serial.println("Slack error.");
    return;
  }
  String type = doc["messages"][0]["type"].as<const char*>();
  if (type != "message") {
    Serial.println("Not a message.");
    return;
  }
  String text = doc["messages"][0]["text"].as<const char*>();

  Serial.println(text);
}
```

`DynamicJsonDocument` オブジェクトを作成し、`deserializeJson()` 関数を呼び出すことでJSONをパースできます。
念のため、"ok" が `true` であることを確認し、最新のメッセージの "type" が "message" であることを確認してから `doc["messages"][0]["text"]` でメッセージのテキストを抽出しています。データを抽出する際、型は自動で変換される場合もありますが、型を明示して取り出すには `as<T>()` 関数を呼び出します。

`loop()` 関数を以下のように修正して実行すれば、先ほどの長いJSON形式のレスポンスの代わりに、最新のメッセージのテキスト（今回の場合 "Hello World"）がシリアル端末に表示されます。

```CPP
void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    String payload = get_slack_message();
    if (!payload.isEmpty()) {
      //Serial.println(payload); // この行をコメントアウト
      parse_response(payload);   // この行を追加
    }
  }

  Serial.println("Wait 1min before next round...");
  delay(60*1000);
}
```


## エアコン制御システムの作成

10章からここまでやってきたことを統合して、Slack経由でエアコンを制御するシステムを作成します。
今回は以下のような仕様とします。

1. 1分ごとに現在の温度湿度を測定し、LDCに表示。
2. その後、Slackのチャンネルの最新メッセージを取得。
3. 取得したメッセージが3分以内に送信されたもので、ボット宛 (`@iot_bot`) だったらメッセージを解析。
4. メッセージの解析結果に応じてエアコンを操作。
5. 操作内容をSlackに送信するとともに、LCDに表示。

温度湿度の測定にはAHT25を使います。4章で作成したGR-CITRUS用のAHT25クラスをほぼそのままESP8266に移植します。

メッセージの解析は次のようにします。

1. メッセージは `@iot_bot ` で始まっているものを受け付ける。  
   `@` を使った指定ユーザーあてのメッセージは、テキストでは `<@U05QKM8J006> ` のように `U` で始まるユーザーIDを使って表記されます。
2. `@iot_bot ` のあとは、アルファベット1文字以上のコマンドを指定する。コマンドのあとにパラメーターが必要な場合は、スペースを1つ空けてパラメーターを指定する。  
   サポートするコマンドは以下の通り。
    * `c[ooler] <温度>`  
      指定温度で冷房の電源を入れる。例: `@iot_bot c 28`
    * `h[eater] <温度>`  
      指定温度で暖房の電源を入れる。例: `@iot_bot heat 20`
    * `d[ry]`  
      ドライ運転で電源を入れる。
    * `f[an]`  
      ファン運転で電源を入れる。
    * `a[uto]`  
      自動運転で電源を入れる。
    * `off`  
      エアコンの電源を切る。
    * `hello`  
      このシステムが動いているかどうかを確認する。
    * `help`  
      このシステムの使い方を表示する。


(あとで書く)

ソースコード全体は [`sketch_slack_lcd_aht25.ino`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/sketch_slack_lcd_aht25.ino) から取得できます。
