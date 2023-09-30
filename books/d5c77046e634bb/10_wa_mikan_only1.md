---
title: "WA-MIKANを単体で使う（その1）"
---

ここでは、WA-MIKANを単体で使う方法を紹介します。

WA-MIKANにはESP8266というWi-Fiモジュールが搭載されていますが、このモジュール自体がマイクロコントローラーを内蔵しており、単体でプログラムを動かすこともできます。[Arduino IDE](https://www.arduino.cc/en/software)を使えば、簡単にESP8266用のプログラムを作成し、WA-MIKANで動かすことができます。

この章と次の章では、WA-MIKANを使って以下のようなものを作成していきます。

* Hello World
* 赤外線リモコン送受信
* I²Cを使ってLCD表示
* NTPで時刻合わせ
* Slackメッセージ送受信
* Slackを介してエアコンを制御

最終的には、「[ラズパイで外部からエアコンの電源を入れてみる その1](https://bsblog.casareal.co.jp/archives/5010)」などを参考に、Slack経由で外出先から自宅のエアコンを操作できるシステムを作ります。

この章では、LCD表示までを行います。


## ハードウェアの準備

WA-MIKANにプログラムを書き込むには、USBシリアル経由で行います。

WA-MIKANの基板の端にはUSBシリアル接続用のランドがありますので、ここに6ピンのL型ピンヘッダーを半田付けします。

USBシリアル変換モジュールは3.3V対応のものが必要です。今回はSparkFunの[DEV-15096](https://www.sparkfun.com/products/15096)を[千石電商で購入](https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-5EFU)しました。(5Vのモジュールを使う場合は、[WA-MIKANボードの改造が必要](https://qiita.com/tarosay/items/0744055468cd0248def3)です。)

なお、WA-MIKANとDEV-15096の間の接続は、TxD (送信) → RXI (受信), RxD (受信) → TXO (送信) のようになりますので、そのまま差し込むことができます。ピンの順序が違うのではと焦る必要はありません。


## ファームウェアの戻し方

Arduino IDEを使ってWA-MIKANをプログラミングすると、GR-CITRUSからWA-MIKANを使うことができなくなってしまいます。そこで、WA-MIKANのプログラミングを始める前に、ファームウェアの戻し方を確認しておきましょう。

基本的には、ボードの作者のtarosay氏による「[WA-MIKAN(和みかん)をGR-CITRUSから使える状態に戻す方法 - Qiita](https://qiita.com/tarosay/items/618104528d4e5026fa8f)」に従えばよいですが、ツールのバージョンなどが更新されていますのでいくつか差分があります。

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

### インストール

まずは[Arduino IDE](https://www.arduino.cc/en/software)をダウンロードしましょう。現時点の最新版は2.2.1です。(`arduino-ide_2.2.1_Windows_64bit.exe`)


[ESP8266用コアライブラリ](https://github.com/esp8266/Arduino)をインストールします。
ドキュメントに記載されている通り、ボードマネージャの追加URLとして、以下のURLを指定します。(ESP8266だけでなく、複数のボードを追加したい場合は、URLをコンマで区切って指定します。)

```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

これでボードマネージャでESP8266用の環境が選択できるようになるので、それを選択してインストールします。

**参考:** [Arduino IDEでWA-MIKAN(和みかん)のESP8266をプログラミングする　環境インストール編 - Qiita](https://qiita.com/tarosay/items/28ba9e0208f41cec492d) （この記事が書かれた当時とは異なり、手動でESP8266の環境をダウンロードしたりPython 2.7をインストールする必要はありません。）


### Hello World

動作確認のために、シリアル通信で文字を表示するプログラムを動かしてみましょう。

```CPP
void setup() {
  Serial.begin(115200);
  delay(10);
}

void loop() {
  Serial.println("Hello World.");
  delay(1000);
}
```

Arduino IDEからスケッチの書き込みを行い、シリアルモニタを開くと以下の文字列が1秒ごとに表示されます。

```
Hello World.
```

**Tips:** WA-MIKANのリセットボタンを押すと、シリアルに何やらゴミが表示されますが、これはシリアル端末の通信速度を [74880 bps](https://arduino-esp8266.readthedocs.io/en/3.0.2/ideoptions.html#crystal-frequency) (=115200 * 26 / 40) にすると正しく表示されます。


## IRremoteESP8266を使って赤外線リモコンの送受信を行う

[IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)という赤外線リモコン送受信用のライブラリがあります。これを使うと、各種の家電を自分のプログラムで制御することが簡単にできるようになります。


### 作成

#### 部品表

赤外線リモコン送受信器を作成するために以下の部品を用意します。

| 部品 | 型番 | 説明 |
|------|------|------|
|赤外線リモコン受信モジュール|[OSRB38C9AA](https://akizukidenshi.com/catalog/g/gI-04659/)||
|赤外線LED|[OSI5LA5113A](https://akizukidenshi.com/catalog/g/gI-12612/)||
|NPNトランジスター|[2SC2001](https://akizukidenshi.com/catalog/g/gI-13828/)|Ic が 500mA ~ 1A 程度流せるもの|
|抵抗|5.1Ω|LED電流制限抵抗(R3)|
|抵抗|680Ω|ベース抵抗(R1)|
|抵抗|4.7kΩ|ベース・エミッター間抵抗(R2)|


#### 回路図および接続

以下に今回の回路図を示します。

[![ac-controller-schema](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/ac-controller-schema.png)](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/ac-controller-schema.png)

ESP8266のIO14ピンはWA-MIKANのTP1に接続されています。ここに赤外線リモコンの受信回路を接続します。回路は簡単で、赤外線リモコン受信モジュールに3.3V電源とGNDをつなぎ、出力をTP1につなぐだけです。

ESP8266のIO12ピンはWA-MIKANのTP2に接続されています。ここに赤外線リモコンの送信回路を接続します。


#### 送信部分の回路設計

今回使った赤外線LEDは、定常動作でIf 100mA、パルス動作でIf 1000mAとなっています。赤外線リモコンの送信に使う場合はパルス動作で使うことになりますので、それを前提にLEDに流す電流を決めることにします。

とりあえず、Ifとして200mAを流すことを考えてみます。
[データシート](https://akizukidenshi.com/download/ds/optosupply/OSI5LA5113A.pdf)のIF-VFグラフを見ると、If=200mAのときのVfは約2.4Vです。

[![IF-VF](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/if-vf.png)](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/if-vf.png)

この時の電流制限抵抗は、

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

ここで、データシートのIF-VFグラフ上に (2.0V, 255mA) - (2.5V, 157mA) の線を引くと、Vf = 2.3V, If = 190mA の辺りが交点となります。

[![IF-VF / 5.1Ω](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/if-vf2.png)](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/if-vf2.png)

したがって、5.1Ωを使うとIf 190mAになると見積もることができます。予定の200mAよりは少し小さい値になりますが、特に問題はないでしょう。

実際には、2SC2001のコレクタ・エミッタ間飽和電圧Vce(sat)があるため、もう少し電流は下がります。

トランジスターのベース抵抗とベース・エミッター間抵抗の計算は、「[デジタルトランジスタ（デジトラ）の使い方と抵抗値選定方法 | アナデジ太郎の回路設計](https://ana-dig.com/digi-tra/)」が参考になります。今回は、このページを参考にしつつ、実験で抵抗値を決めました。

4.7kΩのベース・エミッター間抵抗(R2)は、入力電圧が低いときにLEDが点かないようにするためのものです。実際、この抵抗がない場合、ESP8266が書き込みモードに入っているときなどにLEDがうっすら点いているのが（デジカメなどで）確認できます。今回はLEDが勝手に点灯しない範囲でできるだけ大きな抵抗値になるよう実際に試した結果、4.7kΩとなりました。

ベース抵抗(R1)については、ESP8266のGPIOが出力できる電流は12mAとのことなので、それよりも電流が小さくなるように調整する必要があります。今回は余裕を見て4mA程度に抑えることにするため、680Ωを使いました。

$$
I_{i} = (V_{i} - V_{BE}) / R1 = (3.3 [V] - 0.6 [V]) / 680 [Ω] = 3.97 [mA]
$$

$$
I_{BE} = V_{BE} / R2 = 0.6 [V] / 4.7 [kΩ] = 0.13 [mA]
$$

つまり、入力電流 $I_{i}$ が 0.13mA 以上にならなければトランジスタはオンにはなりません。つまりトランジスタがオンになる入力電圧は、

$$
0.6 [V] + 0.13 [mA] \times 680 [Ω] = 0.69 [V]
$$

ということになります。
このとき、ベース電流 $I_{B}$ は、

$$
I_{B} = I_{i} - I_{BE} = 3.84 [mA]
$$

となり、このとき $I_{C} / I_{B}$ は次のようになります。

$$
I_{C} / I_{B} = 190 [mA] / 3.84 [mA] = 49.5
$$

上記ページでは $h_{FE}$ を20程度に抑える必要があると記載されていますが、今回は49.5となってしまいました。この場合、コレクタ・エミッタ間飽和電圧が想定よりも高くなってしまうことが考えられますが、実際に試した範囲では特に問題なさそうなため、今回はこの値を使います。


### 赤外線受信

まずは、自分が制御したい家電のリモコンにどのようなプロトコルが使われているかを調べます。

「[赤外線リモコンを自作する - その1データ解析編 - Qiita](https://qiita.com/ayakix/items/3ad454f135fb63a92026)」が参考になります。

IRremoteESP8266のサンプルの1つであるIRrecvDumpV3 (あるいはIRrecvDumpV2)がそのまま使えます。
Arduino IDEのメニューから以下を選択します。

ファイル > スケッチ例 > IRremoteESP8266 > IRrecvDumpV3

スケッチを実行し、赤外線受光器に向けてリモコンのボタンを押すと、以下のようなログが出ます。

ダイキンのエアコンの例:
```
Protocol  : DAIKIN
Code      : 0x11DA2700C500401711DA27004200005411DA270000393800A0000000000000C00000E3 (280 Bits)
Mesg Desc.: Power: On, Mode: 3 (Cool), Temp: 28C, Fan: 10 (Auto), Powerful: Off, Quiet: Off, Sensor: Off, Mould: Off, Comfort: Off, Swing(H): Off, Swing(V): Off, Clock: 00:00, Day: 0 (UNKNOWN), On Timer: Off, Off Timer: Off, Weekly Timer: On
uint16_t rawData[583] = {422, 472,  396, 472, ..., 1316,  420};  // DAIKIN
uint8_t state[35] = {0x11, 0xDA, ..., 0x00, 0xE3};
```

DAIKIN 280ビットプロトコルを使用していることが分かります。

THREEUPのサーキュレーターの例:
```
Protocol  : SYMPHONY
Code      : 0xD84 (12 Bits)
uint16_t rawData[119] = {1318, 376,  1314, 378, ..., 1214,  446};  // SYMPHONY D84
uint64_t data = 0xD84;
```

SYMPHONY 12ビットプロトコルを使用していることが分かります。

`rawData` は赤外線リモコンのパルスが出ている期間（マーク）と出ていない期間（スペース）をμ秒単位で表現した生データです。（よく使われる赤外線リモコンのプロトコルについては、「[赤外線リモコンの通信フォーマット](http://elm-chan.org/docs/ir_format.html)」を参照してください。）
生データは[アナログ的なデータ表現](https://github.com/crankyoldgit/IRremoteESP8266/wiki/Frequently-Asked-Questions#why-is-the-raw-data-for-a-button-or-ac-state-always-different-for-each-capture)であるため、同じリモコン操作をしても `rawData` は毎回変わってきます。

生データを1段階デコードしたものが **state[]**/**integer** フォーマットです。数ビットから数百ビットのデータになります。上記のログであれば、"Code" の行や `uint8_t state[35]`, `uint64_t data` として表示されている情報がそれに当たります。

さらに、ライブラリが対応していれば **state[]**/**integer** フォーマットをデコードして、リモコンデータの意味を表示してくれます。例えばエアコンであれば、動作モード、温度、風量などのレベルの情報が取得できます。上記のログであれば、"Mesg Desc." の行に表示されている情報がそれに当たります。


### 赤外線送信

次に、赤外線送信を行う方法をいくつかのケースに分けて説明します。


#### 生データの送信

`sendRaw()` 関数を使うと、受信した生データをそのまま送信することができます。

今回、ESP8266の12ピンを出力に使用していますので、`IRsend` クラスのコンストラクタにはこのピン番号を指定します。

IRrecvDumpV3 でダンプしたログをそのままペーストし、`rawData` の要素数と、赤外線リモコンの変調周波数 (kHz単位) を指定して `sendRaw()` を呼び出すことで、送信を行います。

```CPP
#include <IRremoteESP8266.h>
#include <IRsend.h>

#define KHZ 38
#define PIN_SEND 12
IRsend irsend(PIN_SEND);

// Protocol  : DAIKIN
// Code      : 0x11DA2700C500401711DA27004200005411DA270000393800A0000000000000C00000E3 (280 Bits)
// Mesg Desc.: Power: On, Mode: 3 (Cool), Temp: 28C, Fan: 10 (Auto), Powerful: Off, Quiet: Off, Sensor: Off, Mould: Off, Comfort: Off, Swing(H): Off, Swing(V): Off, Clock: 00:00, Day: 0 (UNKNOWN), On Timer: Off, Off Timer: Off, Weekly Timer: On
uint16_t rawData[583] = {426, 442, ...};

void setup() {
  irsend.begin();
}

void loop() {
  irsend.sendRaw(rawData, sizeof(rawData) / sizeof(rawData[0]), KHZ);
  delay(10 * 1000);
}
```

#### 汎用の送信関数を使う場合

`IRsend` クラスの `send()` 関数を使えば、簡単に **state[]**/**integer** フォーマットのデータを送信できます。

**integer** フォーマットのデータを送信するには以下の関数を使います。第1引数でプロトコル、第2引数で送信すべきデータ (最大64ビット)、第3引数で送信すべきビット数を指定します。リピート送信する場合は第4引数を使います。

```CPP
  bool send(const decode_type_t type, const uint64_t data,
            const uint16_t nbits, const uint16_t repeat = kNoRepeat);
```

**state[]** フォーマットのデータを送信するには以下の関数を使います。第1引数でプロトコル、第2引数で送信すべきデータ、第3引数で送信すべきバイト数を指定します。

```CPP
  bool send(const decode_type_t type, const uint8_t *state,
            const uint16_t nbytes);
```

THREEUPのサーキュレーターのコードを送信する場合は以下のようになります。

```CPP
#include <IRsend.h>

#define PIN_SEND 12
IRsend irsend(PIN_SEND);

void setup() {
  irsend.begin();
}

void loop() {
  uint64_t code = 0xD84;
  irsend.send(SYMPHONY, code, 12);  // 12 bits
  delay(10 * 1000);
}
```

これを実行すると、10秒ごとに電源をon/offするようになります。


#### エアコン制御の例1

`IRac.h` をインクルードし `IRac` クラスを使うと、各社のエアコンを統一的なインターフェイスで制御することができます。

```CPP
#include <IRac.h>

#define PIN_SEND  12

void setup () {
}

void loop() {
  IRac irsend(PIN_SEND);

  irsend.next.protocol = decode_type_t::DAIKIN;
  irsend.next.power = true;
  irsend.next.mode = stdAc::opmode_t::kCool;
  irsend.next.fanspeed = stdAc::fanspeed_t::kAuto;
  irsend.next.degrees = 28.0;

  irsend.sendAc();

  delay(10 * 1000);
}
```

`IRac` クラスの `next` メンバー変数で次に送信すべき状態を保持しています。`sendAc()` を呼び出すことで、`next` の状態が送信されます。

`next.protocol` でエアコンのプロトコルを指定します。ここではダイキン280bitプロトコルを指定しています。
`next.power` で電源のon/offを指定します。
`next.mode` でエアコンのモード（暖房冷房等）を指定します。
`next.fanspeed` でファンのスピードを指定します。
`next.degrees` で温度を指定します。

それ以外の設定項目は [`IRsend.h`](https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.h) の `state_t` 構造体で確認できます。

ちなみに、コンパイル時に表示されるメモリー使用量は以下のようになっています。

```
. Variables and constants in RAM (global, static), used 29768 / 80192 bytes (37%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1496     initialized variables
╠══ RODATA   2528     constants       
╚══ BSS      25744    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 60015 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27247    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 326772 / 1048576 bytes (31%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     326772   code in flash   
```

`IRac` クラスを使った場合のコード使用量 (Code in flash) はおよそ330kBです。


#### エアコン制御の例2

次に `IRac` クラスの代わりに、エアコンメーカーごとのクラスを使う場合を見てみます。`IRac` クラスに比べて、より細かな制御ができる場合があります。また、他社向けのコードをリンクせずにすむため、プログラムサイズが小さくなります。

ダイキンの280bitプロトコルを使う場合は `IRDaikinESP` クラスを使用します。

ダイキンの280bitプロトコルは0.5℃単位での温度設定が可能ですが、現在 `IRDaikinESP` クラスが提供している `setTemp()` 関数は1℃単位での温度設定しかできません。
`getRaw()` 関数で送信データを取得し、それを直接書き換えてから `setRaw()` 関数で設定することで、0.5℃単位での温度設定が可能になります。

```CPP
#include <IRac.h>

#define PIN_SEND  12

void setup () {
}

void loop() {
  IRDaikinESP irsend(PIN_SEND);

  irsend.begin();
  irsend.setPower(true);
  irsend.setMode(kDaikinCool);
  irsend.setFan(kDaikinFanAuto);

  //irsend.setTemp(28);
  float temp = 27.5;
  uint8_t *raw = irsend.getRaw();
  raw[22] = static_cast<int>(temp * 2);
  irsend.setRaw(raw);

  irsend.send();

  delay(10 * 1000);
}
```

コンパイル時に表示されるメモリー使用量は以下のようになっています。

```
. Variables and constants in RAM (global, static), used 28168 / 80192 bytes (35%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1496     initialized variables
╠══ RODATA   920      constants       
╚══ BSS      25752    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59879 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27111    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 237028 / 1048576 bytes (22%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     237028   code in flash   
```

コード使用量はおよそ240kBとなっており、`IRac` クラスを使った場合のコード使用量である330kBに比べると、90kB程度少なくなっています。`IRac` クラスでは対応していないメーカー固有の機能を操作したい場合や、コードサイズが問題になるような場合は、このようにメーカー固有のクラスを使うと良いでしょう。
(`IRac` クラスを使う場合でも、`#define` を制御することでコードサイズを削減する方法がありますが、今回は割愛します。)


## I²Cを使ってLCD表示を行う

次はWA-MIKANでI²C通信を使ってみましょう。

Arduinoでは `Wire` オブジェクトを使うことでI²C通信ができます。
本家のArduinoのデフォルト設定では、A4ピンとA5ピンをSDAとSCLに使うことになっていますが、ESP8266でも同様にIO4ピンとIO5ピンがデフォルトとなっています。
他のピンを使うこともできますが、今回はデフォルトの設定をそのまま使います。
ESP8266のIO4とIO5をWA-MIKANのピンと接続するためには、下記の通り、J16およびJ15をショートして使う必要があります。

| ESP8266ピン番号 | WA-MIKANピン番号 | I²C信号 | 備考 |
|-----|----|-----|-----------------------|
| IO4 | 16 | SDA | J16をショートして使う |
| IO5 | 14 | SCL | J15をショートして使う |

**注意:** GR-CITRUSの14ピンと16ピンはA0とA2が割り当てられています。もし、再度WA-MIKANとGR-CITRUSを組み合わせて使いたい場合、GR-CITRUS側で14ピンと16ピンを使うならばJ16とJ15のショートを元に戻す必要があります。

8章で実装したGR-CITRUS向けのArduinoライクなスケッチがそのままWA-MIKANでも使うことができます。ただし `Wire1` オブジェクトではなく `Wire` オブジェクトを使います。

それに加え、WA-MIKAN向けのコンパイラはGR-CITRUS向けのコンパイラ (GCC 4.8) より新しいため、配列の渡し方が少し簡単に書けるようになっています。
GR-CITRUSでは、配列を関数に渡す場合、一度変数に代入する必要がありました。

```CPP
    uint8_t cmds3[] = {0x38, 0x0C, 0x01};
    send_seq(cmds3);
```

一方、WA-MIKANでは以下のように配列を直接関数の引数に書くことができます。

```CPP
    send_seq({0x38, 0x0C, 0x01});
```

この2点について変更した部分を以下に示します。

```CPP
class Lcd {

  ...

  void init() {
    delay(10);
    send_cmd(0x38);
    delay(2);
    send_seq({0x39, 0x14});

    uint8_t contrast = 0x20;
    send_seq({
      uint8_t(0x70 + (contrast & 0x0F)),
      uint8_t(0x5C + ((contrast >> 4) & 0x03)),
      0x6C});
    delay(200);

    send_seq({0x38, 0x0C, 0x01});
    delay(2);
  }

  ...

  void send_seq(const uint8_t *cmds, size_t cmdlen, const uint8_t *data=nullptr, size_t datalen=0) {
    Wire.beginTransmission(addr);
    if (data == nullptr) {
      // Only command data
      Wire.write(0x00);       // Command byte: Co=0, RS=0
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire.write(cmds[i]);  // Command data byte
      }
    } else {
      // Send command words (if any)
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire.write(0x80);     // Command byte: Co=1, RS=0
        Wire.write(cmds[i]);  // Command data byte
      }
      // Send RAM data bytes
      Wire.write(0x40);       // Command byte: Co=0, RS=1
      for (size_t i = 0; i < datalen; ++i) {
        Wire.write(data[i]);  // RAM data byte
      }
    }
    Wire.endTransmission();
  }

  ...

};
```

あとは、GR-CITRUSと同じやり方でLCDに文字を表示できます。

```CPP
Lcd lcd;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  lcd.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.set_cursor(0, 0);
  lcd.print("Hello World");
  delay(1000);
}
```

これでLCDには "Hello World" と表示されます。

ソースコード全体は [`sketch_mikan_lcd.ino`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/sketch_mikan_lcd.ino) から取得できます。
