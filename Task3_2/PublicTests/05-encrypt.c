#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void strcopy(char *dst, char *src)
{
    while ((*dst++ = *src++));
}


void encrypt(char *in, char *out, char *password)
{
    char *p = password;
    long long hash = 0;
    while (*p) {
        hash += *p++ * 53;
    }
    for (int i = 0; i < strlen(in); ++i) {
        char hash_part = (hash >> ((i % 4) * 8)) & 0xff;
        out[i] = in[i] ^ hash_part;
    }
}


int main(void)
{
    char in[] = "Very Very Very Secret Data 54321!";
    char password[] = "Very Very Very Long Password 12345!";
    char *out = calloc(1, sizeof(in));
    encrypt(in, out, password);
    FILE *f = fopen("public_file.txt", "wb");
    fwrite(out, sizeof(in), 1, f);
    free(out);
    return 0;
}
