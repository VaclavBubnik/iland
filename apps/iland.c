#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "runilandmodel.h"

int main(int argc, char *argv[])
{
    char *end = NULL;
    long years;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <project.xml> <years>\n", argv[0]);
        return 2;
    }

    errno = 0;
    years = strtol(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || years < 0 || years > INT_MAX) {
        fprintf(stderr, "Invalid year count: %s\n", argv[2]);
        return 2;
    }

    return runilandmodel(argv[1], (int)years);
}
