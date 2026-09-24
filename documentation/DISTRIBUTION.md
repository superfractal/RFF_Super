# RFF_Super 配布準備

Modified by GPT-6 on 2026-09-24, 2026-09-25.

## GitHubリポジトリ内で公開する資料

第三者ライセンス原文と対応ソース・パッチは、Git除外の `build/` から
[third-party/](../third-party/README.md) へ移動しています。このフォルダーは本体の
`LICENSE`・`NOTICE` と合わせてコミット・pushしてください。
ソースパッケージ23個を収録し、大きな5個は24 MiB以下に分割しています。
全パーツと復元スクリプトを公開することで、利用者は外部ダウンロードなしで復元できます。
今回、全23個の復元と元のSHA-256との一致を確認しています。

これは第三者資料の公開準備です。現在の本体EXEと本体ソースの対応を確定する作業は別途必要です。
以下の `build/` 内での候補作成手順は、依存更新時の再収集・再確認に使用できます。

## 今回の構成

RFF_Super は GPLv3 のままです。商用利用・販売も許可されます。
静的リンクに変更しても、ライセンス表示や対応するソースの提供が不要になるわけではありません。

| 対象 | 配布構成 | 根拠・扱い |
| --- | --- | --- |
| RFF_Super | EXE | GPLv3。ビルドしたタグのソース・ビルド手順をセットで提供 |
| GMP 6.3.0 | 静的リンク | GPLv2以降の選択肢からGPLv3を選択。同版のソースとMSYS2パッチ・ビルド条件を提供。GMP自体の上流の選択肢は維持 |
| OpenCV core/imgproc/imgcodecs | DLL | Apache-2.0と同梱部分の条件。画像形式の対応を維持 |
| zstd、画像コーデック、数値ライブラリ | DLL | EXEから再帰的に必要なDLLだけを選択。版・ハッシュ・原文を記録 |
| GCC runtime、libgfortran | DLL | GPLv3以降とGCC Runtime Library Exception。DLL自体の再配布義務も確認 |
| libquadmath | DLL | LGPLv2.1以降。GCC例外と一括扱いしない |
| MinGW runtime、GLM、stb_image、nlohmann/json | 静的・ヘッダー | 各著作権表示と条件を保持 |
| Vulkan loader/driver | 利用者のGPUドライバー | 作業用binからコピーしない |
| FFmpeg、ローカルAIサーバー・モデル | 利用者が別途用意 | この配布候補には含めない |
| FlyWire、全脳モデル、PyArrow | 削除 | ソース、UI、導入ツール、専用モデルフォルダーを削除 |

確認した構成では、本体1個と必要なDLL31個になります。作業用 `bin/` の多数の
開発ツール・別用途のDLLは配布対象にしません。OpenCVの画像読み込みを維持するため、
そのコーデック依存を単に削除してDLL数だけを減らすことはしていません。
正確な版は `tools/release-license-review.json`、個々のファイルは生成する
`DEPENDENCIES.json` に記録します。依存の版が変わると梱包ツールは停止します。

## 原文で再確認した注意点

* JBIG-KIT のMSYS2メタデータは `GPL-2.0` と省略されていますが、取得した
  2.1の `libjbig/jbig.c` は「version 2 ... or ... any later version」と明記します。
  GPLv2限定と誤認せず、GPLv3を選択できることを確認しています。
* XZパッケージ全体の表示にはGPL/LGPLも含まれますが、`COPYING` は
  配布対象のliblzmaを0BSDと説明しています。xzのコマンド類は同梱しません。
* OpenCVの主ライセンス、GMPのCOPYING類は、インストール済みライセンスフォルダー
  だけでは不足します。対応するソースアーカイブ内の原文も同梱します。
* JPEGの文書用表示：This software is based in part on the work of the Independent JPEG Group.
* OpenStaxのCC BY-NC-SA 4.0は教科書コンテンツの条件です。該当シェーダーは物理式の
  独立実装で、教科書本文・図・サンプルコードの収録は確認されていません。
  出典はNOTICEに残しています。Lustreの元HTMLは所有者の依頼によるAI生成との申告を記録しています。

確認は二段階です。まず実ファイルとインストール済みパッケージのハッシュを照合し、
公式の同版ソース・パッチ・ライセンスを確認します。次に、別実装の `objdump` で
DLL依存を照合し、梱包後の全ファイルのハッシュを記録します。
これは確認した版とファイルに関する技術的な監査記録です。

## 候補フォルダーの作成

必要なもの：Windows x64、MSYS2 MINGW64、Python 3.10以降、Windowsの `tar.exe`、
MSYS2の `gpgv` と `msys2-keyring`。Pythonは配布準備用で、アプリの実行には不要です。
ビルドの基本手順は [BUILDING.md](BUILDING.md) にあります。

PowerShellでMINGW64のbinをPathに加え、配布用の独立したビルド先を使います。
GMPはMSYS2の正規パッケージの静的ライブラリを使います。独自GMPを使う場合は、
そのソース・パッチ・ビルド条件も別途記録し、現在の照合ツールを更新してください。

```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
cmake -S . -B build/distribution -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DRFF_DISTRIBUTION=ON -DRFF_ASAN=OFF
cmake --build build/distribution --parallel 6
python tools/release_inventory.py --exe build/distribution/runtime/bin/RFF_Super.exe --static-gmp C:/msys64/mingw64/lib/libgmp.a --output build/release-inventory.json
python tools/release_sources.py --inventory build/release-inventory.json --output build/release-sources
python tools/release_stage.py --inventory build/release-inventory.json --sources build/release-sources --shaders build/distribution/runtime/shaders --output build/release-candidate
```

コマンドは `documentation` 内ではなく、`CMakeLists.txt` があるプロジェクトルートで実行します。
別の場所にMSYS2を導入した場合はPathと `-DRFF_DEPENDENCY_PREFIX=...` を合わせ、
GMPの参照先も変更してください。ビルド先はシェーダー生成のためプロジェクト内に置きます。
Pythonは上記のMINGW64版、`tar.exe` はWindows版を使います。
依存パッケージの更新で版が変わった場合は、以前の監査一覧をそのまま流用せず再確認します。

非標準のGMP配置は `-DRFF_GMP_PREFIX=...` で指定します。監査時には
MSYS2キャッシュの正規GMPパッケージから `build/release-deps/mingw64` に取り出した
ヘッダーと静的ライブラリを使い、パッケージ記録とのハッシュ一致を確認しています。
その場合、inventoryの `--static-gmp` にも同じ実ファイルを指定します。

配布用ビルドのシェーダーは `build/distribution/runtime/shaders` に生成します。
通常版の `shaders` は更新しないため、別ビルドの実行ファイルと混在しません。

候補フォルダーにはEXE/DLL、シェーダー、著作権表示、23個の第三者ソースパッケージ
（ビルドレシピ・パッチを含む）、署名検証記録が入ります。
出力先は空の新しい名前を使います。既存フォルダーは上書きしません。
取得済みソースは再利用できます。`SHA256SUMS.json` は候補全体の照合用です。

## 実際に配布する前の最終手順

候補には `NOT-FOR-DISTRIBUTION.txt` を付けています。作業ツリーの本体ソースを
ZIPにすることはありません。変更をコミットしたリリースタグを用意し、そのタグの
クリーンなチェックアウトから再ビルドして、上の照合・梱包を実行します。
ソースはそのタグからのみ作成します。

```powershell
git status --short
git archive --format=tar.gz --prefix=RFF_Super-source/ --output=build/RFF_Super-source.tar.gz <release-tag>
```

タグがビルドしたコミットを指すことと、追跡対象に未コミット変更がないことを確認します。
本体ソース、第三者ソース、ビルド手順、ライセンス表示をバイナリと一緒に提供し、
ダウンロード配布なら対応ソースも同じ場所で取得できるようにします。
候補マーカーはこの確認後に外します。FFmpegやAIモデルを将来同梱する場合は、
その実ファイルとビルド構成を新たに確認してください。

## 一次資料

* [GPLv3本文](https://www.gnu.org/licenses/gpl-3.0.html)
* [GMPの選択可能なライセンス](https://gmplib.org/manual/Copying)
* [GCC Runtime Library Exception](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html)
* [JBIG-KIT公式ソース](https://www.cl.cam.ac.uk/~mgk25/jbigkit/)
* [MSYS2公式ソースパッケージ](https://repo.msys2.org/mingw/sources/)
* [MSYS2のライセンス情報の範囲](https://www.msys2.org/license/)
* [CC BY-NC-SA 4.0の原文](https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.en)
