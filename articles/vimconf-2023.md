---
title: "VimConf 2023 Tiny に参加してきた & LT資料"
emoji: "⌨️"
type: "idea" # tech: 技術記事 / idea: アイデア
topics: ["Vim"]
published: true
---

先月18日、4年ぶりに開催されたVimConf 2023 Tinyに参加してきました。

https://vimconf.org/2023/

コロナのために準備期間が取れなかったこともあり、今回は午前の部やLT (Lightning Talk)のない縮小版として開催されました。


## 開場

Tinyということで、今回はいつもの5階の[アキバホール](https://www.fsi.co.jp/akibaplaza/hall.html)ではなく、少し小さい6階の[セミナールーム1](https://www.fsi.co.jp/akibaplaza/seminar01.html)での開催でした。

びっくりしたのは、受付に[VimGirl](http://vim-jp.org/docs/vac2012.html)のコスプレしてる人がいたということ。


## セッション

まずはmattnさんで、Bram Moolenaar氏がどのようにVimを開発してきたかという話。
一部は[Software Design 2023年11月号](https://gihyo.jp/magazine/SD/archive/2023/202311)の「[追悼 Bram Moolenaar ～Vimへの情熱と貢献を振り返る](https://gihyo.jp/article/2023/11/memorial-to-bram-moolenaar)」でも明らかにされている内容でしたが、心に来るエピソードを直接聞けたのは良かったです。

2人目はΛlisueさんで、Denoを使ったVim/Neovimプラグイン環境である[Denops](https://github.com/vim-denops/denops.vim)について。Denopsを使ってプラグインを書くための基礎的な概念から説明されていました。

3人目はゴリラさんで、[ゴリラ.vim](https://gorillavim.connpass.com/)を運営してきた話。ゴリラ.vimの開催はもう少しで30回とのことで、これだけの回数に渡って継続してきたのは素晴らしいことです。

4人目は大倉雅史さんで、Rubyを使ってVimのプラグインを書く方法。if_rubyを使ってVimのプラグインを書いている人は会場での挙手でも圧倒的少数派でしたが、Rubyに関連するプラグインを書くならif_rubyを使うのが良いようです。

5人目はkuuさんで、[skkeleton](https://github.com/vim-skk/skkeleton)についての話。SKKをVim上に実装するにはいろいろ苦労する点があったようです。

最後はaiya000さんで、vimrcを書くときに便利なテクニックの紹介。最近のVimで使えるようになった新しいVim scriptの文法やvital.vimの便利機能を紹介されていました。

そういえば今回は、終了後の全員集合写真を撮ってなかったですね。


## 懇親会 & LT

今年は本会場でのLTはありませんでしたが、例年通り懇親会会場でのLTは開催されました。

LTで何か発表してほしいと頼まれたため、急遽その場でスライドを作成して、今後のVim開発体制について発表しました。
今後の開発体制については、mattnさんのセッションでも軽く触れていましたが、もう少し踏み込んで、開発メンバー、今までと変わった点、変わっていない点、Vim 9.1に向けた予定などについて説明してみました。

スライド: [今後のVim開発体制](https://github.com/k-takata/zenn-contents/tree/master/articles/files/vimconf2023_takata_lt.pdf)

運営の方々は、LTで発表する人が集まるか心配していたようですが、最終的には本編よりもたくさんの方が発表していました。中には、VimConfがきっかけで文字通り人生が変わったという発表をされた方もおられて、とても感慨深かったです。

懇親会ではVimGirlのコスプレをしていたつぴさんとも少しお話しできました。Vimはずいぶん前から使っていたけど、Bramの訃報を受けてVimGirlのコスプレをしたとのことでした。[雑誌にも掲載](https://twitter.com/tsupi_cos/status/1710263505381953596)されて、その際、作品名を書く欄に作品とは違うと思いながらVimと書いたという話が面白かったです。

また、vim-jp slackでいつも会話している方々ともお話しできてとても良かったです。来年のVimConfも楽しみです。
