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

(あとで書く)

slackにアプリが正しく登録できたかを確かめるために、curlコマンドを使ってメッセージを送信してみましょう。

```shell-session
$ curl -X POST 'https://slack.com/api/chat.postMessage' -d 'token=xoxb-*************-*************-************************' -d 'channel=C**********' -d 'text=Hello+World'
```


#### ESP8266からメッセージを送信する

(あとで書く)


### Slackからメッセージを受信する

SlackからのデータはJSON形式で返ってきます。
JSONを解析するために、ArduinoJSONというライブラリを使います。



(あとで書く)
