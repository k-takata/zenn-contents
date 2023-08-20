---
title: "イントロダクション"
---

## はじまり

2023年7月某日、秋月電子の2階の新規オープンセールでCITRUS&MIKANフルフルセットという物を見つけて、何に使えるか分からないまま試しに買ってみました。
@[tweet](https://twitter.com/k_takata/status/1675070606075895808)


## 概要

[GR-CITRUS](https://www.renesas.com/jp/ja/products/gadget-renesas/boards/gr-citrus)は[がじぇっとるねさす](https://www.renesas.com/jp/ja/products/gadget-renesas)で企画された製品の1つで、Renesasの[RX631](https://www.renesas.com/jp/ja/products/microcontrollers-microprocessors/rx-32-bit-performance-efficiency-mcus/rx631-32-bit-microcontrollers-enhanced-security-image-capture)を搭載し、組み込み向けのRubyである[mruby](https://github.com/mruby/mruby)を簡単に動かせる小型マイコンボードです。拡張ボードのWA-MIKANと組み合わせて使うことで、Wi-FiやマイクロSDカードも使うことができます。前述のCITRUS&MIKANフルフルセットは、このGR-CITRUSとWA-MIKANのフル版(コネクタ有り版)がセットになった物でした。
2016年の発売で、現在は生産終了になっていますが、7年経って在庫が放出されることになったようです。

2023年8月時点では、CITRUS&MIKANフルフルセットは売り切れてしまいましたが、コネクタ無しのノーマル版の単体販売はまだ継続されているようです。

* [GR-CITRUS-NORMAL](https://akizukidenshi.com/catalog/g/gK-11217/)
* [WA-MIKAN-NORMAL](https://akizukidenshi.com/catalog/g/gK-11218/)

GR-CITRUSは発売から7年も経っていることもあり、ネット上の情報にはそのまま使えないものも多くなっています。2023年時点の情報をまとめていきたいと思います。


## 参考サイト

* [公式サイト](https://www.renesas.com/jp/ja/products/gadget-renesas/boards/gr-citrus)
* <https://github.com/wakayamarb/wrbb-v2lib-firm>
  GR-CITRUSの説明書とRubyファームウェアのソースコードがあります。
* <https://github.com/wakayamarb/wa-mikan>
  WA-MIKANの説明書と回路図があります。
* [tarosay - Qiita](https://qiita.com/tarosay)
  GR-CITRUS & WA-MIKAN の開発者であるtarosay氏のQiitaページです。GR-CITRUSとWA-MIKANに関する記事があります。
* [GR-CITRUS カテゴリーの記事一覧 - コンピュータを楽しもう！！](https://tarosay.hatenablog.com/archive/category/GR-CITRUS)
  GR-CITRUS & WA-MIKAN の開発者であるtarosay氏のHatena Blogページです。
