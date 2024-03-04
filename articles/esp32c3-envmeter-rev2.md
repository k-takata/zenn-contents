---
title: "ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る"
emoji: "🌡"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32", "esp32c3", "bme680"]
published: false
---

## 概要

前回作成した[IoT環境メーター](https://zenn.dev/k_takata/articles/esp32c3-envmeter)を改良し、Rev. 2としてスマートリモコン機能を追加しました。
スマートリモコン機能は、以下の記事で紹介した内容をWA-MIKAN (ESP8266)からESP32-C3に移植したものとなっており、Slack経由でエアコンを制御できるというものです。

* [WA-MIKANを単体で使う（その1）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/10_wa_mikan_only1)
* [WA-MIKANを単体で使う（その2）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/11_wa_mikan_only2)


## ハードウェア

前回同様に、[PCB_envmeter_esp32c3](https://github.com/k-takata/PCB_envmeter_esp32c3)で回路図と基板を公開しています。

前回のRev. 1と異なる点は、赤外線送受信部が追加されていることです。
また、今回はPCBWay様からのスポンサーの連絡があったため、PCBWay様に基板を発注しています。


## PCBWayでの発注

今まで基板はJLCPCBに発注していましたが、今回初めてPCBWayに発注することになりましたので、いろいろと比較してみたいと思います。

### ガーバーファイルの作成

KiCadのプラグイン＆コンテンツ マネージャーで "PCBWay" で検索すると、2つのプラグインが見つかります。

* PCBWay Plug-in for KiCad  
  ボタン一発で、PCBWay向けのガーバーファイルの作成から、ブラウザで注文画面を開いてガーバーファイルのアップロードまでできてしまいます。ただし、注文画面は英語画面の [pcbway.com](https://www.pcbway.com/orderonline.aspx) が開くようになっており、日本語画面の [pcbway.jp](https://www.pcbway.jp/orderonline.aspx) は開けません。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの直下に作成されます。
* PCBWay Fabrication Toolkit  
  ボタン一発でPCBWay向けのガーバーファイルの作成ができます。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの下に `pcbway_production` というディレクトリが作成され、その下に作成されます。

ボタン一発で注文画面まで行きたく英語も苦にならない場合は、前者がよいでしょう。
一方、発注前にガーバーファイルの内容を確認したい場合や、日本語画面から注文したい場合は後者がよいでしょう。


### 注文画面

スタンダード基板の注文画面について、PCBWayとJLCPCBで違う点を挙げてみました。

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


### 送料

JLCPCBでは、OCS NEPを選択すると1ドル程度、OCS Expressを選択すると2ドル程度の格安送料(6 ~ 8営業日)になりますが、PCBWayにはそのようなものはないようです。

PCBWayでは、担当者にOCSを選択することを推奨されました。今回は2 ~ 3営業日で$13.04と表示されました。OCSよりも安価な業者もいくつかありますが、配送期間とのバランスを考えると推奨通りOCSが一番よさそうです。


### 支払い

今回はPCBWay様のご厚意により、基板製作費と送料を負担していただけることになりました。
注文画面からガーバーファイルをアップロードし、レジスト色と注文番号の位置指定のオプションを選んでカートに追加すると、製造番号が発行されましたので、それを担当者に連絡したところ、製作費とOCS前提の送料が自分のアカウントの残高として追加されました。

支払い時に、配送業者としてOCSを選択し、クーポン選択欄でAccount Balance (アカウント残高)にチェックを入れたところ、無事$0.0で発注ができました。

支払い方法として、PayPalやクレジットカードを使う場合は、PayPal feeやBank feeとして1ドル程度取られるようでした。


### 共有プロジェクト

PCBWayで面白いと思ったサービスが、[共有プロジェクト](https://www.pcbway.com/project/shareproject/)です。
自分の作成した基板をオープンソースハードウェアとして公開する場合、それをPCBWayの共有プロジェクトとして登録することができます。あるいは、他の人が設計した基板で気に入ったものがあれば、それを発注することができます。
誰かが自分のプロジェクトを気に入って基板を発注した場合、その金額の10%が自分に支払われるとのことです。

[Edit Your Project - PCBWay Community](https://www.pcbway.com/project/shareproject/techshare.aspx)


### ポイント

もう一つ面白いと思ったのが、[PCBWayリワード](https://member.pcbway.jp/specials/rewards)という仕組みです。
PCBWayポイントを貯めることで、1 ~ 5%の割引などの特典を受け取ることができるようです。
また、PCBWay Beansを貯めることで、ギフトやクーポンを引き換えることができるようです。

