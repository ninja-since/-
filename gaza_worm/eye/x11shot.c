#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    Display *display;
    Window root;
    XImage *image;
    int width, height;

    // 1. الاتصال بخادم X (الواجهة الرسومية)
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "لا يمكن فتح العرض (Display)\n");
        return 1;
    }

    // 2. الحصول على النافذة الجذرية (الشاشة كاملة) وأبعادها
    root = DefaultRootWindow(display);
    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
    width = gwa.width;
    height = gwa.height;

    // 3. التقاط محتوى الشاشة ووضعه في هيكل XImage
    image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);

    if (image != NULL) {
        printf("تم التقاط الشاشة بنجاح! الأبعاد: %dx%d\n", width, height);
        
        // ملاحظة: بيانات البكسلات موجودة الآن في image->data
        // لحفظها كملف (مثلاً PPM لسهولة الكتابة):
        FILE *f = fopen("screenshot.ppm", "wb");
        fprintf(f, "P6\n%d %d\n255\n", width, height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned long pixel = XGetPixel(image, x, y);
                unsigned char r = (pixel & image->red_mask) >> 16;
                unsigned char g = (pixel & image->green_mask) >> 8;
                unsigned char b = pixel & image->blue_mask;
                fputc(r, f); fputc(g, f); fputc(b, f);
            }
        }
        fclose(f);
        XDestroyImage(image);
    }

    XCloseDisplay(display);
    return 0;
}
