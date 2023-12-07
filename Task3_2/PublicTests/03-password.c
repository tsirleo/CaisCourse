#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char password[] = "password";
    char *copy = malloc(sizeof(password));
    for (int i = 0; i < sizeof(password); ++i) {
        copy[i] = password[i];
    }
    FILE *f = fopen("public_file.txt", "w");
    fprintf(f, "%s", copy);
    free(copy);
    return 0;
}
