#include <stdio.h>
#include <assert.h>
#include "dxf_utility.h"

int main(int argc, char* argv[]) {
    FILE* f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return -1;
    }
    Dxf* root = dxfProcessDocument(f);
    assert(root);
    fclose(f);
    return 0;
}
