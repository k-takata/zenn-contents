---
title: "WA-MIKANを単体で使う"
---

ここでは、WA-MIKANを単体で使う方法を紹介します。

WA-MIKANにはESP8266というWi-Fiモジュールが搭載されていますが、このモジュール自体がマイクロコントローラーを内蔵しており、単体でプログラムを動かすこともできます。[Arduino IDE](https://www.arduino.cc/en/software)を使えば、簡単にESP8266用のプログラムを作成し、WA-MIKANで動かすことができます。


## ハードウェアの準備

WA-MIKANにプログラムを書き込むには、USBシリアル経由で行います。

WA-MIKANの基板の端にはUSBシリアル接続用のランドがありますので、ここに6ピンのL型ピンヘッダーを半田付けします。

USBシリアル変換モジュールは3.3V対応のものが必要です。今回はSparkFunの[DEV-15096](https://www.sparkfun.com/products/15096)を[千石電商で購入](https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-5EFU)しました。(5Vのモジュールを使う場合は、[WA-MIKANボードの改造が必要](https://qiita.com/tarosay/items/0744055468cd0248def3)です。)

なお、WA-MIKANとDEV-15096の間の接続は、TxD (送信) → RXI (受信), RxD (受信) → TXO (送信) のようになりますので、そのまま差し込むことができます。ピンの順序が違うのではと焦る必要はありません。


## ファームウェアの戻し方

Arduino IDEを使ってWA-MIKANをプログラミングすると、GR-CITRUSからWA-MIKANを使うことができなくなってしまいます。そこで、WA-MIKANのプログラミングを始める前に、ファームウェアの戻し方を確認しておきましょう。

基本的には、ボードの作者のtarosay氏による[WA-MIKAN(和みかん)をGR-CITRUSから使える状態に戻す方法 - Qiita](https://qiita.com/tarosay/items/618104528d4e5026fa8f)に従えばよいですが、ツールのバージョンなどが更新されていますのでいくつか差分があります。

ファームウェアを書き込む前に、現在のファームウェアのバージョンを確認しておきます。
確認にはTera Term等のシリアル端末ソフトを使いますが、この際、設定は以下のようにしておきます。送信する改行コードを CR+LF にしておくのがポイントです。

| 項目 | 設定値 |
|------|--------|
| スピード | 115200 |
| データ | 8 bit |
| パリティ | none |
| ストップビット | 1 bit |
| フロー制御 | none |
| 改行コード (受信) | CR |
| 改行コード (送信) | **CR+LF** |

ここで[ATコマンド](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/index.html)を入力すると、ESP8266からの応答が返ってきます。
[`AT`](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/Basic_AT_Commands.html#cmd-at) [Enter] とだけ打てば、"OK" が返ってきます。

```
AT

OK
```

[`AT+GMR`](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/Basic_AT_Commands.html#cmd-gmr) [Enter] と打てば、ファームウェアのバージョンが返ってきます。

```
AT+GMR
AT version:0.52.0.0(Jan  7 2016 18:44:24)
SDK version:1.5.1(e67da894)
compile time:Jan  7 2016 19:03:11
OK
```

ファームウェアのバージョンが確認できたところで、ファームウェアの更新作業に入っていきます。
まずは、[Flash Download Tools](https://www.espressif.com/en/support/download/other-tools?keys=&field_type_tid%5B%5D=14)から最新のFlash Download Toolsをダウンロードします。2023年8月時点の最新版はV3.9.5です。

次に、[ESP8266 NONOS SDK](https://github.com/espressif/ESP8266_NONOS_SDK/releases)からファームウェアをダウンロードします。私が試した範囲では、v2.0.0とv2.2.1は動作しましたが、v3.0以降は動作しませんでした。
各バージョンの説明欄の下部にある "Assets" をクリックし、"Source code (zip)" をクリックしてダウンロードします。

Flash Download Toolsを起動して書き込みます。
ダウンロードするファイルは、ファイル名の左のチェックボックスにチェックを入れておく必要があります。
(あとで書く)


ファームウェアの書き込みが終わったら、正しく書き込めたか確認しましょう。

JP1のショートを外してリセットボタンを押して、Tera Termと接続します。
まずは先ほどと同じように、`AT+GMR` コマンドでバージョンを使ってバージョンを確認します。

```
AT+GMR
AT version:1.6.2.0(Apr 13 2018 11:10:59)
SDK version:2.2.1(6ab97e9)
compile time:Jun  7 2018 19:34:29
Bin version(Wroom 02):1.6.2
OK
```

次に [`AT+RESTORE`](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/Basic_AT_Commands.html#cmd-restore) コマンドを使って、設定を出荷時設定に戻しておきます。

```
AT+RESTORE

OK

 ets Jan  8 2013,rst cause:2, boot mode:(3,6)

load 0x40100000, len 2408, room 16
tail 8
chksum 0xe5
load 0x3ffe8000, len 776, room 0
tail 8
chksum 0x84
load 0x3ffe8310, len 632, room 0
tail 8
chksum 0xd8
csum 0xd8

2nd boot version : 1.6
  SPI Speed      : 40MHz
  SPI Mode       : QIO
  SPI Flash Size & Map: 32Mbit(512KB+512KB)
jump to run user1 @ 1000
```

GR-CITRUSと接続して、今まで通り `WiFi` クラスが動作すれば成功です。(Tera TermをGR-CITRUSと接続する際は、送信する改行コードの設定を CR に戻しておく必要があります。)


## Arduino IDEを使ってみる

まずは[Arduino IDE](https://www.arduino.cc/en/software)をダウンロードしましょう。現時点の最新版は2.1.1です。(`arduino-ide_2.1.1_Windows_64bit.exe`)


[ESP8266用コアライブラリ](https://github.com/esp8266/Arduino)をインストールします。
ドキュメントに記載されている通り、ボードマネージャの追加URLとして、以下のURLを指定します。(ESP8266だけでなく、複数のボードを追加したい場合は、URLをコンマで区切って指定します。)

```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

これでボードマネージャでESP8266用の環境が選択できるようになるので、それを選択してインストールします。


参考: [Arduino IDEでWA-MIKAN(和みかん)のESP8266をプログラミングする　環境インストール編 - Qiita](https://qiita.com/tarosay/items/28ba9e0208f41cec492d)
この記事が書かれた当時とは異なり、手動でESP8266の環境をダウンロードしたりPython 2.7をインストールする必要はありません。


## IRremoteESP8266を使って赤外線リモコンの送受信を行う

[IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)という赤外線リモコン送受信用のライブラリがあります。これを使うと、各種の家電を自分のプログラムで制御することが簡単にできるようになります。

### 作成

赤外線リモコン送受信器を作成するために以下の部品を用意します。

| 部品 | 型番 | 説明 |
|------|------|------|
|赤外線リモコン受信モジュール|[OSRB38C9AA](https://akizukidenshi.com/catalog/g/gI-04659/)||
|赤外線LED|[OSI5LA5113A](https://akizukidenshi.com/catalog/g/gI-12612/)||
|NPNトランジスター|[2SC2001](https://akizukidenshi.com/catalog/g/gI-13828/)|Ic が 500mA ~ 1A 程度流せるもの|
|抵抗|5.1Ω||
|抵抗|680Ω||
|抵抗|4.7kΩ||

今回使った赤外線LEDは、定常動作でIf 100mA、パルス動作でIf 1000mAとなっています。赤外線リモコンの送信に使う場合はパルス動作で使うことになりますので、それを前提にLEDに流す電流を決めることにします。

とりあえず、Ifとして200mAを流すことを考えてみます。
データシートのIF-VFグラフを見ると、If=200mAのときのVfは約2.4Vです。この時の電流制限抵抗は、

$$
(3.3 [V] - 2.4 [V]) / 200 [mA] = 4.5 [Ω]
$$

と計算できます。
ここで、たまたま手元に5.1Ω抵抗があったので、これを代わりに使った場合のIfを見積もってみます。
Vf=2.0Vと仮定すると抵抗に流れる電流は、

$$
(3.3 - 2.0) / 5.1 = 255 [mA]
$$

Vf=2.5Vと仮定すると抵抗に流れる電流は、

$$
(3.3 - 2.5) / 5.1 = 157 [mA]
$$

ここで、データシートのIF-VFグラフ上に (2.0V, 255mA) - (2.5V, 157mA) の線を引くと、Vf = 2.3V, If = 190mA の辺りが交点となります。したがって、5.1Ωを使うとIf 190mAになると見積もることができます。


[赤外線リモコンの通信フォーマット](http://elm-chan.org/docs/ir_format.html)
[ラズパイで外部からエアコンの電源を入れてみる その1](https://bsblog.casareal.co.jp/archives/5010)

[赤外線リモコンを自作する - その1データ解析編 - Qiita](https://qiita.com/ayakix/items/3ad454f135fb63a92026)


### 赤外線受信


### 赤外線送信



## Wi-Fiを使ってみる


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

`setup()` 関数内の `WiFi.setOutputPower()` でWi-Fiの出力強度を設定しています。0.0 ~ 20.5 dBm の範囲で強度を設定できます。この設定は必須ではありませんが、通信に問題がない範囲で小さい値を指定することでESP8266の消費電力を下げることができます。
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

HTTPS接続

ESP8266でHTTPS接続を行う方法はいくつかあるようですが、今回は[BearSSL WiFi Classes](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html)を使ってみることにします。



curlコマンドでメッセージを送信する例



### Slackからメッセージを受信する

JSON解析
