#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Windowsのコンパイラで構造体のパディングを無効にするためのおまじない
#pragma pack(push, 1)

// BMPファイルのヘッダ (14バイト)
typedef struct {
    unsigned short bfType;      // 'BM' (0x4D42)
    unsigned int   bfSize;      // ファイルサイズ
    unsigned short bfReserved1; // 予約領域1 (0)
    unsigned short bfReserved2; // 予約領域2 (0)
    unsigned int   bfOffBits;   // 先頭から画像データまでのオフセット
} BITMAPFILEHEADER;

// BMPの情報ヘッダ (40バイト)
typedef struct {
    unsigned int   biSize;          // この構造体のサイズ (40)
    int            biWidth;         // 画像の幅
    int            biHeight;        // 画像の高さ
    unsigned short biPlanes;        // プレーン数 (1)
    unsigned short biBitCount;      // 1ピクセルあたりのビット数
    unsigned int   biCompression;   // 圧縮形式 (0:無圧縮)
    unsigned int   biSizeImage;     // 画像データのサイズ
    int            biXPelsPerMeter; // 水平解像度
    int            biYPelsPerMeter; // 垂直解像度
    unsigned int   biClrUsed;       // カラーパレット数
    unsigned int   biClrImportant;  // 重要色数
} BITMAPINFOHEADER;

#pragma pack(pop)

// 画像データを保持するためのカスタム構造体
typedef struct {
    int width;
    int height;
    int channels; // 1:グレースケール, 3:BGRカラー
    unsigned char* data;
} Image;

// --- 関数のプロトタイプ宣言 ---
Image* createImage(int width, int height, int channels);
void freeImage(Image* img);
Image* readBMP(const char* filename);
int writeBMP(const char* filename, const Image* img);
Image* convertToGrayscale(const Image* src);
Image* binarizeImage(const Image* src, int threshold);
Image* applyConvolution(const Image* src, const double kernel[3][3]);
Image* detectEdges(const Image* src);
Image* applyLaplacian(const Image* src);
Image* smoothImage(const Image* src, double sigma);
Image* applyLoG(const Image* src, double sigma);

// 画像用のメモリ確保
Image* createImage(int width, int height, int channels) {
    Image* img = (Image*)malloc(sizeof(Image));
    if (!img) return NULL;
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = (unsigned char*)malloc(width * height * channels);
    if (!img->data) {
        free(img);
        return NULL;
    }
    return img;
}

// 画像用のメモリ解放
void freeImage(Image* img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

// BMPファイルを読み込む関数
Image* readBMP(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    if (fileHeader.bfType != 0x4D42) { // 'BM'のチェック
        fprintf(stderr, "Error: Not a valid BMP file.\n");
        fclose(fp);
        return NULL;
    }
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    // 24bitカラーまたは8bitグレースケール無圧縮のみ対応
    if (infoHeader.biCompression != 0 || (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 8)) {
        fprintf(stderr, "Error: Only 24-bit or 8-bit uncompressed BMPs are supported.\n");
        fclose(fp);
        return NULL;
    }

    int channels = infoHeader.biBitCount / 8;
    Image* img = createImage(infoHeader.biWidth, abs(infoHeader.biHeight), channels);
    if (!img) {
        fclose(fp);
        return NULL;
    }

    // 画像データの開始位置に移動
    fseek(fp, fileHeader.bfOffBits, SEEK_SET);

    // BMPの1行のバイト数（4バイトの倍数にパディングされる）
    int padding = (4 - (img->width * channels) % 4) % 4;
    int row_size = img->width * channels + padding;
    unsigned char* row_buffer = (unsigned char*)malloc(row_size);

    // BMPは上下逆さまに格納されているので、下の行から読み込む
    for (int y = img->height - 1; y >= 0; y--) {
        fread(row_buffer, 1, row_size, fp);
        memcpy(img->data + (y * img->width * channels), row_buffer, img->width * channels);
    }
    
    free(row_buffer);
    fclose(fp);

    // 8bit BMPの場合はグレースケールとして扱うのでチャンネルは1
    if (infoHeader.biBitCount == 8) {
        img->channels = 1;
    }

    return img;
}

// グレースケール画像を8bit BMPとして書き込む関数
int writeBMP(const char* filename, const Image* img) {
    if (img->channels != 1) {
        fprintf(stderr, "Error: writeBMP currently only supports grayscale images.\n");
        return 0;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) return 0;

    int width = img->width;
    int height = img->height;
    int padding = (4 - width % 4) % 4;
    int row_size = width + padding;
    int image_data_size = row_size * height;
    
    // 8bit BMPはカラーパレットが必要
    int palette_size = 256 * 4;
    int file_header_size = sizeof(BITMAPFILEHEADER);
    int info_header_size = sizeof(BITMAPINFOHEADER);
    int file_size = file_header_size + info_header_size + palette_size + image_data_size;


    BITMAPFILEHEADER fileHeader = {
        0x4D42,
        file_size,
        0, 0,
        file_header_size + info_header_size + palette_size
    };

    BITMAPINFOHEADER infoHeader = {
        sizeof(BITMAPINFOHEADER),
        width, height, 1, 8, 0,
        image_data_size,
        0, 0, 256, 0
    };
    
    fwrite(&fileHeader, 1, file_header_size, fp);
    fwrite(&infoHeader, 1, info_header_size, fp);

    // グレースケールのためのカラーパレットを作成
    unsigned char palette[palette_size];
    for (int i = 0; i < 256; i++) {
        palette[i * 4 + 0] = i; // Blue
        palette[i * 4 + 1] = i; // Green
        palette[i * 4 + 2] = i; // Red
        palette[i * 4 + 3] = 0; // Reserved
    }
    fwrite(palette, 1, palette_size, fp);

    // 画像データを書き込み
    unsigned char* row_buffer = (unsigned char*)calloc(row_size, 1);
    for (int y = height - 1; y >= 0; y--) {
        memcpy(row_buffer, img->data + (y * width), width);
        fwrite(row_buffer, 1, row_size, fp);
    }

    free(row_buffer);
    fclose(fp);
    return 1;
}

// カラー画像をグレースケールに変換
Image* convertToGrayscale(const Image* src) {
    if (src->channels == 1) return NULL; // すでにグレースケール
    Image* dst = createImage(src->width, src->height, 1);
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int i_src = (y * src->width + x) * src->channels;
            int i_dst = y * dst->width + x;
            unsigned char b = src->data[i_src + 0];
            unsigned char g = src->data[i_src + 1];
            unsigned char r = src->data[i_src + 2];
            // グレースケール変換の標準的な計算式
            dst->data[i_dst] = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
        }
    }
    return dst;
}

// 二値化
Image* binarizeImage(const Image* src, int threshold) {
    Image* dst = createImage(src->width, src->height, 1);
    for (int i = 0; i < src->width * src->height; i++) {
        dst->data[i] = (src->data[i] >= threshold) ? 255 : 0;
    }
    return dst;
}

// 畳み込み演算を行う汎用関数
Image* applyConvolution(const Image* src, const double kernel[3][3]) {
    Image* dst = createImage(src->width, src->height, 1);
    for (int y = 1; y < src->height - 1; y++) {
        for (int x = 1; x < src->width - 1; x++) {
            double sum = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel_val = src->data[(y + ky) * src->width + (x + kx)];
                    sum += pixel_val * kernel[ky + 1][kx + 1];
                }
            }
            // 0-255の範囲にクリッピング
            if (sum < 0) sum = 0;
            if (sum > 255) sum = 255;
            dst->data[y * dst->width + x] = (unsigned char)sum;
        }
    }
    return dst;
}

// エッジ検出（Sobelフィルタ）
Image* detectEdges(const Image* src) {
    Image* dst = createImage(src->width, src->height, 1);
    
    const double sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const double sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int y = 1; y < src->height - 1; y++) {
        for (int x = 1; x < src->width - 1; x++) {
            double gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel_val = src->data[(y + ky) * src->width + (x + kx)];
                    gx += pixel_val * sobel_x[ky + 1][kx + 1];
                    gy += pixel_val * sobel_y[ky + 1][kx + 1];
                }
            }
            double magnitude = sqrt(gx * gx + gy * gy);
            if (magnitude > 255) magnitude = 255;
            dst->data[y * dst->width + x] = (unsigned char)magnitude;
        }
    }
    return dst;
}

// ラプラシアンフィルタ
Image* applyLaplacian(const Image* src) {
    const double kernel[3][3] = {{0, 1, 0}, {1, -4, 1}, {0, 1, 0}};
    Image* convolved = applyConvolution(src, kernel);
    // 結果が見やすいようにスケーリング（ここでは単純に絶対値を取るなどでも良い）
    // applyConvolution内で0-255にクリッピングしているのでそのまま返す
    return convolved;
}

// 平滑化（ガウシアンフィルタ）
Image* smoothImage(const Image* src, double sigma) {
    Image* dst = createImage(src->width, src->height, src->channels);
    int ksize = (int)(sigma * 3) * 2 + 1; // カーネルサイズを奇数に
    int k_half = ksize / 2;
    double* kernel = (double*)malloc(ksize * ksize * sizeof(double));
    double k_sum = 0;

    // ガウシアンカーネルの生成
    for (int y = -k_half; y <= k_half; y++) {
        for (int x = -k_half; x <= k_half; x++) {
            double val = exp(-(x * x + y * y) / (2 * sigma * sigma));
            kernel[(y + k_half) * ksize + (x + k_half)] = val;
            k_sum += val;
        }
    }
    // カーネルの正規化
    for (int i = 0; i < ksize * ksize; i++) {
        kernel[i] /= k_sum;
    }

    // 畳み込み
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            for (int c = 0; c < src->channels; c++) {
                double sum = 0;
                for (int ky = -k_half; ky <= k_half; ky++) {
                    for (int kx = -k_half; kx <= k_half; kx++) {
                        int px = x + kx;
                        int py = y + ky;
                        // 境界処理（ミラーリング）
                        if (px < 0) px = -px;
                        if (py < 0) py = -py;
                        if (px >= src->width) px = 2 * src->width - px - 2;
                        if (py >= src->height) py = 2 * src->height - py - 2;
                        
                        if(px >= 0 && px < src->width && py >= 0 && py < src->height) {
                           int i_src = (py * src->width + px) * src->channels + c;
                           sum += src->data[i_src] * kernel[(ky + k_half) * ksize + (kx + k_half)];
                        }
                    }
                }
                int i_dst = (y * dst->width + x) * dst->channels + c;
                dst->data[i_dst] = (unsigned char)sum;
            }
        }
    }

    free(kernel);
    return dst;
}

// LoGフィルタ
Image* applyLoG(const Image* src_gray, double sigma) {
    Image* smoothed = smoothImage(src_gray, sigma);
    Image* result = applyLaplacian(smoothed);
    freeImage(smoothed);
    return result;
}

void showUsage(const char* name) {
    fprintf(stderr, "Usage: %s <process_type> <input_image> [options]\n\n", name);
    fprintf(stderr, "Process Types:\n");
    fprintf(stderr, "  binarize    <threshold>   ... Binarize image\n");
    fprintf(stderr, "  smooth      <sigma>       ... Smooth with Gaussian filter\n");
    fprintf(stderr, "  edge                      ... Detect edges with Sobel filter\n");
    fprintf(stderr, "  laplacian                 ... Apply Laplacian filter\n");
    fprintf(stderr, "  log         <sigma>       ... Apply Laplacian of Gaussian filter\n\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s binarize mandrill512.bmp 128\n", name);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        showUsage(argv[0]);
        return 1;
    }

    const char* processType = argv[1];
    const char* inputImagePath = argv[2];

    Image* inputImage = readBMP(inputImagePath);
    if (!inputImage) {
        return 1;
    }

    Image* grayImage = NULL;
    // カラー画像はグレースケールに変換して処理する（平滑化を除く）
    if (inputImage->channels == 3) {
        grayImage = convertToGrayscale(inputImage);
    } else {
        grayImage = inputImage; // 参照をコピー
    }
    
    Image* resultImage = NULL;
    int needs_param = (strcmp(processType, "binarize") == 0 || strcmp(processType, "smooth") == 0 || strcmp(processType, "log") == 0);

    if (needs_param && argc < 4) {
         fprintf(stderr, "Error: Process type '%s' requires a parameter.\n", processType);
         showUsage(argv[0]);
         return 1;
    }

    if (strcmp(processType, "binarize") == 0) {
        resultImage = binarizeImage(grayImage, atoi(argv[3]));
    } else if (strcmp(processType, "smooth") == 0) {
        // 平滑化はカラー画像に直接適用
        resultImage = smoothImage(inputImage, atof(argv[3]));
    } else if (strcmp(processType, "edge") == 0) {
        resultImage = detectEdges(grayImage);
    } else if (strcmp(processType, "laplacian") == 0) {
        resultImage = applyLaplacian(grayImage);
    } else if (strcmp(processType, "log") == 0) {
        resultImage = applyLoG(grayImage, atof(argv[3]));
    } else {
        fprintf(stderr, "Error: Unknown process type '%s'\n", processType);
        showUsage(argv[0]);
    }
    
    const char* outputFilename = "output.bmp";
    if (resultImage) {
        if(resultImage->channels == 3) {
            // 現在のwriteBMPはグレースケールのみ対応のため、結果をグレースケールに変換
            Image* grayResult = convertToGrayscale(resultImage);
            if(writeBMP(outputFilename, grayResult)) {
                printf("Processing complete. Image saved as %s\n", outputFilename);
            } else {
                fprintf(stderr, "Error: Failed to save the output image.\n");
            }
            freeImage(grayResult);
        } else {
            if (writeBMP(outputFilename, resultImage)) {
                printf("Processing complete. Image saved as %s\n", outputFilename);
            } else {
                fprintf(stderr, "Error: Failed to save the output image.\n");
            }
        }
    }

    // メモリ解放
    if (inputImage->channels == 3) {
        freeImage(grayImage); // カラーから変換した場合のみ解放
    }
    freeImage(inputImage);
    freeImage(resultImage);

    return 0;
}