# 画像処理プログラム マニュアル (Windows版)

このリポジトリには、C言語で記述されたコマンドラインベースの画像処理ツールと、それを操作するためのPython製GUIが含まれています。このマニュアルはWindows環境での利用を前提としています。

## 1. 必要なもの (Windows)

### 実行環境
* **OpenCV 4.x (Pre-built library)**
* **MinGW-w64** (C/C++コンパイラ `gcc` を含む)
* **Python 3.x**
* Pythonライブラリ: `opencv-python`, `Pillow`

### インストール手順 🛠️

**ステップ A: MinGW-w64 (コンパイラ) のインストール**
`gcc`コマンドを使えるようにします。
1.  [MSYS2の公式サイト](https://www.msys2.org/)にアクセスし、インストーラーをダウンロードして実行します。
2.  インストールが完了したら、MSYS2のターミナルを起動し、以下のコマンドを実行して最新の状態に更新し、開発ツールをインストールします。
    ```bash
    pacman -Syu
    pacman -Su
    pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
    ```
3.  Windowsの環境変数 `Path` に `C:\msys64\ucrt64\bin` を追加します。これにより、コマンドプロンプトやPowerShellから`gcc`コマンドが使えるようになります。

**ステップ B: OpenCV (ライブラリ) のインストール**
1.  [OpenCV公式サイトのリリースぺージ](https://opencv.org/releases/)にアクセスします。
2.  最新のバージョン（例: 4.9.0）の「**Windows**」リンクをクリックし、自己解凍形式のexeファイルをダウンロードします。
3.  ダウンロードしたexeファイルを実行します。展開先のフォルダを聞かれるので、`C:\` など分かりやすい場所を指定します。（例: `C:\opencv`）
4.  展開が完了すると、`C:\opencv` フォルダ内に `build` などのフォルダが作成されます。

**ステップ C: Pythonライブラリのインストール**
コマンドプロンプトやPowerShellで以下のコマンドを実行します。
```bash
pip install opencv-python Pillow
```

## 2. セットアップ (コンパイル)

C言語プログラム `image_processor.c` をコンパイルします。
コマンドプロンプト（またはPowerShell）を起動し、`image_processor.c` があるフォルダに移動してから、以下のコマンドを実行します。

**【重要】** 以下のコマンドは、OpenCVを`C:\opencv`に展開し、バージョンが`4.9.0`の場合の例です。ご自身の環境に合わせて、**パスとバージョン番号（`490`の部分）を修正してください。**

```cmd
gcc image_processor.c -o image_processor.exe -I"C:\opencv\build\include" -L"C:\opencv\build\x64\mingw\lib" -lopencv_world490
```

**コマンド解説:**
* `gcc image_processor.c -o image_processor.exe`: `image_processor.c`をコンパイルして `image_processor.exe` という名前の実行ファイルを作成します。
* `-I"C:\opencv\build\include"`: OpenCVのヘッダファイル（`.h`ファイル）がどこにあるかを指定します。
* `-L"C:\opencv\build\x64\mingw\lib"`: OpenCVのライブラリファイル（`.lib`ファイル）がどこにあるかを指定します。
* `-lopencv_world490`: リンクするライブラリファイルを指定します。`libopencv_world490.dll.a` の `lib` と `.dll.a` を除いた部分です。バージョンが `4.8.0` なら `-lopencv_world480` となります。

成功すると、`image_processor.exe` という実行ファイルが生成されます。

## 3. 使用方法

### A. コマンドラインツール (`image_processor.exe`)

基本的な書式は以下の通りです。

```
.\image_processor.exe <処理の種類> <入力画像ファイル> [オプション値]
```
処理結果は、常に `output.bmp` というファイル名で保存されます。

**実行例:**
* **二値化 (閾値: 128)**
    ```cmd
    .\image_processor.exe binarize mandrill512.bmp 128
    ```
* **平滑化 (ガウシアンフィルタ, σ=2.0)**
    ```cmd
    .\image_processor.exe smooth dollar512.bmp 2.0
    ```

### B. GUIツール (`gui.py`)

GUIツールは、コマンドを直接入力せずに画像処理を実行できます。

**起動方法:**
```cmd
python gui.py
```
**操作手順:**
1.  ウィンドウが起動したら、「選択...」ボタンを押し、先ほどコンパイルした `image_processor.exe` を選択します。
2.  「入力画像を選択」ボタンで処理したい画像を選択します。
3.  実行したい処理とパラメータを選び、「実行」ボタンを押します。
4.  成功すると、ウィンドウの右側に結果画像が表示されます。

## 4. 関数仕様 (C言語)

(このセクションは前回の回答から変更ありません)