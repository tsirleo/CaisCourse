#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
read_file_into_buffer(char *buffer, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    size_t n = fread(buffer, 1, 2048, fp);
    buffer[n] = 0;
    fclose(fp);
}

void
success(void)
{
    printf("Congratulations!  You have successfully diverted control to another function.\n");
    printf("Now try to craft shellcode.\n");
    exit(0);
}

int
main(int argc, char **argv)
{
    struct
    {
        char a;
        unsigned int b;
        char buffer[BUFFER_SIZE];
    } s;

    read_file_into_buffer(s.buffer, argv[1]);
    printf("File has letter 'a'?  %s\n", strchr(s.buffer, 'a') ? "yes" : "no");
    return 0;
}
