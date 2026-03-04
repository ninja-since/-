#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SECRET_MAGIC 0x00000000

typedef struct {
    uint32_t magic;
    uint32_t payload_sz;
    uint8_t  xor_key;
} HiddenHeader;

int main(int argc, char *argv[]) {


    FILE *f = fopen("output.pdf", "rb");
    fseek(f, 0, SEEK_END); size_t sz = ftell(f); rewind(f);
    unsigned char *data = malloc(sz); fread(data, 1, sz, f); fclose(f);

    for (size_t i = 0; i <= sz - sizeof(HiddenHeader); i++) {
        HiddenHeader *h = (HiddenHeader*)(data + i);
        if (h->magic == SECRET_MAGIC) {
            unsigned char *payload = data + i + sizeof(HiddenHeader);
            for (size_t k = 0; k < h->payload_sz; k++) payload[k] ^= h->xor_key;
            
            FILE *out = fopen("imageinjection.tar.xz", "wb");
            fwrite(payload, 1, h->payload_sz, out);
            fclose(out);
            printf("[+] Found and Decrypted! Key was 0x%02X\n", h->xor_key);
            return 0;
        }
    }
    printf("[-] No data found.\n");
    return 1;
}
