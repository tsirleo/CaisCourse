#include <stdio.h>

int main(void)
{
    long long a = 42;
    int b = a;
    char c = b;
    a = 0;
    asm("movb $0, %al");
    printf("%lld %d %c\n", a, b, c);
    return 0;
}
