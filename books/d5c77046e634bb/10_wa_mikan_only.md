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
[`AT`](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/Basic_AT_Commands.html#cmd-at) [Enter] とだけ打てば、"OK" が返ってきます。[`AT+GMR`](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/Basic_AT_Commands.html#cmd-gmr) [Enter] と打てば、ファームウェアのバージョンが返ってきます。

```
AT

OK
```

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

GR-CITRUSと接続して、今まで通り `WiFi` クラスが動作すれば成功です。


## Arduino IDEを使ってみる

まずは[Arduino IDE](https://www.arduino.cc/en/software)をダウンロードしましょう。現時点の最新版は2.1.1です。(`arduino-ide_2.1.1_Windows_64bit.exe`)


参考: [Arduino IDEでWA-MIKAN(和みかん)のESP8266をプログラミングする　環境インストール編 - Qiita](https://qiita.com/tarosay/items/28ba9e0208f41cec492d)


## IRremoteESP8266を使って赤外線リモコンの送受信を行う
   [赤外線リモコン受信モジュールOSRB38C9AA](https://akizukidenshi.com/catalog/g/gI-04659/)
   [赤外線LED OSI5LA5113A](https://akizukidenshi.com/catalog/g/gI-12612/)
   If 100mA, Vf 1.35V, Pulse 1000mA
   If 190mA, Vf 2.3V
   [2SC2001](https://akizukidenshi.com/catalog/g/gI-13828/)
   [赤外線リモコンの通信フォーマット](http://elm-chan.org/docs/ir_format.html)
   [ラズパイで外部からエアコンの電源を入れてみる その1](https://bsblog.casareal.co.jp/archives/5010)

## Wi-Fiを使ってみる (予定)
