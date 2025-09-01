---
title: "NiMH水素充電器&測定器の作成 (その1)"
emoji: "🔋"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "avr", "pcbway"]
published: false
---

## 概要

以前、[USB NiMH ゆっくり充電器](https://github.com/k-takata/PCB_USB_NiMH_Charger/tree/batt-4)というものを作ってみたのですが、ニッケル水素電池の劣化度合いを定量的に見てみたいと思い、容量や内部抵抗を測定できる充電器を作ってみることにしました。

機能や特徴としては以下のようになります。

* USB NiMH ゆっくり充電器を踏襲し、USB Type-Cで電源供給。
* USB NiMH ゆっくり充電器を踏襲し、0.1C程度でゆっくりと充電。
* 単3または単4を1本充電・測定。
* OLEDディスプレイに表示。

作成に当たっては、以下の書籍を参考にしました。

1. [トランジスタ技術SPECIAL No.135 Liイオン/鉛/NiMH蓄電池の充電&電源技術](https://shop.cqpub.co.jp/hanbai/books/46/46751.html) 「第4章　研究！ ニッケル水素蓄電池の耐久テスト」 下間 憲行著
2. [トランジスタ技術SPECIAL No.170 教科書付き 小型バッテリ電源回路](https://www.cqpub.co.jp/trs/trsp170.htm) 「Appendix 1　充放電回数の限界…サイクル耐久特性の実測」 下間 憲行著
3. [電池応用ハンドブック](https://www.cqpub.co.jp/hanbai/books/34/34461.htm)


## ハードウェア

回路図と基板は[PCB_NiMH_Charger_Tester](https://github.com/k-takata/PCB_NiMH_Charger_Tester)で公開しています。

いくつかの部品については、部品の入手状況によってどちらかを選択するようになっています。
例えばUSBコネクターについては、[通常のUSB Type-Cコネクター](https://akizukidenshi.com/catalog/g/g114356/)か、[電源供給用のコネクター](https://akizukidenshi.com/catalog/g/g116438/)を選択できるようになっています。

[AVR64DD28](https://akizukidenshi.com/catalog/g/g118314/)を使って充電・放電制御を行います。

昨年、[IoT環境メーター/スマートリモコン](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)を作った際と同様に、今回も[PCBWay](https://www.pcbway.com/)様からのスポンサーの申し出があったため、PCBWay様に基板を発注しています。


## PCBWayでの発注

PCBWayへの発注方法は、[IoT環境メーター/スマートリモコン](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)に記載した内容から大きな変更はありませんので、基本的には前回の記載内容を参照してください。ここでは、前回からの変更点を中心に説明します。


### ガーバーファイルの作成

KiCadでPCBWay用のガーバーファイルを作成するには、専用のプラグインを使うのが楽です。

前回はKiCad 7.0を使用していましたが、今回はKiCad 9.0を使用したため、プラグインのインストール方法に少しだけ変更がありました。

KiCadのプラグイン＆コンテンツ マネージャーを開いてから、「製造プラグイン」タブに切り替えます。そこで "PCBWay" で検索すると、2つのプラグインが見つかります。

![KiCad plugins](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway2/kicad-plugins.png)

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
通常使用においてはあまり効果はないかもしれませんが、高温環境下で動かす基板を作成したい場合などは嬉しいかもしれません。
[PCBWay Upgrades PCB Material to ShengYi Material - News - PCBway](https://www.pcbway.com/blog/News/PCBWay_Upgrades_Multi_Layers_PCB_Material_to_ShengYi_Material_d4ba6d1c.html)

2. 板材の厚みは最大で8.0mmが選択できるようになっていました。(前回は最大で6.0mm)

3. UV printing Multi-colorというものが選択できるようになっていました。
[Unlock Color PCB Printing with PCBWay! - News - PCBway](https://www.pcbway.com/blog/News/Unlock_Color_PCB_Printing_with_PCBWay_0939d559.html)

4. 「チェックマークは、追加料金なしで当社の裁量で「HASL」を「ENIG」に変更することがあることに同意することを意味します。」のチェックマークが無くなっていました。

なお、JLCPCBではしばらく前に発注番号の削除が無料でできるようになりましたが、PCBWayでは引き続き有料となっていました。



### 配送方法

配送方法は前回から変更は無いようです。

前回同様、担当者にOCSを選択することを強く推奨されました。今回は2 ~ 3営業日で$9.77と表示されました。前回は$13.07でしたので、少し安くなっていました。
OCSよりも安価な業者もいくつかありますが、配送期間とのバランスを考えると推奨通りOCSが一番よさそうです。

![PCBWay shipping](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/shipping.png)
*PCBWayの配送方法*


### 支払い

今回はPCBWay様のご厚意により、基板製作費と送料を負担していただけることになりました。
注文画面からガーバーファイルをアップロードし、レジスト色と注文番号の位置指定のオプションを選んでカートに追加すると、製造番号が発行されましたので、それを担当者に連絡したところ、製作費とOCS前提の送料が自分のアカウントの残高として追加されました。
![message](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/message.png)

支払い時に、配送業者としてOCSを選択し、クーポン選択欄でAccount Balance (アカウント残高)にチェックを入れたところ、無事$0.0で発注ができました。
![payment](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/payment.png)

支払い方法として、PayPalやクレジットカードを使う場合は、PayPal feeやBank feeとして1ドル程度取られるようでした。


なお今回は[PCBWay Ruler](https://www.pcbway.com/project/gifts_detail/PCBWay_Ruler_All_Color.html) 9色セットを購入したいと思って$9.00を支払うつもりでいたところ、こちらも併せてPCBWay様にサポートしていただけることになりました。ありがとうございます。





### 生産追跡

PCBWayも(JLCPCBなどと同様に)生産状況を確認することができます。今回は3/4の昼頃に発注して、3/7の未明に完成しました。黒のレジストを選択したため、元々3 ~ 4日の予定でしたが、思ったより早く完成しました。
![production tracking](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/production-tracking.png)

生産追跡画面で再生ボタンを押すと、各工程のビデオを見ることができるはずなのですが、pcbway.jpではビデオの画面が生産追跡画面の後ろに表示されてしまい、見れませんでした。pcbway.comでは正しく表示されるので、改善をお願いしたいです。


前回は、日本語サイトでは生産追跡画面上のビデオの再生がうまくいかなかったのですが、今回試したところ、問題なく再生できるようになっていました。改善ありがとうございます。




## 基板到着

元々PCBWayでの基板の製造が3 ~ 4日、OCSでの送付に2 ~ 3日の予定でしたが、実際には3/4に製造開始して、3日後の3/7に完了、同日中にOCSに引き渡され、4日後の3/11に一度配達されたものの自分が不在だったために、3/12に再配達となりました。結局、再配達を除けばちょうど1週間で基板を受け取ることができました。

製造開始から5日で受け取ることができました。

前回と同じサイズの箱で届きました。

https://twitter.com/k_takata/status/1767352744569111038


PCBWay Rulerは実際のPCBで使える技術そのままで製造されているので、自分が基板を作成する際の色見本としても便利ですし、PCBの加工精度を確認することもできます。つやあり・つや消しを見比べたり、パターンを眺めているだけでも面白いです。もちろん、名前通り定規として電子部品のサイズを測ったりフットプリントを確認するのにも便利です。



### 受領確認

PCBWayの注文リストを見ると、基板が到着したのに配達状態のままになっており、完了状態に移行しないのを不思議に思っていたのですが、よく見ると「受け確認」と書かれたボタンがあることに気づきました。ボタンを押すと「受け確認?」とだけ聞かれて意味が分かりませんでしたが、どうやらこれが受領確認のボタンだったようです。（英語ページでは "Confirm received" でした。）
ボタンは「受領確認」、質問メッセージは「製品を受け取りましたか？」だったら分かりやすいと思いました。日本語訳の改善に期待したいです。
このボタンを押すと、無事完了状態になりました。

![confirm received](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/confirm-received.png)

さらに、受領確認を行ったことで、[PCBWayリワード画面](https://member.pcbway.jp/specials/rewards)でポイントとBeansが加算されていることが確認できました。
1回の注文ごとに10ポイント、さらに$1の支払いにつき1ポイントで、合計28ポイント加算されていました。


## 組み立て

チップ部品は熱に弱いので、注意して取り付けてください。私は温度センサーICを1つ壊してしまいました。



## 反省点



## 続き

ソフトウェアは現在作成中です。出来上がり次第、続きを公開する予定です。
