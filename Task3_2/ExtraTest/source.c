#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

int global;

int mystrspccmp(const char *str1, const char *str2)
{
    unsigned const char *s1 = str1, *s2 = str2;

    while (isspace(*s1)) {
        s1++;
    }

    while (isspace(*s2)) {
        s2++;
    }

    while (*s1 == *s2 && *s1 != '\0') {
        s1++;
        s2++;

        while (isspace(*s1)) {
            s1++;
        }

        while (isspace(*s2)) {
            s2++;
        }
    }
    global = *s1 - *s2;
    return *s1 - *s2;
}

int fibo(int n) {
    if (n <= 1) {
        return 1;
    }

    return fibo(n - 1) + fibo(n - 2);
}

int sw(unsigned ret) {
    switch (ret) {
        case 0:
            return 0;
        default:
            ret = 1;
            // fallthrough
    }

    return ret;
}

int long_cal_with_global() {
    global = 0;
    uint64_t a = global;
    uint64_t b = 0;
    for (uint64_t i = 0; i < 1000; ++i) {
        uint64_t a_square = a * a;
        uint64_t b_square = b * b;
        uint64_t res = a_square + b_square + global;
        a = b;
        b = res;
        global = i / 3 + 1; 
    }

    int ret = -1;
    if (b > INT_MAX) {
        ret = INT_MAX;
    } else {
        ret = (int) b;
    }

    return b; // lower byte is 201
}

int generate_strs_and_cmp(char *s1) {
    char tmp[2] = "\0\0";

    for (global = 0; global < 'z' - 'a'; ++global) {
        for (int j = 0; j < 'z' - 'a'; ++j) {
            tmp[0] = 'a' + global;
            tmp[1] = 'a' + j;

            if (strcmp(s1, tmp) == 0) {
                return 1;
            }
        }
    }

    return 0;
}


unsigned big[1000] = { 0 };
void reset() {
    for (int i = 0; i < 1000; ++i) {
        big[i] = 0;
    }
}
unsigned seq() {
    big[0] = global;
    for (int i = 2; i < 1000; i += 3) {
        big[i] = big[i / 2] + big[i / 2 + 1] + big[i / 2 + 13] + 1 + big[i - 2];
    }
    global = big[2];

    return big[123];
}

int main(int argc, char *argv[]) {
    char *s1 = "abcd";
    char *s2 = "dcabq";
    int ret = 0;

    ret = (mystrspccmp(s1, s2) != 0);
    int q = global;
    ret += mystrspccmp(s1, s1);
    ret += q;
    ret -= fibo(5);
    ret = sw(ret > 0);
    char *s3 = "aq";
    ret += generate_strs_and_cmp(s3);

    for (int i = 0; i < 100; ++i) {
        if (i % 20 == 0) {
            reset();            
        }
        ret += sw(seq() > 0);
    }

    return ret;
}