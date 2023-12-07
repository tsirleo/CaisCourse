#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void strcopy(char *dst, char *src)
{
    while ((*dst++ = *src++));
}


void build(char *ss[], char *out)
{
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 2; ++j) {
            out[2 * i + j] = ss[i][2 * i + j];
        }
    }
}


void swap(char *c1, char *c2)
{
    char tmp = *c1;
    *c1 = *c2;
    *c2 = tmp;
}


void reverse(char *s)
{
    for (int i = 0; i < strlen(s) / 2; ++i) {
        swap(s + i,  s + strlen(s) - i - 1);
    }
}


int main(void)
{
    char *ss[] = {
        "12cdefghijklmnopqrtuvwxyz",
        "ab34efghijklmnopqrtuvwxyz",
        "abcd56ghijklmnopqrtuvwxyz",
        "abcdef78ijklmnopqrtuvwxyz",
        "abcdefgh90klmnopqrtuvwxyz"
    };
    char s[26] = {0};
    build(ss, s);
    reverse(s);
    printf("%s\n", s);
    return 0;
}
