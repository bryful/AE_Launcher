# AE_Launcher

C++で作って軽量・高速のAfter effects バージョン選択ランチャーです。<br>
.aepをダブルクリックすると使用するAfter Effectsのバージョンが選べます。<br>
<br>
[https://github.com/bryful/AepSelector](https://github.com/bryful/AepSelector) と同じものです。<br>
こちらはC++でコーディングされていて軽量・高速です。<br>


# 使い方
拡張子登録を行う。<br>

1. 適当な場所にAE_Launcher.exeをおく(C:\bin\AE_Launcher\AE_Launcher.exe を推奨)
2. 拡張子.aepのファイルを右クリックしてプロパティ。
3. プログラム：変更ボタンでAE_Launcher.exeを選択（PCでアプリを選択する）
4. 適用ボタンで終了

これで使えるはずです。おまけでコンソールで

```
AE_Launcher -inst
```
でも拡張子登録できます。<br>
```
AE_Launcher -cs
```
でデスクトップにショートカットを作成できます。<br>
一応**AE_Launcher.cmd**をダブルクリックで上記のコマンドを実行できます。

# 3Pattern あります

## Imgui版
最初に作ったものです。今のところこれが一番実用的です。

## DxLib版
作り途中です。かなり完成した時に他のバターンと勘違いして上書き。Gitの履歴残していなかったので、一番進捗遅れてます。

## win32版
ライブラリを使わずWin32Apiを直接使って実装したものです。これもまだ完成してません。

# 使用ライブラリ

[Dear ImGui](https://github.com/ocornut/imgui)を使用しています。<br>
[DxLib](https://dxlib.xsrv.jp/)を使用しています。<br>


## Dependency
Visual studio 2026 C#<br>
Gemini + Github copilot<br>

## License
This software is released under the MIT License, see LICENSE

## Authors

bry-ful(Hiroshi Furuhashi)<br>
twitter:[bryful](https://twitter.com/bryful)<br>

