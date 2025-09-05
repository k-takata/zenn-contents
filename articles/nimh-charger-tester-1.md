---
title: "NiMH充電器&測定器の作成 (その1)"
emoji: "🔋"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "avr", "pcbway"]
published: false
---

## 概要

以前、[USB NiMH ゆっくり充電器](https://github.com/k-takata/PCB_USB_NiMH_Charger/tree/batt-4)というものを作ってみたのですが、ニッケル水素電池の劣化が進んでくると充電監視がうまく動かない問題がありました。そこで、ニッケル水素電池の劣化度合いを定量的に見てみたいと思い、容量や内部抵抗を測定できる充放電器を作ってみることにしました。

機能や特徴としては以下のようになります。

* 「USB NiMH ゆっくり充電器」を踏襲し、USB Type-Cで電源供給。
* 「USB NiMH ゆっくり充電器」を踏襲および以下の効果を期待し、0.1C程度でゆっくりと充電。
  1. 電池の劣化を抑える。
  2. 他の充電器では充電できない程度に劣化の進んだ電池でも充電できるようにする。(大電流を必要としない用途ではまだ使える可能性があるため。)
* 単3または単4を1本充放電し、容量や内部抵抗を測定。
* 充放電状況をLEDで表示。詳細情報はOLEDディスプレイに表示。

作成に当たっては、以下の書籍を参考にしました。

1. [トランジスタ技術SPECIAL No.135 Liイオン/鉛/NiMH蓄電池の充電&電源技術](https://shop.cqpub.co.jp/hanbai/books/46/46751.html)   
   「第4章　研究！ ニッケル水素蓄電池の耐久テスト」 下間 憲行著
2. [トランジスタ技術SPECIAL No.170 教科書付き 小型バッテリ電源回路](https://www.cqpub.co.jp/trs/trsp170.htm)   
   「Appendix 1　充放電回数の限界…サイクル耐久特性の実測」 下間 憲行著
3. [電池応用ハンドブック](https://www.cqpub.co.jp/hanbai/books/34/34461.htm)


## ハードウェア

回路図と基板は[PCB_NiMH_Charger_Tester](https://github.com/k-takata/PCB_NiMH_Charger_Tester)で公開しています。

基本的な回路は参考文献\[1\]の回路をベースとし、以下のような変更を行っています。

* 電源をUSB Type-Cに変更し、それに合わせて電源電圧を5Vとした回路に変更。
* 参考文献\[2\]のブロック図を参考に、充電電流の制御にはP型MOSFETを使用し、放電電流の制御にはN型MOSFETを使用するように変更。
* 充放電電流は0.1C程度の比較的小さい電流とするため、それに合わせて抵抗値を調整。
* AVRマイコンを[AVR64DD28](https://akizukidenshi.com/catalog/g/g118314/)に変更し、マイコンへのプログラミングにはUPDI (Unified Program and Debug Interface)を使うように変更。
* シリアル通信端子はRS-232Cではなく、近年よく使われている6pin TTLシリアル端子を使用。
* 制御電圧の生成を簡素化。(基準電圧ICの出力をPWM制御するのではなく、電源電圧を基準としたPWM制御で目的の電圧を直接生成。)
* 温度監視もできるように温度センサーを追加。
* 挿している電池を検出できるようにブリッジダイオードを追加。
* その他部品や定数の調整。

いくつかの部品については、部品の入手状況や半田付けの難易度によってどちらかを選択するようになっています。
例えばUSBコネクターについては、[通常のUSB Type-Cコネクター](https://akizukidenshi.com/catalog/g/g114356/)か、[電源供給用のコネクター](https://akizukidenshi.com/catalog/g/g116438/)を選択できるようになっています。

昨年、[IoT環境メーター/スマートリモコン](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)を作った際と同様に、今回も[PCBWay](https://www.pcbway.com/)様からのスポンサーの申し出があったため、PCBWay様に基板を発注しています。


## PCBWayでの発注

PCBWayへの発注方法は、[IoT環境メーター/スマートリモコン](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)に記載した内容から大きな変更はありませんので、基本的には前回の記載内容を参照してください。ここでは、前回からの変更点を中心に説明します。


### ガーバーファイルの作成

KiCadでPCBWay用のガーバーファイルを作成するには、専用のプラグインを使うのが楽です。

前回はKiCad 7.0を使用していましたが、今回はKiCad 9.0を使用したため、プラグインのインストール方法に少しだけ変更がありました。

KiCadのプラグイン＆コンテンツ マネージャーを開いてから、「基板製造用プラグイン」タブに切り替えます。そこで "PCBWay" で検索すると、2つのプラグインが見つかります。

![KiCad 9.0 plugins](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/kicad90-plugins.png)

プラグインの機能や使い方は前回から変わりありません。

* [PCBWay Plug-in for KiCad](https://www.pcbway.com/blog/News/PCBWay_Plug_In_for_KiCad_3ea6219c.html)  
  ボタン一発で、PCBWay向けのガーバーファイルの作成から、ブラウザで注文画面を開いてガーバーファイルのアップロードまでできてしまいます。ただし、注文画面は[英語画面](https://www.pcbway.com/orderonline.aspx)が開くようになっており、[日本語画面](https://www.pcbway.jp/orderonline.aspx)は開けません。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの直下に作成されます。
* [PCBWay Fabrication Toolkit](https://www.pcbway.com/blog/News/PCBWay_Fabrication_Toolkit_for_Kicad_23c41e77.html)  
  ボタン一発でPCBWay向けのガーバーファイルの作成ができます。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの下に `pcbway_production` というディレクトリが作成され、その下に作成されます。

ボタン一発で注文画面まで行ける方がよく、英語も苦にならない場合は、前者がよいでしょう。
一方、発注前にガーバーファイルの内容を確認したい場合や、日本語画面から注文したい場合は後者がよいでしょう。(私は後者を利用しています。)


### 注文画面

前回の注文時からは数か所変更がありました。

1. FR4-TG(ガラス転移温度)の選択欄の注意を見ると、2層基板の基材は無料で自動的にS1000H TG150にアップグレードされるとの記載がありました。
   ![PCBWay order: base material](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/pcbway-order-base-material.png)
   通常使用においてはあまり効果はないかもしれませんが、高温環境下で動かす基板を作成したい場合などは嬉しいかもしれません。
   [PCBWay Upgrades PCB Material to ShengYi Material - News - PCBway](https://www.pcbway.com/blog/News/PCBWay_Upgrades_Multi_Layers_PCB_Material_to_ShengYi_Material_d4ba6d1c.html)

2. 基板の厚みは最大で8.0mmが選択できるようになっていました。(前回は最大で6.0mm)

3. UV printing Multi-colorというものが選択できるようになっていました。
   [Unlock Color PCB Printing with PCBWay! - News - PCBway](https://www.pcbway.com/blog/News/Unlock_Color_PCB_Printing_with_PCBWay_0939d559.html)

4. 「チェックマークは、追加料金なしで当社の裁量で「HASL」を「ENIG」に変更することがあることに同意することを意味します。」のチェックマークが無くなっていました。

ところで、前回もそうでしたが、注文画面に表示される製造時間が24horasとtypoされているのが気になります。改善を期待したいです。
![PCBWay order: typo](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/pcbway-order-typo.png)

なお、JLCPCBではしばらく前に発注番号の削除が無料でできるようになりましたが、PCBWayでは引き続き有料となっていました。
![PCBWay order: remove product no](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/pcbway-order-remove-product-no.png)


### 配送方法

配送方法は前回から変更は無いようです。

前回同様、担当者にOCSを選択することを強く推奨されました。今回は2 ~ 3営業日で$9.77と表示されました。前回は$13.07でしたので、少し安くなっていました。
OCSよりも安価な業者もいくつかありますが、配送期間とのバランスを考えると推奨通りOCSが一番よさそうです。


### 支払い

今回もPCBWay様のご厚意により、基板製作費と送料を負担していただけることになりました。
[注文画面](https://www.pcbway.jp/QuickOrderOnline.aspx)からガーバーファイルをアップロードし、レジスト色と注文番号の位置指定のオプションを選んで(それ以外はデフォルトのまま)カートに追加すると、製造番号が発行されましたので、それを担当者に連絡したところ、製作費とOCS前提の送料が自分のアカウントの残高として追加されました。

支払い時に、配送業者としてOCSを選択し、クーポン選択欄でAccount Balance (アカウント残高)にチェックを入れたところ、無事$0.0で発注ができました。
![payment](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/payment.png)

支払い方法として、PayPalやクレジットカードを使う場合は、PayPal feeやBank feeとして1ドル程度取られるようですので、留意が必要です。


なお今回は[PCBWay Ruler](https://www.pcbway.com/project/gifts_detail/PCBWay_Ruler_All_Color.html) 9色セットを購入したいと思って$9.00を支払うつもりでいたところ、こちらも併せてPCBWay様にサポートしていただけることになりました。ありがとうございます。
PCBWayの[ギフトページ](https://www.pcbway.com/project/gifts.html)の検索欄で "ruler" で検索すると他にも何種類かのPCB定規が見つかります。(なお、検索欄のHOT SEARCHESでは "PCB Ruler" という候補も出たのですが、これだと1件も見つかりません。単に "ruler" とするか "PCBWay Ruler" で検索する必要があります。)


### 生産追跡

PCBの生産状況は[メンバーページ](https://member.pcbway.jp/)から確認することができます。今回は8/13の朝に発注して、8/16の朝に完成しました。青のレジストを選択して、3 ~ 4日の予定のところ、3日で完成したので、比較的早くてよかったです。
![production tracking](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/production-tracking.png)

前回は、日本語サイトでは生産追跡画面上のビデオの再生がうまくいかなかったのですが、今回試したところ、問題なく再生できるようになっていました。改善ありがとうございます。


## 基板到着

元々PCBWayでの基板の製造が3 ~ 4日、OCSでの送付に2 ~ 3日の予定でしたが、実際には8/13に製造開始して、3日後の8/16に完了、同日中にOCSに引き渡され、その2日後の8/18に到着しました。結果的にほぼ最速の、製造開始から5日で受け取ることができました。

前回と同じサイズの箱で届きました。PCBWay Rulerも一緒に入っています。

https://x.com/k_takata/status/1957249028413890592

USBコネクター周りでKiCadのDRCエラーが複数出ていたため、意図通りに製造されるか気になっていましたが、全く問題ありませんでした。
前回は10枚発注してなぜか11枚入っていましたが、今回は発注通り10枚届きました。

PCBWay Rulerの別写真です。

https://x.com/k_takata/status/1957424703951540463

PCBWay Rulerは実際のPCBで使える技術そのままで製造されているので、自分が基板を作成する際の色見本としても便利ですし、PCBの加工精度を確認することもできます。つやあり・つや消しを見比べたり、パターンを眺めているだけでも面白いです。もちろん、名前通り定規として電子部品のサイズを測ったりフットプリントを確認するのにも便利です。


### 受取確認

無事基板を受け取ったので、メンバーページから受取確認を行います。受取確認画面の文言が分かりにくいのは特に改善はされていませんでした。

「受け確認」と書かれたボタンを押すと「受け確認?」とだけ表示されたポップアップが現れ、OKすると受け取り確認が完了します。

ボタンは「受取確認」、質問メッセージは「製品を受け取りましたか？」になっていると分かりやすいと思いました。引き続き、日本語訳の改善に期待したいです。

今回も受取確認を行ったことで、[PCBWayリワード画面](https://member.pcbway.jp/specials/rewards)でポイントとBeansが加算されていることが確認できました。
1回の注文ごとに10ポイント、さらに$1の支払いにつき1ポイントで、合計33ポイント加算されていました。


## 組み立て

おおよそ以下の順序で組み立てるのがよいでしょう。

1. USBコネクター
2. CC1/CC2抵抗
3. チップ部品
4. 背の低い部品
5. 背の高い部品
6. 電池ケース


### USBコネクター

前述の通りUSBコネクターは、[通常のUSB Type-Cコネクター](https://akizukidenshi.com/catalog/g/g114356/)か、[電源供給用のコネクター](https://akizukidenshi.com/catalog/g/g116438/)を選択できるようになっています。

https://x.com/k_takata/status/1957421486320538038

電源供給用のコネクターを使う場合は、基板から少し飛び出すように取り付けます。

通常のコネクターを使う場合は、コネクターの先端と基板の縁を合わせるように取り付けます。その際、コネクターのシェル(金属部分)と電源供給用コネクター用のパッド(6個)が接触しないよう、ポリイミドテープ等でパッドを絶縁しておきます。

![polyimide](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/nimh-charger-tester/polyimide.jpg)

端子とパッドにフラックスを塗り、半田ごての先端には少量の半田を乗せ、端子とパッドに半田を染み込ませるように半田付けします。

半田付けが正しくできたかを確認するには、[USBtype-CコネクターDIP化キット(全ピン版)](https://akizukidenshi.com/catalog/g/g113471/)などと組み合わせて導通確認をするのが便利です。
CC1/CC2端子は向きがあるので、ケーブルの裏表を入れ替えて導通確認をする必要があります。DIP化キット側と作成した基板側のそれぞれのケーブルの向きによって、以下の4通りの導通パターンがありますが、少なくとも基板側はケーブルの裏表を変えて導通確認すべきです。

| DIP化キット側 | 基板側 |
|---------------|--------|
| CC1           | CC1    |
| CC1           | CC2    |
| CC2           | CC1    |
| CC2           | CC2    |

あるいは、専用のテスターを作っている方もいますので、そのようなものを使う(あるいは作る)のもよいかもしれません。(私自身は試していません。)

* [USB Type-C ソケットテスターを作った - @74thの制作ログ](https://74th.hateblo.jp/entry/2023/04/15/150437)
* [USB-C 2.0 ソケットテスターの作成 #USB-C - Qiita](https://qiita.com/nak435/items/256b84d4f6ccc4b889c2)


### CC1/CC2抵抗

USBコネクターのCC1/CC2端子につながっている抵抗(R1, R2)を取り付けると、USB経由の電源供給が動くようになります。この段階でUSB経由で5Vが正しく供給されることを確認しておくとよいでしょう。ケーブルの裏表を入れ替えてどちらでも5Vが供給されることを確認しておきます。


### チップ部品

チップ部品は熱に弱いので、注意して取り付けます。私は温度センサーICを1つ壊してしまいました。

特にU5の温度センサーICは、電池ケースを取り付けてしまうと交換が困難になってしまいますので、電池ケースを取り付ける前に動作確認をしておくのがお勧めです。

USBコネクター(J1 or J2)、ポリヒューズ(F1 or F2)、CC1/CC2抵抗(R1, R2)を取り付けると、USB経由で5Vが供給できるようになりますので、この状態でU5の出力電圧を確認しておきましょう。
U5(とU3)で使用している温度センサーIC [MCP9700BT-E/TT](https://akizukidenshi.com/catalog/g/g130948/)は、0℃で0.5Vを出力し、10.0mV/℃の温度勾配がありますので、例えば20℃であれば0.70V、25℃であれば0.75Vが出力されます。それに近い電圧が出ていればよいです。


### 残り

残りの部分の組み立てに関しては、極性がある部品や、見た目が似た部品を間違えないように気を付ければ、大きな問題はないでしょう。基本的には背の低い部品から取り付けていけばよいです。


### LED周り

LEDは充電表示を赤色、放電表示を緑色としていますが、他の色でも問題ありません。R8が充電表示LED用の電流制限抵抗で、R9が放電表示LED用の電流制限抵抗となっていますので、LEDの明るさを変えたい場合は、これらの抵抗の値を変更するとよいです。


### 完成品

(U5が壊れてしまった)完成品です。OLEDは取り外した状態になっています。

https://x.com/k_takata/status/1958694681119772762


## 反省点

1台目はU5の半田付けに失敗して壊してしまったことに気づかないまま最後まで組み立ててしまい、修理が困難になってしまいました。途中で動作確認をしておくべきでした。
U5は電池の温度を測定するために電池のそばに配置しているので、配置を変更して修理しやすくするのは難しいですが、サーミスターなど他の部品を使うことを検討するのもよいかもしれません。

AVRマイコン(U1)とオペアンプ(U2)をかなり近づけて配置してしまったことにより、ICの取り外しが困難になってしまいました。IC取り外し工具を差し込めるようにもう少し離して配置すべきでした。

UPDIコネクターは、[SerialUPDI](https://github.com/SpenceKonde/AVR-Guidance/blob/master/UPDI/jtag2updi.md)の記載を参考に3pinコネクターのピンソケットとしましたが、AVRのマニュアル等を確認したところ、UPDI v2と呼ばれる4pinコネクターのピンヘッダーが正式なもののようです。(UPDI v1と呼ばれる6pinコネクターもありますが、今回は割愛します。)
[AVR Programming Adaptor](https://www.microchip.com/en-us/development-tool/AC31S18A)を見ると以下のようになっていました。

| Pin | 機能  | 色 |
|-----|-------|----|
|   1 | RESET | 白 |
|   2 | VDD   | 赤 |
|   3 | GND   | 黒 |
|   4 | UPDI  | 緑 |

高電圧プログラミングを行わなければRESETピンの接続は不要ですが、できれば正式なコネクターに合わせておきたいところです。

OLEDのコネクターの位置が1mm程度内側にずれていました。

ピンヘッダーやピンソケットのドリル径はデフォルトの1.0mmを使っていましたが、0.9mmにした方がブレが少なくなって良さそうです。

単3電池の "+" マークのシルクの位置が(なぜか)ずれていました。(操作ミス?)

ブリッジダイオードも、チップ部品とスルーホール部品を選択できるようにしておきたかったです。

TTLシリアル端子のピン順序が逆になっていました。(1pin側がGNDのところ、6pin側をGNDにしていた。)

AVRのプログラム未書き込みの状態で電源を入れると、充電LEDと放電LEDが同時に点灯してしまう問題が起きていました。R12, R15の値を元の47kΩから、手持ちの抵抗に合わせて適当に10kΩに変更していたことが原因でしたので、元の値に戻すことにしました。(抵抗入りトランジスターに入っている抵抗のことを見落としていました。)

U5周りを除き、上記の点を修正したRev. 2を[リポジトリ](https://github.com/k-takata/PCB_NiMH_Charger_Tester)にコミットしてありますが、実際に発注するかは未定です。

PCBWayでは5枚も10枚も同じ値段で発注できるので、今回も10枚発注してしまいましたが、改善点が出てくることを見越して5枚に抑えておいてもよかったかもしれません。


### 今後の課題

現在は単3または単4いずれか1本しか充放電できませんが、2本同時に充放電できるように拡張することを検討したいです。ただ、基板のサイズやAVRの空きピン数により、増やすのは難しいかもしれません。

ソフトウェアの作成にも関わりますが、基準電圧IC U4や、環境温度測定用の温度センサーIC U3を省略しても機能を実現できるか検討したいです。精度や安定性が十分であれば、AVR内蔵の基準電圧や温度センサーが使えるかもしれません。


## 続き

ソフトウェアは現在作成中です。出来上がり次第、続きを公開する予定です。
