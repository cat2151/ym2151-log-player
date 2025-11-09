# ym2151-log-player

<p align="left">
  <a href="README.ja.md"><img src="https://img.shields.io/badge/🇯🇵-Japanese-red.svg" alt="Japanese"></a>
  <a href="README.md"><img src="https://img.shields.io/badge/🇺🇸-English-blue.svg" alt="English"></a>
</p>

YM2151レジスタイベントログをJSONファイルから読み込んで、リアルタイム再生とWAVファイル出力を行うプログラム。

## 状況

アーカイブします。後継のRust版を開発します。

このリポジトリの用途は、C言語による最小限のコードを提供する用です。

## 概要

このプログラムはYM2151（OPM）のレジスタ操作ログをJSON形式で読み込み、Nuked-OPMエミュレータを使用して音声を再生します。

## 機能

- ✅ JSONログファイルからイベントを読み込み
- ✅ リアルタイムオーディオ再生
- ✅ WAVファイル出力（output.wav）
- ✅ Nuked-OPMライブラリによる正確なYM2151エミュレーション

## ビルド

### Linux (gcc)
```bash
gcc -o player src/player.c opm.c -lm -lpthread -ldl -fwrapv
```

### Linux (zig cc) / Windows
```bash
python build.py build-phase4
```

または

```bash
python build.py build-phase4-gcc  # Linux with gcc
python build.py build-phase4-windows  # Windows cross-compile
```

## 使用方法

```bash
./player <json_log_file>
```

例：
```bash
./player events.json
```

## 入力JSON形式

YM2151ログJSONファイルは以下の形式である必要があります：

```json
{
  "event_count": 8,
  "events": [
    {"time": 0, "addr": "0x08", "data": "0x00"},
    {"time": 100, "addr": "0x20", "data": "0xC7"},
    ...
  ]
}
```

各イベントフィールド：
- `time`: サンプル時刻（絶対時刻、デルタではない）
- `addr`: YM2151レジスタアドレス（16進数文字列）
- `data`: レジスタに書き込むデータ（16進数文字列）

**注意**: プログラムは自動的にレジスタ書き込みを2段階（アドレス書き込み→データ書き込み）に分割し、必要な遅延を追加します。
JSONファイルに`is_data`フィールドが含まれていても無視されます。入力は常にpass1形式（単純なレジスタ書き込み）として扱われます。

## 出力

- リアルタイムオーディオ再生
- `output.wav` - 再生内容を記録したWAVファイル（ハードコーディング）

## 利用ライブラリ

- Nuked-OPM: LGPL 2.1
- MiniAudio: Public Domain OR MIT-0

## ライセンス

MIT License
