#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
read_file_into_buffer(char *buffer, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    fscanf(fp, "%s", buffer);
    fclose(fp);
}

void
process_data(const char *data)
{
    printf("File size: %zu\n", strlen(data));
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
        char buffer[BUFFER_SIZE];
        int a;
        unsigned char b;
        void (*func)(const char *);
    } s;

    s.func = process_data;
    read_file_into_buffer(s.buffer, argv[1]);
    s.func(s.buffer);
    return 0;
}
