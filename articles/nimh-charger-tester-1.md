---
title: "NiMH水素充電器&測定器の作成 (その1)"
emoji: "🔋"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "avr", "pcbway"]
published: false
---

## 概要

以前、[USB NiMH ゆっくり充電器](https://github.com/k-takata/PCB_USB_NiMH_Charger/tree/batt-4)というものを作ってみたのですが、ニッケル水素電池の劣化度合いを定量的に見てみたいと思い、容量や内部抵抗を測定できる充電器を作ってみることにしました。

作成に当たっては、以下の書籍を参考にしました。

1. [トランジスタ技術SPECIAL No.135 Liイオン/鉛/NiMH蓄電池の充電&電源技術](https://shop.cqpub.co.jp/hanbai/books/46/46751.html) 「第4章　研究！ ニッケル水素蓄電池の耐久テスト」
2. [トランジスタ技術SPECIAL No.170 教科書付き 小型バッテリ電源回路](https://www.cqpub.co.jp/trs/trsp170.htm) 「Appendix 1　充放電回数の限界…サイクル耐久特性の実測」
3. [電池応用ハンドブック](https://www.cqpub.co.jp/hanbai/books/34/34461.htm)


## ハードウェア

回路図と基板は[PCB_NiMH_Charger_Tester](https://github.com/k-takata/PCB_NiMH_Charger_Tester)で公開しています。

[AVR64DD28](https://akizukidenshi.com/catalog/g/g118314/)を使って充電・放電制御を行います。

昨年、[IoT環境メーター/スマートリモコン](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)を作った際と同様に、今回も[PCBWay](https://www.pcbway.com/)様からのスポンサーの申し出があったため、PCBWay様に基板を発注しています。


## PCBWayでの発注

PCBWayへの発注方法は、[IoT環境メーター/スマートリモコン](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)に記載した内容から大きな変更はありません。


### ガーバーファイルの作成

KiCadでPCBWay用のガーバーファイルを作成するには、専用のプラグインを使うのが楽です。

前回はKiCad 7.0を使用していましたが、今回はKiCad 9.0を使用したため、プラグインのインストール方法に少しだけ変更がありました。



KiCadのプラグイン＆コンテンツ マネージャーで "PCBWay" で検索すると、2つのプラグインが見つかります。

![KiCad plugins](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/kicad-plugins.png)

* [PCBWay Plug-in for KiCad](https://www.pcbway.com/blog/News/PCBWay_Plug_In_for_KiCad_3ea6219c.html)  
  ボタン一発で、PCBWay向けのガーバーファイルの作成から、ブラウザで注文画面を開いてガーバーファイルのアップロードまでできてしまいます。ただし、注文画面は[英語画面](https://www.pcbway.com/orderonline.aspx)が開くようになっており、[日本語画面](https://www.pcbway.jp/orderonline.aspx)は開けません。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの直下に作成されます。
* [PCBWay Fabrication Toolkit](https://www.pcbway.com/blog/News/PCBWay_Fabrication_Toolkit_for_Kicad_23c41e77.html)  
  ボタン一発でPCBWay向けのガーバーファイルの作成ができます。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの下に `pcbway_production` というディレクトリが作成され、その下に作成されます。

ボタン一発で注文画面まで行ける方がよく、英語も苦にならない場合は、前者がよいでしょう。
一方、発注前にガーバーファイルの内容を確認したい場合や、日本語画面から注文したい場合は後者がよいでしょう。


### 注文画面

前回の注文時からは1か所だけ変更がありました。

FR4-TG(ガラス転移温度)の選択欄の注意を見ると、2層基板の基材は無料で自動的にS1000H TG150にアップグレードされるとの記載がありました。
高温環境下で動かす基板を作成したい場合は嬉しいかもしれません。

なお、JLCPCBではしばらく前に発注番号の削除が無料でできるようになりましたが、PCBWayでは引き続き有料となっていました。



![PCBWay order](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/order.png)
*PCBWayの注文画面*

スタンダード基板の注文画面で選択できる項目について、PCBWayとJLCPCBで違う点を挙げてみました。

* 5枚と10枚で値段が変わらない
  - PCBWay: 5 ~ 10枚で$5.00
  - JLCPCB: 5枚で通常価格$4.00のところ割引で$2.00、10枚で$5.00
* 基板の層の選択肢が多い
  - PCBWay: 1 ~ 14層 (ただし4層以上は追加料金)
  - JLCPCB: 1 ~ 4層 (4層でも50x50mm以内なら2層までと同一価格)
* FR4-TG(ガラス転移温度)の選択ができる
  - PCBWay: TG 130-140, TG 150-160が同一価格で選択可能。追加料金で他も選択可能。
  - JLCPCB: TG 135-140のみ
* PCBの厚さの選択肢が多い
  - PCBWay: 0.2 ~ 3.2mm
  - JLCPCB: 0.4 ~ 2.0mm
* レジストの選択肢が多い
  - PCBWay: 黒(つや消し)、緑(つや消し)が選べる。ただし、つや消しと紫は追加料金。
  - JLCPCB: 紫を含め、同一料金。
* シルクの選択肢が多い
  - PCBWay: 白、黒、黄色、なしから選択可能。標準以外の組み合わせは追加料金。
  - JLCPCB: レジストの色により白か黒か自動決定。
* 表面処理の選択肢が多い
  - PCBWay: 10種類以上。PCBWayの裁量でHASLをENIGに変更する可能性があることに同意というチェックがある。
  - JLCPCB: HASL、無鉛HASL、ENIG
* 銅箔の選択肢が多い
  - PCBWay: 1 ~ 13 oz
  - JLCPCB: 1 ~ 2 oz
* 発注番号の位置指定
  - PCBWay: `WayWayWay`
  - JLCPCB: `JLCJLCJLCJLC`

全般的に、スタンダード基板で選択できる幅はPCBWayの方が多いです。価格はオプションにもよりますが、ほぼ同じか若干JCLPCBの方が安いように見えます。

注文画面の操作性の違いについては以下の点が挙げられます。

* リセットボタンがある  
  JLCPCBとは異なり、リセットボタンがあるので設定を最初からやり直したいときに便利です。
* ガーバーファイルのアップロード方法  
  JLCPCBとは異なり、「クイックオーダー基板」のリンクをクリックすることでガーバーファイルがアップロードできるようになり、一手間多いです。



### 送料

JLCPCBでは、OCS NEPを選択すると1ドル程度、OCS Expressを選択すると2ドル程度(どちらも6 ~ 8営業日)になりますが、PCBWayには同様のOCSの格安サービスはないようです。

![JLCPCB shipping](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/shipping-jlcpcb.png)
*JLCPCBの配送方法*

一方PCBWayでは、担当者にOCSを選択することを強く推奨されました。今回は2 ~ 3営業日で$13.04と表示されました。OCSよりも安価な業者もいくつかありますが、配送期間とのバランスを考えると推奨通りOCSが一番よさそうです。

![PCBWay shipping](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/shipping.png)
*PCBWayの配送方法*


### 支払い

今回はPCBWay様のご厚意により、基板製作費と送料を負担していただけることになりました。
注文画面からガーバーファイルをアップロードし、レジスト色と注文番号の位置指定のオプションを選んでカートに追加すると、製造番号が発行されましたので、それを担当者に連絡したところ、製作費とOCS前提の送料が自分のアカウントの残高として追加されました。
![message](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/message.png)

支払い時に、配送業者としてOCSを選択し、クーポン選択欄でAccount Balance (アカウント残高)にチェックを入れたところ、無事$0.0で発注ができました。
![payment](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/payment.png)

支払い方法として、PayPalやクレジットカードを使う場合は、PayPal feeやBank feeとして1ドル程度取られるようでした。


### 生産追跡

PCBWayも(JLCPCBなどと同様に)生産状況を確認することができます。今回は3/4の昼頃に発注して、3/7の未明に完成しました。黒のレジストを選択したため、元々3 ~ 4日の予定でしたが、思ったより早く完成しました。
![production tracking](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/production-tracking.png)

生産追跡画面で再生ボタンを押すと、各工程のビデオを見ることができるはずなのですが、pcbway.jpではビデオの画面が生産追跡画面の後ろに表示されてしまい、見れませんでした。pcbway.comでは正しく表示されるので、改善をお願いしたいです。




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


## 続き

ソフトウェアは現在作成中です。出来上がり次第、続きを公開する予定です。
