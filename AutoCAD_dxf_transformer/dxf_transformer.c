#include <stdio.h>
#include <assert.h>
#include "dxf_utility.h"

int main(int argc, char* argv[]) {
    FILE* f = fopen(argv[0], "r");
    if (f == NULL) {
        perror(argv[0]);
        return -1;
    }
    Dxf* root = dxfProcessDocument(f);
    assert(root);
    fclose(f);
    return 0;
}
