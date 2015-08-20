#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dxf_utility.h"

static void usage(char* execName) {
    printf("Usage: %s <dxf_file_name> txt|html|json\n", execName); 
}
int main(int argc, char* argv[]) {
    if (argc != 3) {
        usage(argv[0]);
        return -1;
    }
    FILE* f = fopen(argv[1], "r");
    if (f == NULL) {
        perror(argv[1]);
        return -1;
    }
    if (!strcmp(argv[2], "json")) {
        Dxf* root = dxfProcessDocument(f);
        assert(root);
        dxfConvert2JSON(root);
        dxfDestroy(root);
    } else if (!strcmp(argv[2], "txt")) {
        dxfTokenizeDocument(f);
    } else if (!strcmp(argv[2], "html")) {
        Dxf* root = dxfProcessDocument(f);
        assert(root);
        dxfConvert2HTML(root);
        dxfDestroy(root);
    } else {
        usage(argv[0]);
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}
