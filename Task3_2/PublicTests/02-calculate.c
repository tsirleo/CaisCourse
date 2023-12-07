#include <stdio.h>

int main(void)
{
    int a = 42;
    int b = a * 2;
    int c = 100;
    int d = (a + c) / 3 | c;
    int e = 1;
    for (int i = 0; i < 2; ++i) {
        e *= d;
    }
    e = -e;
    int f = e << 10;
    int g = 1 << (a - c / 5);
    int h = c * 100 + e;
    h = h * h;
    printf("%d %d %d\n", f, g, h);
    return 0;
}
