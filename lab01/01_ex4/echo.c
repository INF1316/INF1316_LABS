#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        fprintf(stdout, "argv[%d]: %s\n", i, argv[i]);
    }

    exit(EXIT_SUCCESS);
}