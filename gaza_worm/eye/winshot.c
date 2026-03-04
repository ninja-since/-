#include <windows.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// فرضاً أن imageData هو المخزن الذي يحتوي على البكسلات
void sendScreenshot(char* imageData, int imageSize) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(8080); // المنفذ
    server.sin_addr.s_addr = inet_addr("192.168.1.5"); // IP السيرفر

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    // إرسال حجم الصورة أولاً ليقوم الطرف الآخر بالاستعداد
    send(sock, &imageSize, sizeof(int), 0);
    
    // إرسال بيانات الصورة الفعلية
    send(sock, imageData, imageSize, 0);

    close(sock);
}

void CaptureScreen(const char* filename) {
    // 1. الحصول على أبعاد الشاشة
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // 2. الحصول على سياق الجهاز (DC) للشاشة
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // 3. إنشاء Bitmap متوافق مع الشاشة
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    // 4. نسخ البكسلات من الشاشة إلى الذاكرة (اللقطة الفعلية)
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    // 5. إعداد معلومات ملف الـ BMP وحفظه
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = width * height * 3;

    bfh.bfType = 0x4D42; // رمز "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;
    bfh.bfReserved1 = 0; bfh.bfReserved2 = 0;

    FILE *fp = fopen(filename, "wb");
    if (fp) {
        fwrite(&bfh, sizeof(bfh), 1, fp);
        fwrite(&bih, sizeof(bih), 1, fp);
        
        // استخراج البكسلات وحفظها
        char *pixels = malloc(bih.biSizeImage);
        GetDIBits(hScreenDC, hBitmap, 0, height, pixels, (BITMAPINFO*)&bih, DIB_RGB_COLORS);
        fwrite(pixels, bih.biSizeImage, 1, fp);
        free(pixels);
        fclose(fp);
        printf("تم حفظ اللقطة باسم: %s\n", filename);
    }

    // تنظيف الموارد
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

int main() {
    CaptureScreen("screenshot.bmp");
    return 0;
}
