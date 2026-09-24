# bin 配布の再確認（2026-09-24）

Modified by GPT-6 on 2026-09-24, 2026-09-25.

2026-09-25追記：以下は2026-09-24時点の履歴です。現在のコミット済みEXEの
SHA-256は `346df5e682eedce433a59d6151427f44239ab16a87abff2aa3c5bab606ad3418` で、
本レビュー対象とは異なります。現EXEと正確に対応するソース・静的GMP・ビルド条件は
未確認です。ライセンス表記の修正状況は [1.md](../1.md) を参照してください。

追記：このレビュー後、既存候補のライセンス・第三者ソースは公開対象の
[third-party/](../third-party/README.md) へ移動しました。以下の `build/` 内の所在は
レビュー時点の記録です。全23アーカイブの復元・ハッシュ一致を確認済みですが、
本体EXEの生成元についての未確認事項は変わりません。

## 判定

現物は RFF_Super.exe と DLL 31個、合計91,824,079バイト。調べた範囲で再配布そのものを禁止する部品は見つからない。ただし、現在の bin 単体を完成した配布物として公開できるとの判定ではない。対応する本体ソース・ビルド記録の確認、第三者ライセンスと対応ソースの提供、シェーダーの同梱が必要。

GitHubリポジトリにバイナリを置くこと自体が禁止されているわけではない。最大ファイルは libopenblas.dll の44,114,671バイトで、Git経由の100 MiB制限を下回る。ブラウザーのアップロードは25 MiBまでなので、このDLLには使えない。[GitHub公式説明](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github)

## 今回確認した証拠

- DLL31個をインストール済みMSYS2パッケージのmtree SHA-256と照合し、全件一致。
- PE通常・遅延インポートを調査し、objdumpの結果とも全32ファイルで一致。検出した非システムDLL依存はbin内で充足。
- 以前のバックアップと残存32ファイルのSHA-256が一致。
- ダウンロード済みの第三者ソース23アーカイブと署名ファイルのハッシュが、保存済み sources.json と一致。今回署名検証を再実行したという意味ではない。
- 既存の候補パッケージには licenses/、sources/third-party/、LICENSE、NOTICE があるが、Git除外の build/ 内にある。ルートのbinを公開するだけでは、これらは一緒に公開されない。
- 現在の本体EXEは、以前の release-inventory.json が対象にした本体EXEとハッシュが異なる。以前のGMP静的ライブラリ照合やビルド記録を、そのまま現EXEの証明にはできない。

## 公開前に解決する項目

1. **本体とソースを対応させる。** 現EXEの生成元コミット・ビルド条件・使用した静的GMPを確定する。確定できない場合は、プロジェクト方針に従ってコミット済みタグから配布用に再ビルドし、同じタグのソースを提供する。タグ必須はこのプロジェクトの管理方針であり、GPLの条文がGitタグを指定しているわけではない。GNUはバイナリと正確に対応するソースへの同等のアクセスを求めている。[GNU FAQ](https://www.gnu.org/licenses/gpl-faq.en.html#AnonFTPAndSendSources)
2. **ライセンスとソースへの導線を付ける。** LICENSE、NOTICE、各部品の原文・著作権表示を配布物に含める。必要な対応ソースとパッチ・ビルド手順を、バイナリの掲載場所から明確に入手できるようにする。GitHubの自動生成ソースZIPには、除外された第三者ソース一式は入らない。
3. **実行時ファイルを揃える。** bin/ と同階層の shaders/ を含める。GPUドライバーは利用者が用意し、FFmpegは動画・音声書き出しを使う人が別途導入する。現在のbinにFFmpegやVulkan loaderは含まれない。
4. **別環境で試す。** 開発環境のPATHに頼らない起動、画像読込、レンダリングを確認する。今回の検証はインポート解析であり、別PCでの実動作、マルウェア検査、脆弱性全件調査ではない。

## ライセンス上の重点項目

| 部品 | 今回の判断・必要な扱い |
| --- | --- |
| RFF_Super / 静的GMP | 本体GPLv3に対応するソースが必要。GMPはGPLv2以降またはLGPLv3以降を選べる。GPLv3での配布経路を明確にし、実際にリンクした版のソース・変更・ビルド資料を用意する。[GMP公式](https://gmplib.org/manual/Copying) |
| libjbig-0.dll | 取得済みJBIG-KIT 2.1ソースの libjbig/jbig.c で「GPL version 2 or any later version」を再確認。GPLv3構成と両立可能。パッケージの省略メタデータ GPL-2.0 だけでGPLv2限定と判断しない。[著作者の説明](https://www.cl.cam.ac.uk/~mgk25/jbigkit/) |
| libquadmath-0.dll | インストール済みgcc-libsのREADMEはLGPL-2.1-or-laterと明記。ライセンス原文と対応するライブラリソースの提供を用意する。他のGCCランタイム例外と一括扱いしない。[GCCの説明](https://gcc.gnu.org/pipermail/fortran/2022-August/058068.html) |
| libgcc / libstdc++ / libgomp / libgfortran | GPL系とGCC Runtime Library Exceptionを各対象の表示に従って扱う。例外は表示のある対象に適用され、全DLLを無条件に免責するものではない。[GCC公式条件](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html) |
| OpenCV / TBB / Lerc | Apache 2.0の原文・既存表示と、適用されるNOTICEを維持する。[OpenCV公式](https://opencv.org/license/)、[Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0) |
| libjpeg-turbo | IJG / BSD / SIMD zlibの表示を保持。製品文書のIJG謝辞は現在のNOTICEに存在する。[同版3.1.4.1の公式条件](https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/3.1.4.1/LICENSE.md) |
| liblzma | 取得済みXZ 5.8.3のCOPYINGはliblzmaを0BSDと明記。XZ全体の複合ライセンス表示と区別する。 |
| その他の画像・数値ライブラリ | MIT / BSD / zlib / PNG系等の個別原文を保持する。既存候補のlicensesを利用できるが、実際の公開物に含める必要がある。 |

既存の配布候補には「本体ソース未同梱」を示す NOT-FOR-DISTRIBUTION.txt が残る。本レビューではその表示を解除せず、公開も実施しない。最終梱包は [DISTRIBUTION.md](DISTRIBUTION.md) に従う。

## 照合したDLL一覧

以下のライセンス欄はMSYS2のパッケージメタデータであり、上の個別確認を置き換えない。

| DLL | パッケージ版 | パッケージのライセンス表記 |
| --- | --- | --- |
| `libdeflate.dll` | 1.25-1 | spdx:MIT |
| `libgcc_s_seh-1.dll` | 16.1.0-2 | spdx:GPL-3.0-or-later WITH GCC-exception-3.1 AND LGPL-2.1-or-later |
| `libgfortran-5.dll` | 16.1.0-2 | spdx:GPL-3.0-or-later |
| `libgomp-1.dll` | 16.1.0-2 | spdx:GPL-3.0-or-later WITH GCC-exception-3.1 AND LGPL-2.1-or-later |
| `libIex-3_4.dll` | 3.4.11-1 | spdx:BSD-3-Clause |
| `libIlmThread-3_4.dll` | 3.4.11-1 | spdx:BSD-3-Clause |
| `libImath-3_2.dll` | 3.2.2-4 | spdx:BSD-3-Clause |
| `libjbig-0.dll` | 2.1-5 | spdx:GPL-2.0 |
| `libjpeg-8.dll` | 3.1.4.1-2 | custom:BSD-like |
| `libLerc.dll` | 4.1.0-1 | spdx:Apache-2.0 |
| `liblzma-5.dll` | 5.8.3-1 | spdx:0BSD AND LGPL-2.1-or-later AND GPL-2.0-or-later |
| `libopenblas.dll` | 0.3.33-2 | spdx:BSD-3-Clause |
| `libopencv_core-413.dll` | 4.13.0-3 | spdx:Apache-2.0 |
| `libopencv_imgcodecs-413.dll` | 4.13.0-3 | spdx:Apache-2.0 |
| `libopencv_imgproc-413.dll` | 4.13.0-3 | spdx:Apache-2.0 |
| `libOpenEXR-3_4.dll` | 3.4.11-1 | spdx:BSD-3-Clause |
| `libOpenEXRCore-3_4.dll` | 3.4.11-1 | spdx:BSD-3-Clause |
| `libopenjp2-7.dll` | 2.5.4-2 | spdx:BSD-2-Clause |
| `libopenjph-0.27.dll` | 0.27.1-1 | spdx:BSD-2-Clause |
| `libpng16-16.dll` | 1.6.58-1 | custom |
| `libquadmath-0.dll` | 16.1.0-2 | spdx:GPL-3.0-or-later WITH GCC-exception-3.1 AND LGPL-2.1-or-later |
| `libsharpyuv-0.dll` | 1.6.0-1 | spdx:BSD-3-Clause |
| `libstdc++-6.dll` | 16.1.0-2 | spdx:GPL-3.0-or-later WITH GCC-exception-3.1 AND LGPL-2.1-or-later |
| `libtbb12.dll` | 2022.3.0-2 | spdx:Apache-2.0 |
| `libtiff-6.dll` | 4.7.1-1 | spdx:MIT |
| `libwebp-7.dll` | 1.6.0-1 | spdx:BSD-3-Clause |
| `libwebpdemux-2.dll` | 1.6.0-1 | spdx:BSD-3-Clause |
| `libwebpmux-3.dll` | 1.6.0-1 | spdx:BSD-3-Clause |
| `libwinpthread-1.dll` | 14.0.0.r37.g2bfe61fba-1 | spdx:MIT AND BSD-3-Clause-Clear |
| `libzstd.dll` | 1.5.7-2 | spdx:BSD-3-Clause OR GPL-2.0-or-later |
| `zlib1.dll` | 1.3.2-2 | spdx:Zlib |

## 本体の識別

現在のEXE SHA-256: `7bca31d2b5c9af7a32a6b2576f5a3a2bd6978ca2850d20d43fc1c297ffb39f48`

以前の配布記録EXE SHA-256: `c55c1bda50fdf235e11ecf1579fab229b69731e6feaa4d137fa76d2e51e7fcf0`

以前の記録にあるrevision: `277039337ae2f1461b75bf1d4cd7d969eff76905`。これは現EXEの生成元を証明しない。
