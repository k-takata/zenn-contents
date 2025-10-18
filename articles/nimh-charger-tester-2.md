---
title: "NiMH充電器&測定器の作成 (その2、ソフトウェア編)"
emoji: "🔋"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "avr", "pcbway", "arduino"]
published: true
---

## 概要

本記事は、以下の記事の続きとなります。

* [NiMH充電器&測定器の作成 (その1、ハードウェア編)](https://zenn.dev/k_takata/articles/nimh-charger-tester-1)

その1では[PCBWay](https://www.pcbway.jp/)様のスポンサーで基板を作成しましたが、今回はその基板上で動くソフトウェアの作成について記載します。

作成する「NiMH充電器&測定器」の機能や特徴を再掲します。

* 「[USB NiMH ゆっくり充電器](https://github.com/k-takata/PCB_USB_NiMH_Charger/tree/batt-4)」を踏襲し、USB Type-Cで電源供給。
* 「USB NiMH ゆっくり充電器」を踏襲および以下の効果を期待し、0.1C程度でゆっくりと充電。
  1. 電池の劣化を抑える。
  2. 他の充電器では充電できない程度に劣化の進んだ電池でも充電できるようにする。(大電流を必要としない用途ではまだ使える可能性があるため。)
* 単3または単4を1本充放電し、容量や内部抵抗を測定。
* 充放電状況をLEDで表示。詳細情報はOLEDディスプレイに表示。

作成に当たっては、その1に引き続き、以下の書籍を参考にしました。

1. [トランジスタ技術SPECIAL No.135 Liイオン/鉛/NiMH蓄電池の充電&電源技術](https://shop.cqpub.co.jp/hanbai/books/46/46751.html)   
   「第4章　研究！ ニッケル水素蓄電池の耐久テスト」 下間 憲行著
   (初出: トランジスタ技術 2010年2月)
2. [トランジスタ技術SPECIAL No.170 教科書付き 小型バッテリ電源回路](https://www.cqpub.co.jp/trs/trsp170.htm)   
   「Appendix 1　充放電回数の限界…サイクル耐久特性の実測」 下間 憲行著
   (初出: トランジスタ技術 2022年3月)
3. [電池応用ハンドブック](https://www.cqpub.co.jp/hanbai/books/34/34461.htm)


## 開発環境

### Arduino IDE

開発には[Arduino IDE](https://www.arduino.cc/en/software/) 2.3.6を使用します。

マイコンには[AVR64DD28-I/SP](https://akizukidenshi.com/catalog/g/g118314/)を使用していますので、対応するボードパッケージとして[DxCore](https://github.com/SpenceKonde/DxCore)を使用します。[DxCore Installation](https://github.com/SpenceKonde/DxCore/blob/master/Installation.md)にしたがってボードマネージャ経由でインストールを行います。

コンパイルに当たっては、以下の項目の設定を変更しています。

* MultiVoltage I/O (MVIO): "Disabled"
  PC0 ~ PC3ピンをアナログ入力できるようにするため。
* printf(): "Full x.xk, prints floats"
  printfで浮動小数を扱えるようにするため。(フラッシュメモリー使用量は増えるが許容範囲内)


### 書き込みツール

[SerialUPDI](https://github.com/SpenceKonde/AVR-Guidance/blob/master/UPDI/jtag2updi.md)の記述にしたがって、UPDI書き込みツールを用意します。

適当なUSBシリアルアダプターと、470Ω抵抗、ショットキーバリアダイオードがあれば簡単に書き込みツールが作成できます。

今回は、USBシリアルには秋月電子の[AE-CH9102F-TYPEC-BO](https://akizukidenshi.com/catalog/g/g129505/)を使用し、それと組み合わせるためのUPDIアダプターとして、[UPDI Adaper for AE-CH9102F](https://github.com/k-takata/PCB_UPDI_for_AE-CH9102F)というものを作成しました。


## ソフトウェア作成方針(当初案)

当初は以下のような機能を実現する予定で開発を始めました。

* 単3と単4のどちらの電池が挿入されているかを自動検出する。
* 充電電流は0.1Cを基本とし、単3は200mA、単4は70mAとする。
* 「USB NiMH ゆっくり充電器」やその元となった「[いたわりNiCd充電器 キット](https://akizukidenshi.com/img/contents/kairo/%E3%83%87%E3%83%BC%E3%82%BF/%E5%85%85%E9%9B%BB%E5%99%A8%E9%96%A2%E4%BF%82/H001%E3%81%84%E3%81%9F%E3%82%8F%E3%82%8ANiCd_.pdf) \[PDF\]」と同様に、充電時の電圧を監視し、指定の電圧に達した時点で終了とする。
* 充電時の温度も測定し、充電電圧に約-3mV/Kの温度係数を含める。
* 安全のため、タイマーでの充電停止も備える。
* 安全のため、温度監視での充電停止も備える。
* 内部抵抗の大きくなってしまった電池でも充電電圧を正しく測定できるようにするため、電圧測定時には一時的に充電を停止し、間欠的に充電を行う。
* 充電中の電圧と、充電一時停止中の電圧を比較することで、電池の内部抵抗を計算し、表示する。

ただ、実際に開発を進めてみると、当初の想定とは異なることがいろいろ出てきました。ここからは、概ね開発を進めた手順をなぞりながら各機能を説明していきます。


## 要素機能の実装

まずは充放電機能を実現するために必要な要素機能(電圧測定等)を実装していきます。


### 電源電圧の測定機能

USBの電源は5.0Vですが、実際には5%程度の誤差がありえます。実際、今回使用していたUSB電源は5.16Vが出力されていました。

今回作成した回路では、電源電圧が様々な電圧・電流の基準となっているため、正確な電源電圧を知ることが重要です。そこでまずは電源電圧を測定する機能を作成します。

```CPP
#define PIN_VREFA   PIN_PD7

// TP1 (VREFA, TL431) の実際の電圧。理想的には 2.495 V
const float vrefa_calib = 2.483;

// TL431 の ADC 値を使い Vdd 電圧を算出する
float getVddVolt()
{
  int vrefa = analogReadEnh(PIN_VREFA, 15);
  float vdd = vrefa_calib * 32768.0 / vrefa;
  return vdd;
}
```

AVRのADC機能は、デフォルトでは電源電圧を基準とした値を返します。基準電圧を他のものに変更することもできますが、今回の回路では、0Vから電源電圧までの範囲で電圧測定を行いたいので、必然的に基準は電源電圧とすることになります。(測定できる電圧は0Vから基準電圧までなので。)

基準電圧ICの[TL431](https://akizukidenshi.com/catalog/g/g112018/)の出力をPD7ピン(VREFAピン)に入力し、そのADC値を取得し、そこから電源電圧を逆算しています。
TL431の公称出力電圧は2.495Vですが、実際には多少の誤差があるので、実測値を使って計算を行います。

Arduinoにおいて、ADC値を取得する関数は、`analogRead()` です。デフォルトの精度は8bitですが、`analogReadResolution()` を使うことで12bit精度にできます。しかし、今回はより高精度に測定を行うため、DxCoreの拡張関数である `analogReadEnh()` を使うことで15bitオーバーサンプリングを行うことにしました。

なお、AVR自身にも1.024V, 2.048V, 4.096V, 2.500Vの電圧リファレンスが内蔵されており、これらを基準にVDDDIV10 (Vddを1/10にした電圧)を測ることでVddを算出することもできます。ただ、この方法の場合、Vddを測定する際には基準電圧を内蔵リファレンスに変更し、その他の電圧を測定する際には基準電圧をVddに戻す手間が発生します。さらにVddを1/10にするためのAVR内部の分圧抵抗の誤差も考慮する必要があります。実際に測定した値を見た範囲では、TL431を使った方式の方が安定性が高そうでした。


### 電池電圧の測定機能

BT1(単3)あるいはBT2(単4)の電圧を取得する関数です。
電池のADC値を取得し、電源電圧を掛けることで実際の電圧を算出します。
15bitオーバーサンプリングを行って正確な値を取得するか、オーバーサンプリングを行わずに短時間で値を取得するかを選択できます。

```CPP
// BT1 あるいは BT2 の電圧を取得
float getBtVolt(int bt, bool hires=true)
{
  if (hires) {
    int vbt = analogReadEnh(PIN_VBT1 + bt - 1, 15);
    return vbt * getVddVolt() / 32768.0;
  }
  else {
    int vbt = analogRead(PIN_VBT1 + bt - 1);
    return vbt * getVddVolt() / 4096.0;
  }
}
```


### 電池の挿入状態の検出機能

[今回の回路](https://github.com/k-takata/PCB_NiMH_Charger_Tester?tab=readme-ov-file#%E5%9B%9E%E8%B7%AF%E5%9B%B3)では、充電回路・放電回路・BT1(単3)・BT2(単4)の間にブリッジダイオードを配置することで、BT1とBT2の電圧を別々に取得できるようにしたつもりでいました。

しかし実際に試してみたところ、BT1、BT2のどちらか一方だけを挿入した状態で充電を続けていると、BT1とBT2の測定値が同じになってしまい、どちらに電池が挿入されているか判別できなくなってしまう問題が発生しました。

おそらくはブリッジダイオードの漏れ電流によって、BT1とBT2の電圧が同じになってしまったと思われます。

そこで対策として、充放電を一時停止し、コンデンサー(C9, C10)の電荷を放電してからBT1, BT2の電圧を測定することで、電池の挿入状態を検出することにしました。

```CPP
#define PIN_VBT1    PIN_PC0
#define PIN_VBT2    PIN_PC1

#define BT_HIGH_THRESHOLD 1.8
#define BT_LOW_THRESHOLD  0.5

// 電池の接続状態をチェック
//  戻り値:
//   -1: エラー。BT1またはBT2が1.8Vを超えている
//    0: 電池未接続
//    1: BT1が接続
//    2: BT2が接続
//    3: BT1とBT2の両方が接続
int checkBtConnection()
{
  chgctl(false);    // 充電停止
  disctl(false);    // 放電停止

  // VBT1,2 ピンに接続されたコンデンサーを放電 (20ms)
  pinMode(PIN_VBT1, OUTPUT);
  pinMode(PIN_VBT2, OUTPUT);
  digitalWrite(PIN_VBT1, LOW);
  digitalWrite(PIN_VBT2, LOW);
  delay(20);

  // VBT1,2 ピンを入力に戻して1ms待つ
  pinMode(PIN_VBT1, INPUT);
  pinMode(PIN_VBT2, INPUT);
  delay(1);

  // BT1, BT2の電圧を取得
  float vbt1 = getBtVolt(1, false);
  float vbt2 = getBtVolt(2, false);

  int ret = 0;
  if (vbt1 > BT_HIGH_THRESHOLD || vbt2 > BT_HIGH_THRESHOLD) {
    ret = -1;   // どちらかが1.8Vを超えていればエラー
  }
  else {
    if (vbt1 > BT_LOW_THRESHOLD) {
      ret |= 1; // BT1が0.5Vを超えている
    }
    if (vbt2 > BT_LOW_THRESHOLD) {
      ret |= 2; // BT2が0.5Vを超えている
    }
  }
  Serial.printf(F("checkBtConnection: %.3f V, %.3f V, ret=%d\n"), vbt1, vbt2, ret);
  return ret;
}
```

充放電を一時停止し、ピンを出力に切り替えてコンデンサーの電荷を放電し、20ms待ってピンを入力に切り替え、1ms待ってから電圧を測定します。

電圧が1.8Vを超えていればエラーとし、0.5Vを超えていれば接続されていると判定します。

待ち時間や電圧の閾値は、試行錯誤によって決定しました。これらの値を使うことで、ほぼ誤動作なく電池の挿入状態を検出できるようになりました。


### 充放電電流の設定機能

PWMを使ってオペアンプに入力する電圧を生成し、それによって充放電電流を設定します。

`analogWrite()` は8bit精度のため精度が足りないので、自前でPWM制御を行い10bit精度とすることにしました。


#### PWMの初期化

PWMの制御はAVRの[TCA0 (16-bit Timer Counter Type A)](https://onlinedocs.microchip.com/oxy/GUID-C313CAB3-6FEE-4BE7-B44A-43F360220F96-en-US-17/GUID-6A2FF641-E7E5-49B1-A1EB-1540AAB70BF4.html)を使用しますが、`takeOverTCA0()` を呼ぶことで、TCA0の制御権を取得できます。

TCA0には比較チャンネルが3つありますが、今回はそのうちの2つを使用します。

TCA0にはSplitモードというものがあり、これを使うと1つの16bitカウンターを2つの8bitカウンターとして扱うことができます。`analogWrite()` はこのSplitモードを使って実装されていますが、今回は10bitを扱いたいので、当然Splitモードは使わずに通常モードを使用しています。通常モードのレジスターは `TCA0.SINGLE.xxx` という名前でアクセスします。

```CPP
#define PWM_MAX_VAL 1023  // PWM値の最大値(10bit)

// PWM の初期化 (TCA0)
void initPwm()
{
  takeOverTCA0();   // TCA0 の制御権を取得
  // CMP1, CMP2 を有効化、波形生成・シングルスロープモード
  TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP2EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.PER = PWM_MAX_VAL;    // PWM値の最大値を設定
  //PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;  // PORTD から PWM 出力 (デフォルト)

  setPwmChg(PWM_MAX_VAL);   // 充電電流を 0 に設定 (実装は後述)
  setPwmDis(0);             // 放電電流を 0 に設定 (実装は後述)

  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm; // PWM 動作開始
}
```


#### PWM値の設定

PWM値を設定することで充放電電流を設定する機能を用意します。

PWM値は10bitなので、値が0~1023の範囲に収まるようにクリップします。
クリップした値を `TCA0.SINGLE.CMPx` レジスタに設定すれば完了です。

```CPP
// PWM値をクリップする
int clipPwmVal(int val)
{
  if (val < 0) {
    val = 0;
  }
  else if (val > PWM_MAX_VAL) {
    val = PWM_MAX_VAL;
  }
  return val;
}

// PIN_PWMCHG(充電ピン)のPWM値を設定
void setPwmChg(int val)
{
  val = clipPwmVal(val);
  TCA0.SINGLE.CMP1 = val;
}

// PIN_PWMDIS(放電ピン)のPWM値を設定
void setPwmDis(int val)
{
  val = clipPwmVal(val);
  TCA0.SINGLE.CMP2 = val;
}
```


#### mA単位での電流の設定

続いて、より使いやすくするため、mA単位で電流を設定できる機能を用意します。

充放電用負荷抵抗(R17, R18)の値と現在の電源電圧からPWM値を算出し、前述の関数で設定します。

```CPP
// 充電用負荷抵抗 R17: 10 Ω
const float r_charge = 10.0;
// 放電用負荷抵抗 R18: 1 Ω
const float r_discharge = 1.0;

// mA単位で充電電流を設定
// 精度: Vdd / (PWM_MAX_VAL + 1) / r_charge = 5.0 / 1024 / 10 = 0.488 mA
void setChgCurrent(float ma)
{
  float vdd = getVddVolt();
  float v = vdd - ma * r_charge / 1000.0;
  int val = int(round(PWM_MAX_VAL * v / vdd));
  setPwmChg(val);
}

// mA単位で放電電流を設定
// 精度: Vdd / (PWM_MAX_VAL + 1) / r_discharge = 5.0 / 1024 / 1.0 = 4.88 mA
void setDisCurrent(float ma)
{
  float vdd = getVddVolt();
  float v = ma * r_discharge / 1000.0;
  int val = int(round(PWM_MAX_VAL * v / vdd));
  setPwmDis(val);
}
```


### 充放電電流の取得機能

当初は、前述の設定機能で設定した電流が流れるものと想定していましたが、実際に試してみたところ、設定した電流と実際の電流が大きく食い違う場合がありました。
特に、電池の内部抵抗が大きい状態で、放電電流を大きく設定した場合に、放電電流が設定値を大きく下回る場合がありました。

そこで、基板に2か所改造を行い、充放電電流を測定できるようにしました。

![PCB patch](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/pcb-patch.jpg)

TP2 (Vchg)とPF0ピンを接続し、TP3 (Vdis)とPF1ピンを接続しています。

```CPP
#define PIN_VCHG    PIN_PF0
#define PIN_VDIS    PIN_PF1

// mA単位での充電電流を取得
float getChgCurrent()
{
  float vdd = getVddVolt();
  return vdd * (32767 - analogReadEnh(PIN_VCHG, 15)) / 32768.0 / r_charge * 1000.0;
}

// mA単位での放電電流を取得
float getDisCurrent()
{
  float vdd = getVddVolt();
  return (vdd * analogReadEnh(PIN_VDIS, 15) / 32768.0) / r_discharge * 1000.0;
}
```

充放電用負荷抵抗(R17, R18)の値と現在の電源電圧から電流値を算出しています。


### 温度測定機能

今回の回路では、温度センサーとして[MCP9700BT-E/TT](https://akizukidenshi.com/catalog/g/g130948/)を2個使用しています。

このセンサーは、0℃でのオフセットが500mV、温度係数は10.0 mV/℃ですので、出力電圧を $V$ [V]、温度を $T$ [℃] とすると次の式が得られます。

$$ V = 0.5 + 0.01 T $$

これを変形すると次のようになります。

$$ T = 100 V - 50 $$

これを元に関数を実装します。

```CPP
#define PIN_VTEMP1  PIN_PC2
#define PIN_VTEMP2  PIN_PC3

// ボードの温度を取得
float getTemp1()
{
  int vtemp = analogReadEnh(PIN_VTEMP1, 14);
  return vtemp * getVddVolt() / 40.96 / 4 - 50.0;
}

// 電池近傍の温度を取得
float getTemp2()
{
  int vtemp = analogReadEnh(PIN_VTEMP2, 14);
  return vtemp * getVddVolt() / 40.96 / 4 - 50.0;
}
```


### (おまけ) AVR内蔵センサーを使った温度測定機能

AVR自身にも温度センサーが内蔵されており、それを使って温度を測定することもできます。

温度の計算方法は、[AVR64DD28](https://www.microchip.com/en-us/product/AVR64DD28)の[データシートに記載](https://onlinedocs.microchip.com/oxy/GUID-C313CAB3-6FEE-4BE7-B44A-43F360220F96-en-US-17/GUID-7886FC26-9F09-417B-9EBF-42BDB3B2AB7D.html)されていますので、それを元に実装します。データシートには内蔵の2.048Vリファレンスを使う方法が記載されていますが、電圧リファレンスを切り替える手間を省くため、今回はVddをリファレンスとしています。

実際に試してみたところ、値のぶれが大きかったため、今回は使用しないことにしました。

```CPP
// AVRの温度を取得する
float getTempAvr()
{
  int adc_temp = analogReadEnh(ADC_TEMPERATURE, 14);
  float sigrow_offset = SIGROW_TEMPSENSE1;
  float sigrow_slope = SIGROW_TEMPSENSE0;
  return (sigrow_offset - adc_temp * getVddVolt() / 2.048 / 4) * sigrow_slope / 4096 - 273;
}
```


### 要素機能のまとめ

ここまでで本機の要素機能の実装がほぼ完了しました。次はこれらを組み合わせて充放電機能を実装していきます。


## 充電機能

### 操作

電池を挿入し、Chgボタンを押すと充電が開始され、Chgランプが点灯します。
充電中にChgボタンあるいはDisボタンを押すと、充電が停止します。
充電停止中にChgボタンを押すと充電が再開します。

Modeボタンを押すと表示が切り替わります。(後述)


### 間欠充電

当初より本機では、内部抵抗の大きくなってしまった電池でも充電電圧を正しく測定できるようにするため、電圧測定時には一時的に充電を停止し、間欠的に充電を行うことを予定していました。
(内部抵抗が大きくなると、内部抵抗×充電電流の分だけ充電電圧が高くなってしまいます。)

今回の回路はオペアンプとMOSFETによる定電流充電回路となっていますが、前述の電池挿入状態の検出機能を組み合わせることにより、図らずも半ば自動的に間欠充電が実装されました。

以下の処理を1秒ごとに繰り返しています。

1. `checkBtConnection()` で電池挿入状態の検出を行う。
   - 充電を停止して20ms待つ。
   - 1ms待って、電池の挿入状態をチェックする。
2. 電圧が安定するのを待つためにさらに10ms待って、充電一時停止中の電圧(v\_idle)を測定する。
3. 充電を再開する。
4. 電圧が安定するのを待つために20ms待って、充電中の電圧(v\_chg)を測定する。
5. 次の1秒サイクルまで充電を継続する。

これにより、1秒のうち、約30msは充電一時停止となり、残りの約970msで実際に充電を行っていることになります。

また、v\_idleとv\_chgから電池の内部抵抗を算出することができます。

ところで、間欠充電方式ですが、実はかつて特許が取られていました。(失効済み)
「電池応用ハンドブック」の第3-6章では、電池ホルダーの接触抵抗の影響を回避し、急速充電充電時の電池電圧をより正確に測定するために、充電を一時停止して電圧を測定する「ブレークスルー方式」というものが提案されています。しかし、その後日談として、その方式が既に特許(特公平8-13169)になっていたことが明かされています。出願は1991年ですので、この本の初版が発行された2005年時点ではまだ特許は有効でしたが、2011年には特許は切れています。

:::message
特許は、[特許情報プラットフォーム（J-PlatPat）](https://www.j-platpat.inpit.go.jp/)で検索できます。
:::


### タイマーによる充電停止

0.1Cでの定電流充電の場合、およそ10時間で充電が完了することになります。

実際には充電時の損失があるためもう少し長い時間が掛かり、電池容量のばらつきも考慮し、1.4倍の14時間で充電を停止するようにしています。

タイマーは、充電一時停止中の時間は除外し、実際に充電を行った時間を積算して計算しています。


### 終止電圧監視による充電停止

「USB NiMH ゆっくり充電器」やその元となった「いたわりNiCd充電器 キット」と同様に、充電時の電圧を監視し、指定の電圧に達した時点で終了とする機能を実装しました。また、ボードの温度を測定し、-3mV/Kの補正を加えることにしました。

実際にいくつかの電池で充電を行ってみたところ、電池によりかなり終止電圧が変わる(1.49V ~ 1.52V)ことが判明しました。

より確実な充電停止を行うようにするため、次の方法を実装してみることにしました。


### -ΔV検出による充電停止

次に実装することにしたのは、NiCd/NiMH急速充電器で一般的に使用されている-ΔV (マイナス・デルタ・ブイ)検出による充電停止です。

:::message
-ΔV方式は、前述の特許(特公平8-13169)によると、1982年10月12日付で発行されたアメリカ特許第4,354,148号や1983年6月7日付で発行されたアメリカ特許第4,387,332号等で特許が取得されていたようです。
:::

一般に、充電電流が多いと-ΔVの下がり幅が大きくなり、充電電流が少ないと下がり幅は小さくなると言われています。0.1C充電では-ΔVを検出できない懸念もありましたが、今回はとりあえず4mVを閾値とし、以下のようなアルゴリズムを実装しました。

1. 充電電圧が ~~1.38V~~ 1.42V 以上になった時点で-ΔV検出処理を開始。
2. 10秒ごとの単純移動平均(SMA)を算出し、最大値から4mVの電圧降下が20回検出されたら充電停止。

~~-ΔV検出処理を開始する1.38Vというのは、充電終盤でグラフの傾きが大きくなり始める付近の電圧を指しています。~~ -ΔV検出処理を開始する1.42Vというのは、充電終盤でグラフの傾きが大きくなって少し経った付近の電圧を指しています。(次節のピーク検出が誤検知する場合があったので、検出開始電圧を1.38Vから1.42Vに引き上げました。)
SMAを取っているのは、1回の電圧測定では、数mV程度のぶれは頻繁に発生するからです。

なお、-ΔV検出の前に終止電圧監視で充電が停止してしまわないように、終止電圧の設定は1.53Vに引き上げています。

試してみたところ、一部の電池では-ΔVが検出され、正常に充電が停止しました。その際、電池の温度は40℃程度とほんのり発熱していました。

![Minus Delta V graph](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/minus-delta-v.png)

(縦軸が充電一時停止中の電圧 \[V\]、横軸が積算時間 \[分\])

しかし、別のかなり劣化した電池では-ΔVが検出されず、ダラダラと電圧が上がり続け、最終的には14時間タイマーで充電が停止しました。

![No Minus Delta V graph](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/no-minus-delta-v.png)

より確実な充電停止を行うようにするため、さらに次の方法を実装してみることにしました。


### ピーク検出による充電停止

[バッテリ充電器の新しい展開](https://www.analog.com/jp/resources/design-notes/new-developments-in-battery-chargers.html)を見たところ、次のような記載がありました。

> NiCdの急速充電は、ΔV/Δtが負になったときに停止します。NiMH電池では、端子電圧がピーク(ΔV/Δtがゼロ)になったときに急速充電を停止してください。

また、前述の特許(特公平8-13169)にも、(NiCd電池とは異なり)NiMH電池では-ΔV検出では過充電になる旨の記載がありました。

ということで、ピーク検出による充電停止を実装してみることにしました。今回は以下のような条件で実装してみました。

1. 充電電圧が ~~1.38V~~ 1.42V 以上になった時点で-ΔV検出処理と同時にピーク検出処理を開始。
2. 充電電圧が3分間1mVも上昇しなければ充電停止。

試してみたところ、-ΔVが検出されなかった電池でも、3分間の電圧の上昇停止が検出され、正しく充電が停止されました。電池の発熱も、-ΔV検出に比べて抑えられていました。(下記のグラフは-ΔVが検出できた電池でのピーク検出結果)

![Zero Delta V graph](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/zero-delta-v.png)


### 温度監視による充電停止 (未実装)

今回の回路は0.1C充電を基本としているため、電池への負担はそれほど高くなく、電池の発熱もあまりありません。

ただ、安全性を追求するなら電池の温度を監視し、異常発熱していれば充電を停止するのがよいと思われます。

また、「電池応用ハンドブック」の「第2-7章 ニッケル水素充電回路の実用知識」によると、多くの充電回路は-ΔVよりも、満充電時に電池の温度上昇率が変化するポイントを検出するΔT/Δt検出を採用しており、2℃/分を越えたときに充電完了としている、とのことです。
0.1C充電の場合は温度変化も緩やかになるので、この条件をそのまま使うことはできないでしょうが、同じような機能を実装するのも面白そうです。

今回は、電池近傍の温度センサーを壊してしまったため、これらの実装は行っていません。


### トリクル充電 (未実装)

充電完了後に、自己放電による電力減少を補うため、微小電流での充電を行って満充電状態を維持することをトリクル充電と呼びます。

かつてのNiMHは自己放電が多いことが課題でしたが、eneloop以降のNiMHは自己放電が少ないことから、今回はトリクル充電は実装していません。


### 充電機能のまとめ

正確な充電電圧を測定できるようにするために間欠充電機能を実装しました。

充電停止は以下の4段階で行うことにしました。

1. ピーク検出
2. -ΔV検出
3. 終止電圧監視
4. タイマー


## 放電機能

電池の放電を行って、電池の容量や内部抵抗を測定することができます。


### 操作

電池を挿入し、Disボタンを押すと放電が開始され、Disランプが点灯します。
充電中にChgボタンあるいはDisボタンを押すと、放電が停止します。
放電停止中にDisボタンを押すと放電が再開します。

Modeボタンを押すと表示が切り替わります。(後述)


### 間欠放電

放電についても充電と同じように間欠処理を行っています。以下の処理を1秒ごとに繰り返しています。

1. `checkBtConnection()` で電池挿入状態の検出を行う。
   - 放電を停止して20ms待つ。
   - 1ms待って、電池の挿入状態をチェックする。
2. 電圧が安定するのを待つためにさらに10ms待って、放電一時停止中の電圧(v\_idle)を測定する。
3. 放電を再開する。
4. 電圧が安定するのを待つために100ms待って、放電中の電圧(v\_dis)を測定する。
5. 次の1秒サイクルまで放電を継続する。

これにより、1秒のうち、約30msは放電一時停止となり、残りの約970msで実際に放電を行っていることになります。
充電では、4. の待ち時間が20msでしたが、放電では電圧の安定に時間が掛かることが判明したため、100msに延ばしてあります。

また、v\_idleとv\_disから電池の内部抵抗を算出することができます。

放電の終了はv\_idleで監視を行い、これが放電終止電圧(1.0V)未満になったら放電を停止します。

v\_disが放電終止電圧を下回っていても、v\_idleが放電終止電圧に達していない限り放電を続けるので、電池の内部抵抗が無視できるような低消費電力機器でどれだけの容量を使用できるかが分かります。

放電グラフの例:
![Discharge graph](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/discharge.png)


## その他の機能

### 操作ボタン

操作ボタンは左から順に以下のようになっています。

* SW1: Chg/↑
* SW2: Dis/↓
* SW3: Mode/OK


### 表示機能

Modeボタンで以下の表示を順に切り替えられるようになっています。

1. 詳細表示
2. 簡易表示
3. グラフ表示
4. 設定画面 (充放電停止中のみ)


#### 詳細表示

詳細表示では以下の項目を表示します。

* 電源電圧
* 充放電一時停止中の電池電圧
* 充放電中の電池電圧
* 充放電電流
* 内部抵抗
* 充放電開始からの積算容量
* 充放電開始からの積算時間
* 温度

放電中の例:
![Detail Mode Display](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/display-detail.jpg)

この例では、放電一時停止中の電圧は1.031Vですが、放電電流47.9mAで放電中の電圧は0.360Vまで下がっており、内部抵抗は14.01Ωと相当劣化した電池だということが分かります。(通常は悪くとも200mΩ程度)
10時間20分41秒放電した状態で、それまでの積算容量は493mAh。まだ放電は完了していませんが、そろそろ完了に近づいています。この電池の公称容量は800mAhでしたから、新品の60%程度の容量になってしまっていることが分かります。
ただ、ここまで劣化した電池であっても、ワイヤレスマウス、時計などの低消費電力機器にはまだまだ使用可能です。(本機以外の通常の充電器では充電できませんが…)


#### 簡易表示

簡易表示では以下の項目を表示します。

* 電源電圧
* 充放電一時停止中の電池電圧
* 充放電開始からの積算容量
* 充放電開始からの積算時間

放電中の例:
![Simple Mode Display](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/display-simple.jpg)

放電を開始したばかりの状況です。

フォントは、「[ESP32-C3とBME680でIoT環境メーターを作る](https://zenn.dev/k_takata/articles/esp32c3-envmeter)」で実装した方法と同じように、[Anonymous Pro](https://www.marksimonson.com/fonts/view/anonymous-pro)を使用し、一部の文字は手動で字形を調整しています。


#### グラフ表示

当初は予定していなかった機能ですが、充放電電圧をグラフ表示できるようにしました。

一番下の線が1.0Vで、そこから0.1V刻みで目盛りが引いてあり、一番上の線が1.5V、画面上端が1.6Vです。OLEDディスプレイの縦は64ピクセルですので、1ピクセルがおよそ0.01Vとなります。
横方向は、128ピクセルに収まるように自動で縮尺が調節されます。

グラフを見やすくするため、グラフと目盛り線が交わる部分では、目盛り線を消して線が重ならないようにしています。

充電完了時のグラフ表示の例:
![Charge Graph Display](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/graph-charge.jpg)

充電開始時が約1.18Vで、充電完了時が約1.49Vであることが読み取れます。

放電完了時のグラフ表示の例:
![Discharge Graph Display](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/graph-discharge.jpg)

充放電時の電圧等のデータは、AVRのRAM 8KBのうち、5KB程の領域を確保してそこに保存しています。


### 設定機能

以下の値を設定できます。

* 充電電流 (以下のいずれかを選択)
  - 0.2C
  - 0.1C
  - 0.05C
* 放電電流 (以下のいずれかを選択)
  - 0.5C
  - 0.1C
  - 0.05C
* 充電終止電圧
  - 1.30 ~ 1.60V、0.01V単位で設定
* 放電終止電圧
  - 0.90 ~ 1.10V、0.01V単位で設定

Chg/↑ボタンとDis/↓ボタンでカーソル(`>`)を移動し、Mode/OKボタンで選択します。

設定値はEEPROMに保存されます。


### シリアル通信機能

シリアル通信端子から、充放電時のログやデバッグメッセージを出力できます。

ホストPCから `send` と送信すると、ログがCSV形式で返ってきます。

以下は、充電時のログの例です。

```CSV
Chg/min,Volt,Ohm,Temp,Capacity,Reason
0,1.148,0.335,27.4,
1,1.159,0.335,27.2,
2,1.167,0.335,27.2,
3,1.173,0.330,27.3,
4,1.178,0.330,27.5,
5,1.183,0.330,27.5,

    ...

571,1.520,0.870,25.4,
572,1.520,0.870,25.4,
573,1.520,0.875,25.4,
574,1.520,0.875,25.4,
575,1.520,0.875,25.5,
576,1.520,0.870,25.4,1914,ZeroDeltaV
```

1分ごとに、充電一時停止中の電圧、内部抵抗、温度を記録しています。
充電完了した場合は、最後の行に積算容量と停止理由も表示されます。


## ソースコード

作成したソースコードは以下に格納しています。

<https://github.com/k-takata/zenn-contents/tree/master/articles/files/nimh-charger-tester>


## まとめ

他の充電器では充電できない程度に劣化の進んだ電池でも充電できるようにするという当初の予定は無事実現できました。
充放電容量の計測機能と内部抵抗の測定機能も実現でき、電池の劣化状況が定量的に分かるようになりました。

また、当初予定していなかったピーク検出や-ΔV検出による安全な充電停止が実現でき、それが相当劣化した電池でも有効に動作することが確認できました。
さらに、グラフ表示機能により、充放電状況が視覚的に把握できるようになりました。

結果的に当初の予定を上回るものができあがりました。(温度監視機能を除く)


## 今後に向けて

その1では基板にいくつかの課題が見つかっており、ソフトウェア作成中にも前述の通り修正が必要な点が見つかったため、それらの修正を行った基板(Rev. 2)を既に発注しています。(スポンサーしていただいたPCBWay様ではなく、今回はJLCPCBに発注中です。)

0.1C充電では、ピーク検出や-ΔV検出が上手く動作することが確認できましたが、0.2C充電や0.05C充電など、充電電流を変えた時の動作も検証したいです。

温度センサーを直して、温度監視の動作も検証したいです。
