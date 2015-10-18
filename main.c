#include <stdio.h>
#include "sqlite3.h"

int main()
{
    printf("Starting main...\n");

    printf("Using sqlite version: %s\n", sqlite3_libversion());    

    printf("Done.\n");
    return 0;
}
