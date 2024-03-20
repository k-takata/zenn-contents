---
title: "ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その2)"
emoji: "🎚️"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32", "esp32c3", "bme680", "pcbway"]
published: true
---

## 概要

本記事は、以下の記事の続きとなります。

* [ESP32-C3とBME680でIoT環境メーターを作る](https://zenn.dev/k_takata/articles/esp32c3-envmeter)
* [ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その1)](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)

今回は、その1で発注した基板の受領、組み立て、ソフトウェアの作成について記載します。


## 基板到着

元々PCBWayでの基板の製造が3 ~ 4日、OCSでの送付に2 ~ 3日の予定でしたが、実際には3/4に製造開始して、3日後の3/7に完了、同日中にOCSに引き渡され、4日後の3/11に一度配達されたものの自分が不在だったために、3/12に再配達となりました。結局、再配達を除けばちょうど1週間で基板を受け取ることができました。

見慣れたJLCPCBのネコポス用の箱に比べると、かなり大きな箱で届きました。

https://twitter.com/k_takata/status/1767352744569111038

左が今回PCBWayで作ったRev. 2、右が前回JLCPCBで作ったRev. 1です。写真では見にくいですが、今回のRev. 2はRev. 1に比べると明らかにつやがあります。
PCBWayでは、黒と黒(つや消し)が明確に分かれているだけあって、つや消しの黒がよい場合は明示的につや消しを選ぶ必要があります。（価格は上がりますが。）

なお、今回注文したのは10枚でしたが、実際に届いたものを数えると11枚ありました。何らかの理由で1枚余計に作ってしまった方が都合がよかったのでしょうか。よく分かりませんが、ちょっと得した気分です。


### 受領確認

PCBWayの注文リストを見ると、基板が到着したのに配達状態のままになっており、完了状態に移行しないのを不思議に思っていたのですが、よく見ると「受け確認」と書かれたボタンがあることに気づきました。ボタンを押すと「受け確認?」とだけ聞かれて意味が分かりませんでしたが、どうやらこれが受領確認のボタンだったようです。（英語ページでは "Confirm received" でした。）
ボタンは「受領確認」、質問メッセージは「製品を受け取りましたか？」だったら分かりやすいと思いました。日本語訳の改善に期待したいです。
このボタンを押すと、無事完了状態になりました。

![confirm received](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/confirm-received.png)

さらに、受領確認を行ったことで、[PCBWayリワード画面](https://member.pcbway.jp/specials/rewards)でポイントとBeansが加算されていることが確認できました。
1回の注文ごとに10ポイント、さらに$1の支払いにつき1ポイントで、合計28ポイント加算されていました。


### フィードバックを残す

「受け確認」ボタンの下にあった「フィードバックを残す」ボタンを押すと、以下のような画面になりました。

![feedback](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/feedback.png)

（英語で）50語以上のフィードバックを送るとクーポンをもらえるチャンスがあるようです。気が向いたらフィードバックを送ってみたいと思います。


## 組み立て

部品表は[PCB_envmeter_esp32c3](https://github.com/k-takata/PCB_envmeter_esp32c3)に記載の通りです。

はんだ付けの一番難しい部品は[USB Type-Cコネクター](https://akizukidenshi.com/catalog/g/g114356/)（レセプタクル）かもしれませんが、慣れると意外と簡単にはんだ付けできます。
コネクターに付いた2か所の突起のおかげで、位置合わせは非常に簡単です。フラックスを塗布した後、はんだこての先にごく少量のはんだを乗せ、ピンとランドに染み込ませるようにするときれいに付きます。

コネクターの導通確認には、[USBtype-CコネクタDIP化キット(全ピン版)](https://akizukidenshi.com/catalog/g/g113471/)があると便利です。両端Type-Cのケーブルを使ってこれと接続し、対応しているピン同士がつながっているか、余計なところがブリッジしていないかを確認していきます。CC1, CC2ピンは、ケーブルの挿す向き(表裏)によって接続が変わります。

| 向き    | 導通      |
|---------|-----------|
| 表 - 表 | CC1 - CC1 |
| 表 - 裏 | CC1 - CC2 |
| 裏 - 表 | CC2 - CC1 |
| 裏 - 裏 | CC2 - CC2 |

コネクターの信号ピンのはんだ付けの後は、コネクターのシールドのはんだ付けですが、はんだの量が少なかったり、加熱時間が短いと正しくはんだ付けできていないことがありますので、正しくはんだ付けできたか確認が必要です。

あとは基本的に、背の低い部品から付けていきます。
例: ESP32-C3、その他の表面実装部品、抵抗、コンデンサー、ピンソケット、LED、タクトスイッチの順

J4は、OLEDが斜めにならないように気を付けて取り付ける必要があります。(Rev. 1に比べて穴のサイズを小さくしたため、斜めになりにくくなりましたが、それでも注意が必要です。)

赤外線受信機能を使わない場合は、U4を取り付ける必要はありません。

赤外線送信機能を使わない場合は、D2, Q1, R7, R8, R9を取り付ける必要はありません。


## 基本動作確認

### IoT環境メーター機能

IoT環境メーター機能は[Rev. 1](https://zenn.dev/k_takata/articles/esp32c3-envmeter)から変更ありませんので、[Rev. 1のソフトウェア](https://github.com/k-takata/zenn-contents/tree/master/articles/files/esp32c3-envmeter)を書き込めばまったく同じように動作します。


### 赤外線受信機能

[WA-MIKANの時](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/10_wa_mikan_only1)と同様に、[IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)のサンプルの1つである[IRrecvDumpV3](https://github.com/crankyoldgit/IRremoteESP8266/tree/master/examples/IRrecvDumpV3)がほぼそのまま使えます。
Arduino IDEのメニューから以下を選択します。

ファイル > スケッチ例 > IRremoteESP8266 > IRrecvDumpV3

赤外線受信モジュールからの入力はGPIO10に接続していますので、`kRecvPin`の設定はそのまま使えます。
`setup()`関数の先頭部分の`Serial.begin()`の呼び出しは以下のようになっています。

```C
#if defined(ESP8266)
  Serial.begin(kBaudRate, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(kBaudRate, SERIAL_8N1);
#endif  // ESP8266
```

ESP32-C3の内蔵USBシリアルを使用する場合、このままではコンパイルエラーになってしまいますので、以下のように修正します。

```C
#if defined(ESP8266)
  Serial.begin(kBaudRate, SERIAL_8N1, SERIAL_TX_ONLY);
#elif ARDUINO_USB_CDC_ON_BOOT
  Serial.begin();
#else  // ESP8266
  Serial.begin(kBaudRate, SERIAL_8N1);
#endif  // ESP8266
```

ESP32-C3の内蔵USBシリアルを使用する場合、`ARDUINO_USB_CDC_ON_BOOT`が定義され、`Serial.begin()`の実体は`USBSerial.begin()`となります。この関数は0個または1個の引数しか取りません。修正前のコードでは引数が2個になっているためコンパイルエラーになってしまっていました。第1引数はボーレートですが、指定しても無視されるため、指定してもしなくてもどちらでも問題ありません。

なお、この変更は以下のPRを作成して取り込まれましたので、IRremoteESP8266の次のリリースからは自分で修正する必要はなくなる見込みです。

https://github.com/crankyoldgit/IRremoteESP8266/pull/2080


### 赤外線送信機能

WA-MIKANの時の[赤外線送信](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/10_wa_mikan_only1#%E8%B5%A4%E5%A4%96%E7%B7%9A%E9%80%81%E4%BF%A1)の部分で説明したコードがほぼそのまま使えます。
ただし、赤外線送信のピンにはGPIO6を使用していますので、ピン番号の定義だけは12から6に変更しておく必要があります。


### HTTPS接続、Slackメッセージ送受信

[WA-MIKANを単体で使う（その2）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/11_wa_mikan_only2)と同様のコードでHTTPS接続ができます。
ただ、ESP8266ではSSL/TLSライブラリーとして[BearSSL](https://bearssl.org/)が使われていましたが、ESP32では[Mbed TLS](https://github.com/Mbed-TLS/mbedtls)という別のライブラリーが使われていることもあり、少し使い方が異なります。

* ヘッダーファイル  
  ファイル名が少し異なります。
  - ESP8266: `ESP8266WiFi.h`, `ESP8266HTTPClient.h`
  - ESP32: `WiFi.h`, `HTTPClient.h`
* ルートCA設定方法  
  使う関数名と、引数が少し異なります。`cert_Root_xxxx`はpem形式のルート証明書で、[`tools/cert.py`](https://github.com/esp8266/Arduino/tree/master/tools)で取得できます。（あるいは、Webブラウザーから手動で取得する方法もあります。）
  - ESP8266:
    ```C
    X509List cert(cert_Root_xxxx);
    client.setTrustAnchors(&cert);
    ```
  - ESP32:
    ```C
    client.setCACert(cert_Root_xxxx);
    ```

具体的な使用方法はESP32の[WiFiClientSecure](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure)のREADMEや、ESP32のサンプルの1つである[BasicHttpsClient.ino](https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino)が参考になります。

以上の変更を行えば、Slackのメッセージ送受信も同じ方法で動作します。


## 統合動作

IoT環境メーター機能と[WA-MIKANを単体で使う（その2）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/11_wa_mikan_only2)で作成したSlack経由のエアコン制御機能を統合し、1つのソフトウェアで動作させたいと思います。
（赤外線受信機能の使用頻度は低いので、この機能は統合しません。）

前述の修正を加えた後は、ソースコードをほぼそのまま統合した後、いくつか細かい変更を加えています。

* 半自動モードの追加  
  `@iot_bot 20`などのように温度を指定するだけで、現在の気温から冷房・暖房を判断して電源を入れる機能を追加しました。
* 設定のLCD表示を削除  
  最後にエアコンに送信したコマンドをLCDに表示するようにしていましたが、OLEDの表示スペースの関係から表示は取りやめました。（modeボタンを押すことで、設定を表示するようにしてもよいかもしれませんが。）
* ディープスリープモードの削除  
  IoT環境メーター機能は常に動作しているため、ディープスリープモードを使うのはやめました。

今回作成したソースコードは以下に格納しています。
https://github.com/k-takata/zenn-contents/tree/master/articles/files/esp32c3-envmeter-rev2


## 完成品

https://twitter.com/k_takata/status/1769706615425409257


## まとめ

今回は、IoT環境メーター/スマートリモコン Rev. 2について、PCBWayに発注した基板の受け取りから、組み立て、ESP8266向けソフトウェアからの変更点、全体を統合したソフトウェアの作成までをまとめました。

今までは、IoT環境メーターとスマートリモコン機能を動かすのに2個のボードが必要でしたが、これでボードが1個で済むようになりました。
今後は余ったボードの活用方法なども考えていきたいです。
https://twitter.com/k_takata/status/1769707334618513517
