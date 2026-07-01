---
title: "Vimのインストーラー用CIを作り直してみた"
emoji: "🕰️"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["Vim", "CI", "GitHub Actions"]
published: false
---

## 概要

現在、VimのWindows版インストーラーは、[vim-win32-installer](https://github.com/vim/vim-win32-installer)のCIで自動生成されています。
今回、このCIをAppVeyorからGitHub Actionsに移行しましたので、その際に行ったことを備忘録として残しておきます。

関連記事として、[Vimのインストーラーを作り直してみた](https://zenn.dev/k_takata/articles/vim-rework-installer)という記事も公開していますので、こちらも併せてご覧ください。


## 歴史

vim-win32-installerは2016年に提供を開始しました。当時は、AppVeyorがWindowsを使えるほぼ唯一のCIサービスだったため、必然的にWindows版インストーラーの作成にはAppVeyorを使うことになりました。しかしそれから10年が経ち、状況は大きく変化しました。

2019年にGitHub自体がCIサービス(GitHub Actions)を提供し、しかもWindowsランナーも使えます。GitHubが提供していることから、GitHubリポジトリとの親和性も高く、AppVeyorよりも使いやすいです。さらに速度の点でもGitHub Actionsの方が有利です。AppVeyorの無料プランではジョブの並列実行はできませんが、GitHub Actionsではユーザーあるいは組織当たり20ジョブまでの並列実行が可能です。

明らかにAppVeyorよりGitHub Actionsの方がよさそうに見えるわけですが、すぐにGitHub Actionsに移行できるかというと、そうではありませんでした。vim-win32-installerでは、2019年から実行ファイルの署名に[SignPath](https://signpath.org/)を使い始めましたが、当時はGitHub ActionsからSignPathを使うことはできなかったためです。
しかし2年ほど前から[公式のaction](https://github.com/SignPath/github-action-submit-signing-request)が提供されるようになり、GitHub Actionsからも簡単に使えるようになりました。1年前にARM64版の要望を受けて[chrisbra](https://github.com/chrisbra)氏が対応した際に、ARM64版はGitHub Actionsでビルドするように実装され、SignPathもGitHub Actionsから呼び出しています。

このような形で、GitHub Actionsへ移行できる環境が整ってきました。
また、私自身は2019年から[vim-kt](https://github.com/k-takata/vim-kt)という独自ビルドのVimパッケージを、GitHub Actionsを使って公開していました。
そこで、vim-ktをベースに、vim-win32-installerのCIをGitHub Actionsに移行することにしました。


## CIの構成

### 移行前

GitHub Actions移行前のCIは以下のような構成になっていました。

1. GitHub Actionsのスケジュールビルドで、[vim/vim](https://github.com/vim/vim)の最新コミットを取得し、その情報を[vim/vim-win32-installer](https://github.com/vim/vim-win32-installer)にプッシュ。
2. それを契機にAppVeyorで、x64版とx86版のビルド・テスト・パッケージ作成を実行。SignPathを呼び出し。新しいリリースを作成。(1時間程度)
3. 新しいリリースの作成を契機に、GitHub ActionsでARM版のビルド・パッケージ作成を実行。SignPathを呼び出し。Attestation (アテステーション、構成証明) 情報の登録。リリースを更新。(10分程度)

トータルの実行時間は1時間10分程度も掛かるうえ、後から処理を追加したことでGitHub ActionsとAppVeyorを行ったり来たりする複雑な構成になっていました。


### 移行後

GitHub Actions移行後は、前述のCIを1つのワークフローに統合し、以下のような構成にしました。

1. GitHub Actionsのスケジュールビルドで、ワークフローを実行。最初に[vim/vim](https://github.com/vim/vim)の最新コミット情報を取得。
2. x64/x86/ARM64版のビルド・テスト・パッケージ作成を並列実行。
3. Attestation情報の登録、SignPathを呼び出し、リリースを作成。

移行前は、ARM64版のテストは省略していましたが、移行後は省略せずにテストしています。
さらに、移行前はgvim (GUI版)のテストとvim (Console版)のテストは順次実行していましたが、こちらも並列実行することで実行時間を短縮しました。
結果的に、トータルの実行時間は20 ~ 25分程度 (元の約1/3) に短縮されました。

今回の実装は主に以下のPRで行いました。(実際には、この後多数のPRで追加修正を行っています。)
https://github.com/vim/vim-win32-installer/pull/421


## 詳細機能

### SignPath

前述の通り、実行ファイルのコード署名にはSignPathを利用していましたが、GitHub Actionsへの移行で、SignPathの公式actionを利用する形にしました。

従来は、SignPathに署名のリクエストを出すまでは自動でしたが、署名されたファイルをダウンロードしてそれをリリースに登録する作業は手動でした。

今回はその処理を半自動化し、署名されたファイルを数クリックでリリースに登録できるようにしました。
完全自動になっていないのは、SignPathでコード署名する際に手動の承認が必須となっているため[^1]です。SignPathで手動承認した後、署名されたファイルを登録するためのワークフローを手動実行すると、ファイルのダウンロード、Attestation情報の登録、リリースへの登録が自動で行われます。
これにより、署名済みパッケージのリリース作業が大幅に簡略化されました。

[^1]: SignPath自体は全自動でのコード署名もできるようですが、現状、vim-win32-installerの署名は手動承認が必須です。

署名の完全自動化ができるのであれば以下のようにしたいところですが、今後の課題としておきます[^2]。

1. SignPathでZIPパッケージ内の実行ファイルの署名を実行
2. それをダウンロードして、NSISインストーラーを作成
3. SignPathでNSISインストーラーの実行ファイルの署名を実行
4. それをダウンロードしてリリースにアップロード

[^2]: 現在のNSISインストーラーのパッケージは、インストーラーの実行ファイルには署名されていますが、その中に含まれる実行ファイルには署名がされていません。


### Attestation (構成証明)

GitHubには[Attestation](https://docs.github.com/ja/actions/concepts/security/artifact-attestations)という機能があります。
[GitHub CLI](https://docs.github.com/ja/github-cli/github-cli)の `gh attestation verify` コマンドを使うことで、入手したファイルが本当にその組織あるいはリポジトリで作成されたものかどうかを確認することができます。

例えば、[gvim_9.2.0712_x64_signed.exe](https://github.com/vim/vim-win32-installer/releases/download/v9.2.0712/gvim_9.2.0712_x64_signed.exe)をカレントディレクトリにダウンロードして、以下のコマンドを実行します。

```
$ gh attestation verify --owner vim gvim_9.2.0712_x64_signed.exe
Loaded digest sha256:893a30d60b53a905d12ad7bc3b4427bffe6caa1ff50f505f3a4ab0e29e381d59 for file://gvim_9.2.0712_x64_signed.exe
Loaded 1 attestation from GitHub API

The following policy criteria will be enforced:
- Predicate type must match:................ https://slsa.dev/provenance/v1
- Source Repository Owner URI must match:... https://github.com/vim
- Subject Alternative Name must match regex: (?i)^https://github.com/vim/
- OIDC Issuer must match:................... https://token.actions.githubusercontent.com

✓ Verification succeeded!

The following 1 attestation matched the policy criteria

- Attestation #1
  - Build repo:..... vim/vim-win32-installer
  - Build workflow:. .github/workflows/upload-signed-files.yml@refs/heads/master
  - Signer repo:.... vim/vim-win32-installer
  - Signer workflow: .github/workflows/upload-signed-files.yml@refs/heads/master
```

`✓ Verification succeeded!` と表示されており、Build repoとして `vim/vim-win32-installer` が表示されていることから、これが確かに `vim/vim-win32-installer` リポジトリでビルドされたファイルであることが分かります。

GitHub ActionsからAttestation情報を登録するには、[actions/attest](https://github.com/actions/attest)を使用します[^3]。

[^3]: 移行前は、ARM64版のビルドで[actions/attest-build-provenance](https://github.com/actions/attest-build-provenance)を使用していましたが、現在はactions/attestの方が推奨されているので、こちらを使うように変更しています。


### Stepの並列実行

テスト時間の短縮のため、gvimとvimのテストを並列実行するように実装したのですが、当初、gvimのテストをフォアグラウンドで動かしている間に、vimのテストをバックグラウンドで動かすように実装していました。
ところが先日、GitHubから次のような新機能が発表されました。

[Actions steps can now be run in parallel - GitHub Changelog](https://github.blog/changelog/2026-06-25-actions-steps-can-now-be-run-in-parallel/)

早速、この機能を使ってテストを書き直してみました。これを使うことでGitHubの画面上から、gvim/vim双方のテスト状況を確認できるようになり、また自前のwait処理などが不要となったため、かなり使いやすくなりました。

`steps:` の下に `- parallel:` を書き、その下にステップを記述すれば、それらを並列実行してくれます。

```yaml
    steps:
    - parallel:
      # Run gvim tests and vim tests in parallel

      - name: Test gvim

      - name: Test vim
```

![ステップの並列実行](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/gha-parallel.png)


## トラブル集

今回の実装で、いくつか苦労した点があったので、メモとして残しておきます。


### jobのpermissions

job内でpermissionsを設定すると、workflowで設定した権限は引き継がれません。

次のようなyamlファイルを考えてみます。workflow全体で `contents: read` パーミッションを設定していますので、`job1` では権限が引き継がれて `contents: read` が設定されます。
`job2` では独自に `actions: write` パーミッションを設定していますが、workflowの権限はクリアされるので、`contents: read` パーミッションは設定されません。

```yaml
permissions:
  contents: read

jobs:
  job1:
    # contents: read は有効

  job2:
    permissions:
      actions: write
    # contents: read は無効
```


### Attestationの権限

PR (forkしたリポジトリ)ではAttestation用の権限はありません。
Attestationはビルドされたファイルの出所を証明するための仕組みですので、第三者が作ったPRからAttestation情報を登録できないのは当然と言えます。

自分の場合、最初はvim/vim-win32-installerリポジトリ内で作業していたのですが、途中から自分のforkからPRを出して作業するように切り替えたところ、actions/attestからエラーが出るようになり、原因究明に時間を取られました。
GitHub Copilotの[Explain error機能](https://docs.github.com/ja/enterprise-cloud@latest/actions/how-tos/troubleshoot-workflows#github-copilot-%E3%81%AE%E4%BD%BF%E7%94%A8)も試してみたのですが、権限が足りないので権限を追加する必要があるとの回答で、forkしたリポジトリからのPRではそもそも権限が取得できない点はCopilotは指摘できませんでした。

今回は、PRではAttestation情報を登録しないように変更して解決しました。


### 一部のテストがfailする

AppVeyorとGitHub Actionsの環境の違いのためか、一部のテストが不安定で頻繁にfailするという問題が発生しました。
さらに、今まで省略していたARM64版のテストを実行するようにしたところ、x86/x64版では動作しているテストがfailするという問題が発生しました。

原因調査の時間が取れなかったため、一時的にそれらのテストをスキップすることにしました。

```yaml
  # Skip pattern
  # FIXME: Some tests are flaky and often fail.
  TEST_SKIP_PAT: 'Test_terminal_composing_unicode\|Test_terminal_version'
  TEST_SKIP_PAT_ARM64: 'Test_terminal_composing_unicode\|Test_terminal_version\|Test_gui_lowlevel_keyevent\|Test_QWERTY_Ctrl_minus\|Test_mswin_event_character_keys\|Test_mswin_event_function_keys\|Test_mswin_event_movement_keys'
```


### GITHUB_TOKENを使ってworkflowを実行

次のようなワークフローがあるとします。

1. ワークフローA
   メインのワークフロー
2. ワークフローB
   `workflow_dispatch` を設定して、手動実行できるようにしておく。
3. ワークフローC
   `workflow_run` を設定して、ワークフローBの実行が終わったら実行されるようにしておく。

ここで、次のようにワークフローAからBを実行することを考えます。

```yaml
    - shell: bash
      env:
        GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
      run: |
        gh workflow run workflow_b.yml
```

環境変数 `GH_TOKEN` に `GITHUB_TOKEN` を設定して `gh workflow run` コマンドを実行すれば、ワークフローBは実行されますが、ワークフローCは実行されません。
これについては、[ワークフローをトリガーする - GitHubドキュメント](https://docs.github.com/ja/actions/how-tos/write-workflows/choose-when-workflows-run/trigger-a-workflow)に次の記載があります。

> リポジトリの `GITHUB_TOKEN` を使用してタスクを実行する場合、 `GITHUB_TOKEN` によってトリガーされるイベントは新しいワークフロー実行を作成しません。ただし、次の例外があります。
>
> * `workflow_dispatch` と `repository_dispatch` イベントでは、常にワークフロー実行が作成されます。
> * (略)

ワークフローBは `workflow_dispatch` イベントを使っているため、`GITHUB_TOKEN` を使ってワークフローを実行することができます。しかし、`workflow_run` イベントは `GITHUB_TOKEN` の例外には含まれないため、ワークフローCは実行されません。

ワークフローCも実行されるようにするには、`GITHUB_TOKEN` 以外のトークンを使用する必要があります。
今回はGitHub appを新規に登録し、[actions/create-github-app-token](https://github.com/actions/create-github-app-token)でトークンを発行して使用するようにしました。


### バッチファイルの罠1

GitHub Actionsで `shell: cmd` とした場合、そのステップのコマンドはバッチファイルとして実行されます。そのため、そこから別のバッチファイルを呼ぶには `call` コマンドが必要です。

例えば、あるステップから一時的に[msys2/setup-msys2](https://github.com/msys2/setup-msys2)の `msys2` コマンドを介してシェルスクリプトを実行する例を考えてみます。

```yaml
jobs:
  job1:
    steps:
    - uses: msys2/setup-msys2@v2

    - shell: cmd
      run: |
        call msys2 do-something.sh    # msys2はバッチファイルなのでcallが必須
        echo msys2 実行完了           # ← callを忘れると実行されない
```

`msys2` コマンドは実際にはバッチファイルなので、`call` コマンドを忘れると、それ以降のコマンドがスキップされてしまいます。


### バッチファイルの罠2

GitHub Actionsで環境変数を次のステップに渡すには、環境変数 `GITHUB_ENV` が指し示すファイルに `変数名=値` という形式で行を追加します[^4]。

[^4]: <https://docs.github.com/ja/actions/reference/workflows-and-actions/workflow-commands#setting-an-environment-variable>

bashであれば、素直に

```bash
echo "VARIABLE_NAME=value" >> "$GITHUB_ENV"
```

と書けばいいのですが、`shell: cmd` の場合は複数の罠があります。

cmdのechoコマンドは、`"` をそのまま出力してしまいます。
リダイレクトの `>>` の前のスペースもそのまま出力してしまいます。
環境変数の参照は `$GITHUB_ENV` から `%GITHUB_ENV%` に変える必要があります。

そこで、以下のようにすると一見うまくいくように見えます。

```batch
echo VARIABLE_NAME=value>> "%GITHUB_ENV%"
```

しかし、次のような場合に問題が発生してしまいます。

```batch
set VALUE=0
echo VARIABLE_NAME=%VALUE%>> "%GITHUB_ENV%"
```

`VALUE` が `0` なので、これは次のように展開されます。

```batch
echo VARIABLE_NAME=0>> "%GITHUB_ENV%"
```

困ったことに、`>>` の直前の数字はファイルディスクリプタとして解釈されてしまいます。

```batch
echo VARIABLE_NAME= 0>> "%GITHUB_ENV%"
```

ファイルディスクリプタ0 (=標準入力) の内容が `%GITHUB_ENV%` に追記されますが、結果的に何も起きず、`VARIABLE_NAME` を設定したつもりが設定されません。

これを防ぐには、echoコマンドを括弧でくくる必要があります。

```batch
(echo VARIABLE_NAME=%VALUE%) >> "%GITHUB_ENV%"
```

あるいは、少しわかりにくいですが、リダイレクトを最初に持ってきてもよいです。

```batch
>> "%GITHUB_ENV%" echo VARIABLE_NAME=%VALUE%
```

こちらも残念ながらGitHub CopilotのExplain error機能では解決できませんでした。


### バッチファイルの罠3

バッチファイルは、行末に `^` を置くことで行継続ができます。しかし好きなように行を分割できるわけではありません。

例えば、次のような行を分割することを考えます。

```batch
echo aaa | findstr "aaa"
```

パイプの後で分割して、

```batch
echo aaa | ^
  findstr "aaa"
```

とすると、コマンドが見つからないとのエラーが出て動きません。(行頭のスペースがコマンド扱いされてしまうようです。)

パイプの前で分割して、

```batch
echo aaa ^
  | findstr "aaa"
```

とするなら動きます。

あるいは、パイプの後で分割しても、行頭にスペースを入れなければ動きます。

```batch
echo aaa | ^
findstr "aaa"
```

バッチファイルにはこのような奇妙な罠がたくさんあるので、特別な理由がない限り、pwshやbashを使うのがよいでしょう[^5]。

[^5]: 自分自身はpwshよりcmdの方が慣れているので、ついcmdを使ってしまいますが。


### robocopyコマンドの罠

Windowsでは、ディレクトリや複数ファイルのコピーにrobocopyコマンドを使うことができます。
robocopyコマンドは戻り値が特殊で、エラーの時は8以上の値を返し、成功の時は8未満の値を返します[^6]。普通のコマンドと同じつもりで使うと、コピーが成功してもエラーになってしまうので注意が必要です。

[^6]: [Robocopy ユーティリティで使用されるリターン コード - Windows Server | Microsoft Learn](https://learn.microsoft.com/ja-jp/troubleshoot/windows-server/backup-and-storage/return-codes-used-robocopy-utility)

`shell: cmd` で使う場合は、例えば次のようにします。

```batch
robocopy src dest /E /NP /NFL /NDL
if ERRORLEVEL 8 exit 1
rem コピー成功、残りの処理を実行
```

`robocopy`がステップ内の最後のコマンドの場合、そのまま抜けるとエラーになってしまうので、`exit 0`が必要です。

```batch
robocopy src dest /E /NP /NFL /NDL
if ERRORLEVEL 8 exit 1
exit 0
```


### PRのマージ

移行前は、PRを作成しても、変更した処理をテストするためのCIが流れるようにはなっていませんでした。そのため、PRを作成する前にテスト専用のブランチを作成して動作を確認したり、あるいは動作確認しないままマージしたりしていました。

移行後は、PRを作成したら、ビルド・テスト・パッケージの作成までをテストするようにしたため、処理変更時の動作確認が大幅に楽になりました。[PRの自動マージ](https://docs.github.com/ja/pull-requests/collaborating-with-pull-requests/incorporating-changes-from-a-pull-request/automatically-merging-a-pull-request)も有効にしたことで、マージも楽になりました。

しかし、パッケージのリリース処理は、masterブランチにマージして実際にリリースしてみないとテストできず、無駄にPRをいくつも作る羽目になってしまいました。一部の処理はforkしたリポジトリでリリースのテストを行ったりもしましたが、SignPath周りの処理については、forkしたリポジトリでテストすることもできず、苦労しました。リリース処理のテストは今後の課題です。


## まとめ

vim-win32-installerのCIを、AppVeyorからGitHub Actionsに移行した際の変更内容やトラブルを記載しました。
何かの参考になれば幸いです。
