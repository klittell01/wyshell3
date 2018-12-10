//gcc errcheck.c -o err
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
void main(int argc, char **argv) {
//    int i;
//    for (i = 1; i < argc; i++) {
//        fprintf(stderr, "%s ", argv[i]);
//    }
    fprintf(stderr, "kevin is testing errors\n");
    exit(0);
}
