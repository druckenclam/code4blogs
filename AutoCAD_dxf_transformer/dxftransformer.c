#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* 
    This is a quick prototype for me to understand DXF format. 
*/

/*
    In production code, string may be more than 255 and dynamic
    memory allocation is in order.
*/ 
#define LONGEST_STRING 255

/* 
    According to DXF document, there are 7 sections:
    1) HEADER
    2) CLASSES
    3) TABLES
    4) BLOCKS
    5) ENTITIES
    6) OBJECTS
    7) THUMBNAIL

*/

/* 10 - 18 */
#define NUM_OF_POINTS (18 - 10 + 1)

#define DEEPEST_STACK 10
 

/* 
    Stack for our parser
*/
typedef enum {UNKNOWN_OBJ = 0, DXF_OBJ, ENTITY_OBJ, SECTION_OBJ, DXB_OBJ} ObjectType;

typedef struct StackItem {
    void* objectPtr;
    ObjectType objectType;
} StackItem;

void* objectStack[DEEPEST_STACK];
ObjectType objectTypeStack[DEEPEST_STACK];
int stackSize = 0;

void stackPush(StackItem stackItem) {
    assert(stackSize < DEEPEST_STACK);
    objectStack[stackSize] = stackItem.objectPtr;
    objectTypeStack[stackSize] = stackItem.objectType;
    ++ stackSize;
}

void stackPop() {
    assert(stackSize > 0);
    -- stackSize;
}

StackItem stackTop() {
    assert(stackSize > 0);
    StackItem stackItem = {
        objectStack[stackSize - 1],
        objectTypeStack[stackSize - 1]
    };
    return stackItem;    
}

/* 
    DXF file can be thought of as a serious of key value pairs with
    key on the first line and value on second. "counter", "startCounter", 
    or "endCounter" are used to count the number of pairs.

    Those "counters" are easily mapped to line numbers, and vice versa.
*/


typedef struct Entity {
    unsigned long long startCounter;   
    unsigned long long endCounter;
    /* 
        Group code common to all entities 
    */
    char type[LONGEST_STRING];              /* group code 0 */
    char handle[LONGEST_STRING];            /* group code 5 */
    char subclass_maker[LONGEST_STRING];    /* group code 100 */
    char layer_name[LONGEST_STRING];        /* group code 8 */
    char linetype_name[LONGEST_STRING];     /* group code 6 */
    char color_number[LONGEST_STRING];      /* group code 62 */
    char line_weight[LONGEST_STRING];       /* group code 370 */
    
    char points_x[NUM_OF_POINTS];   
    char points_y[NUM_OF_POINTS];
    
    /* 
        Next Pointer
    */
    struct Entity* next;
} Entity;

Entity* makeEntity() {
    Entity* ret = (Entity*) calloc(1, sizeof(Entity));
    return ret;
}

typedef struct Section {
    char type[LONGEST_STRING];  /* group code 2 */
    unsigned long long startCounter;
    unsigned long long endCounter;
    
    /*
        A list of Entities
     */
    Entity* pEntityHead;
    Entity* pEntityTail;
    
    /*
        Next Pointer
    */
    struct Section* next;
} Section;

Section* makeSection() {
    Section* ret = (Section*) calloc(1, sizeof(Section));
    return ret;
}

/* Root Node */
typedef struct Dxf {
    /*
        A list of sections
    */
    Section* pSectionHead;
    Section* pSectionTail;
} Dxf;

Dxf* makeDxf() {
    Dxf* ret = (Dxf*) calloc(1, sizeof(Dxf));
    return ret;    
}

typedef struct CodeData {
    unsigned long long counter;
    int code;
    char data[LONGEST_STRING]; 
} CodeData;

struct CodeData readCodeData(FILE* dxfFile) {
    /* 
        static variable keeps track of counter
    */
    static unsigned long long counter = 0;

    CodeData codeData;
    codeData.counter = counter;
    /*
        Read code of int type
    */
    char* ret = fgets(codeData.data, LONGEST_STRING, dxfFile);
    assert(ret);
    codeData.code = atoi(ret);
    /*
        Read data
    */
    ret = fgets(codeData.data, LONGEST_STRING, dxfFile);
    assert(ret);
    ++ counter;
    return codeData;
}

/* 
    Convert dxfFile to HTML file which draws graph with HTML 5 canvas
 */
void dxfProcessEntities(Section* entities, CodeData* pCodeData) {
    
}

void dxfProcessDocument(FILE* dxfFile) {
    Dxf* pdxf = makeDxf();
    StackItem stackItem = {pdxf, DXF_OBJ};
    stackPush(stackItem);
    Section* pCurrentSection = NULL;
    while (1) {
        CodeData codeData = readCodeData(dxfFile);   
        if ((codeData.code == 0) && (!strcmp(codeData.data, "EOF"))) {
            break;
        }
        
        StackItem currentItem = stackTop();
        if ((codeData.code == 0) && (!strcmp(codeData.data, "SECTION"))) {
            /*
                Check Context
            */
            assert(currentItem.objectType == DXF_OBJ);
            
            /*
                Create an new section and push into stack
            */
            Section* newSection = makeSection();
            pCurrentSection = newSection;
            
            StackItem newItem = {newSection, SECTION_OBJ};
            stackPush(newItem);
            
            /* 
                Add the new section to parent
            */
            Dxf* pdxf = (Dxf*) currentItem.objectPtr;
            if (pdxf->pSectionHead) {
                pdxf->pSectionTail->next = newSection;
                pdxf->pSectionTail = newSection;
            } else {
                pdxf->pSectionHead = pdxf->pSectionTail = newSection;
            }
            continue;
        }
        
        if ((codeData.code == 0) && (!strcmp(codeData.data, "ENDSEC"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            ((Section*) (stackItem).objectPtr)->endCounter = codeData.counter;
            stackPop();
            continue;
        }
        
        if ((codeData.code == 2) && (!strcmp(codeData.data, "HEADER"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            strcpy(((Section*)currentItem.objectPtr)->type, codeData.data);
            continue;    
        }
        
        if ((codeData.code == 2) && (!strcmp(codeData.data, "CLASS"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            strcpy(((Section*)currentItem.objectPtr)->type, codeData.data);
            continue;    
        }
        
        if ((codeData.code == 2) && (!strcmp(codeData.data, "TABLES"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            strcpy(((Section*)currentItem.objectPtr)->type, codeData.data);
            continue;    
        }
        
        if ((codeData.code == 2) && (!strcmp(codeData.data, "BLOCKS"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            strcpy(((Section*)currentItem.objectPtr)->type, codeData.data);
            continue;    
        }
        
        if ((codeData.code == 2) && (!strcmp(codeData.data, "ENTITIES"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            strcpy(((Section*)currentItem.objectPtr)->type, codeData.data);
            continue;    
        }
        
        if ((codeData.code == 2) && (!strcmp(codeData.data, "OBJECTS"))) {
            assert(currentItem.objectType == SECTION_OBJ);
            strcpy(((Section*)currentItem.objectPtr)->type, codeData.data);
            continue;    
        }
        
        if (pCurrentSection && (!strcmp(pCurrentSection->type, "ENTITIES"))) {
            dxfProcessEntities(pCurrentSection, &codeData);            
        }
    }    
}    

int main(int argc, char* argv[]) {
    FILE* f = fopen(argv[0], "r");
    if (f == NULL) {
        perror(argv[0]);
        return -1;
    }
    dxfProcessDocument(f);
    fclose(f);
    return 0;
}
