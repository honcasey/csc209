#include <stdio.h>
#include <stdlib.h>
#include "dectree.h"

Dataset *data;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s dataset file\n", argv[0]);
        exit(1);
    }

    data = load_dataset(argv[1]);
    printf("loaded %d items \n", sizeof(data->images));
    return 0;
}