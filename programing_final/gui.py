import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import os
from PIL import Image, ImageTk

class ImageProcessingGUI(tk.Tk):
  def __init__(self):
    super().__init__()
    self.title("画像処理ツール")
    self.geometry("850x600")

    # 変数の初期化
    self.cpp_executable_path = tk.StringVar()
    self.input_image_path = tk.StringVar()
    self.process_type = tk.StringVar(value="binarize")
    self.parameter_value = tk.StringVar(value="128")
    self.output_image_path = "output.bmp"

    # --- UIのレイアウト ---
    main_frame = ttk.Frame(self, padding="10")
    main_frame.pack(fill=tk.BOTH, expand=True)

    # 左側のコントロールパネル
    controls_frame = ttk.Frame(main_frame, padding="10", relief=tk.RIDGE)
    controls_frame.pack(side=tk.LEFT, fill=tk.Y, padx=10, pady=10)

    # 右側の画像表示エリア
    self.image_display_frame = ttk.Frame(
        main_frame, padding="10", relief=tk.SUNKEN)
    self.image_display_frame.pack(
        side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)
    self.image_label = ttk.Label(
        self.image_display_frame, text="ここに結果が表示されます")
    self.image_label.pack(expand=True)

    # --- コントロールパネル内のウィジェット ---
    # 1. 実行ファイル選択
    ttk.Label(controls_frame, text="ステップ1: 実行ファイル", font=(
        "", 10, "bold")).pack(fill=tk.X, pady=(0, 5))
    exe_frame = ttk.Frame(controls_frame)
    exe_frame.pack(fill=tk.X)
    ttk.Entry(exe_frame, textvariable=self.cpp_executable_path,
              width=30).pack(side=tk.LEFT, expand=True, fill=tk.X)
    ttk.Button(exe_frame, text="選択...", command=self.select_executable).pack(
        side=tk.LEFT, padx=(5, 0))

    # 2. 入力画像選択
    ttk.Label(controls_frame, text="ステップ2: 入力画像", font=(
        "", 10, "bold")).pack(fill=tk.X, pady=(15, 5))
    img_frame = ttk.Frame(controls_frame)
    img_frame.pack(fill=tk.X)
    ttk.Entry(img_frame, textvariable=self.input_image_path,
              width=30).pack(side=tk.LEFT, expand=True, fill=tk.X)
    ttk.Button(img_frame, text="選択...", command=self.select_image).pack(
        side=tk.LEFT, padx=(5, 0))

    # 3. 処理の選択
    ttk.Label(controls_frame, text="ステップ3: 処理を選択", font=(
        "", 10, "bold")).pack(fill=tk.X, pady=(15, 5))
    process_options_frame = ttk.LabelFrame(controls_frame, text="処理の種類")
    process_options_frame.pack(fill=tk.X, pady=5)

    processes = {
        "二値化": "binarize",
        "平滑化": "smooth",
        "エッジ検出": "edge",
        "ラプラシアン": "laplacian",
        "LoGフィルタ": "log"
    }
    for text, value in processes.items():
      ttk.Radiobutton(process_options_frame, text=text, variable=self.process_type,
                      value=value, command=self.update_parameter_entry).pack(anchor=tk.W)

    # 4. パラメータ入力
    self.param_frame = ttk.LabelFrame(controls_frame, text="パラメータ")
    self.param_frame.pack(fill=tk.X, pady=5)
    self.param_label = ttk.Label(self.param_frame, text="閾値 (0-255):")
    self.param_label.pack(side=tk.LEFT, padx=5)
    self.param_entry = ttk.Entry(
        self.param_frame, textvariable=self.parameter_value, width=10)
    self.param_entry.pack(side=tk.LEFT, padx=5)

    # 5. 実行ボタン
    ttk.Button(controls_frame, text="実行", command=self.run_processing,
               style="Accent.TButton").pack(fill=tk.X, pady=(20, 10), ipady=5)

    # スタイル設定
    style = ttk.Style(self)
    style.configure("Accent.TButton", font=("", 12, "bold"))

    # 初期状態の更新
    self.update_parameter_entry()

  def select_executable(self):
    path = filedialog.askopenfilename(title="C言語 実行可能ファイルを選択")
    if path:
      self.cpp_executable_path.set(path)

  def select_image(self):
    path = filedialog.askopenfilename(
        title="画像ファイルを選択",
        filetypes=[("BMP/PNG/JPG", "*.bmp *.png *.jpg *.jpeg"),
                   ("All files", "*.*")]
    )
    if path:
      self.input_image_path.set(path)

  def update_parameter_entry(self):
    process = self.process_type.get()
    if process in ["binarize", "smooth", "log"]:
      self.param_entry.config(state="normal")
      if process == "binarize":
        self.param_label.config(text="閾値 (0-255):")
      elif process == "smooth" or process == "log":
        self.param_label.config(text="σ (例: 1.5):")
    else:
      self.param_entry.config(state="disabled")
      self.param_label.config(text="パラメータ:")

  def run_processing(self):
    # 入力チェック
    exe_path = self.cpp_executable_path.get()
    img_path = self.input_image_path.get()

    if not exe_path or not os.path.exists(exe_path):
      messagebox.showerror("エラー", "有効な実行ファイルが選択されていません。")
      return
    if not img_path or not os.path.exists(img_path):
      messagebox.showerror("エラー", "有効な入力画像が選択されていません。")
      return

    # コマンドを構築
    command = [exe_path, self.process_type.get(), img_path]
    if self.process_type.get() in ["binarize", "smooth", "log"]:
      param = self.parameter_value.get()
      if not param:
        messagebox.showerror("エラー", "パラメータ値を入力してください。")
        return
      command.append(param)

    try:
      # C言語プログラムを実行
      print(f"実行中コマンド: {' '.join(command)}")
      subprocess.run(command, check=True, capture_output=True, text=True)

      # 結果画像を表示
      if os.path.exists(self.output_image_path):
        self.display_image(self.output_image_path)
      else:
        messagebox.showerror("エラー", "出力ファイル 'output.bmp' が見つかりません。")

    except FileNotFoundError:
      messagebox.showerror(
          "実行エラー", f"コマンド '{exe_path}' が見つかりません。パスを確認してください。")
    except subprocess.CalledProcessError as e:
      messagebox.showerror(
          "実行エラー", f"プログラムの実行中にエラーが発生しました。\n\n詳細:\n{e.stderr}")
    except Exception as e:
      messagebox.showerror("予期せぬエラー", str(e))

  def display_image(self, image_path):
    try:
      # Pillowを使って画像を開き、Tkinterで表示できる形式に変換
      image = Image.open(image_path)

      # 画像表示エリアのサイズに合わせてリサイズ
      max_width = self.image_display_frame.winfo_width() - 20
      max_height = self.image_display_frame.winfo_height() - 20
      image.thumbnail((max_width, max_height), Image.Resampling.LANCZOS)

      photo = ImageTk.PhotoImage(image)

      self.image_label.config(image=photo, text="")
      # この参照を保持しないと画像が表示されないことがある
      self.image_label.image = photo
    except Exception as e:
      messagebox.showerror("画像表示エラー", f"画像の表示に失敗しました。\n{e}")

if __name__ == "__main__":
  app = ImageProcessingGUI()
  app.mainloop()
