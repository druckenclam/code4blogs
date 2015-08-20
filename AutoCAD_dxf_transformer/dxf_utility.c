#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dxf_utility.h"

/* This is a quick prototype for me to understand DXF format. */

/* For the sake of simplicity, assume strings are less than 255 characters */
#define LONGEST_STRING 255 + 1

/* 10 - 18 */
#define NUM_OF_POINTS (18 - 10 + 1)

#define DEEPEST_STACK 10

/* If we are doing things wrong, we will be notifed of NULL pointer use */
#define SAFE_CAST_TO(CLASS_NAME, STACK_ITEM) \
((STACK_ITEM.objectType == CLASS_NAME ## _OBJ) ?((CLASS_NAME*)STACK_ITEM.objectPtr)\
: NULL)

/* DXF file can be thought of as a serious of key value pairs with
 * key on the first line and value on second. "counter", "startCounter", 
 * or "endCounter" are used to count the number of pairs.
 * Those "counters" are easily mapped to line numbers, and vice versa.*/

typedef struct Entity {
    unsigned long long startCounter;   
    unsigned long long endCounter;
    /* Group Code Common to All Entities */
    char type[LONGEST_STRING];              /* Group Code 0 */
    char handle[LONGEST_STRING];            /* Group Code 5 */
    char subclass_maker[LONGEST_STRING];    /* Group Code 100 */
    char layer_name[LONGEST_STRING];        /* Group Code 8 */
    char linetype_name[LONGEST_STRING];     /* Group Code 6 */
    char color_number[LONGEST_STRING];      /* Group Code 62 */
    char line_weight[LONGEST_STRING];       /* Group Code 370 */
    
    char points_x[NUM_OF_POINTS];   
    char points_y[NUM_OF_POINTS];
    char points_z[NUM_OF_POINTS];
    int numOfPoins;
    
    /* Next Pointer */
    struct Entity* next;
} Entity;

typedef struct Section {
    char type[LONGEST_STRING];  /* Group Code 2 */
    unsigned long long startCounter;
    unsigned long long endCounter;
    
    /* A list of Entities */
    Entity* pEntityHead;
    Entity* pEntityTail;
    
    /* Next Pointer */
    struct Section* next;
} Section;



/* Root Node */
typedef struct Dxf {
    /* A List of Sections */
    Section* pSectionHead;
    Section* pSectionTail;
} Dxf;

typedef struct CodeData {
    unsigned long long counter;
    int code;
    char data[LONGEST_STRING]; 
} CodeData;

/* Stack for Our Parser */
typedef enum {UNKNOWN_OBJ = 0, Dxf_OBJ, Entity_OBJ, Section_OBJ} ObjectType;

typedef struct StackItem {
    void* objectPtr;
    ObjectType objectType;
} StackItem;

/* Global Variables */
void* objectStack[DEEPEST_STACK];
ObjectType objectTypeStack[DEEPEST_STACK];
int stackSize = 0;

/* Function Prototypes */
static Entity* makeEntity(void);
static Section* makeSection(void);
static Dxf* makeDxf(void);
static bool isSection(CodeData* pCodeData);
static void stackPush(StackItem stackItem);
static bool stackEmpty(void);
static void stackPop(void);
static StackItem stackTop(void);
static struct CodeData readCodeData(FILE* dxfFile);
static void dxfProcessEntities(CodeData* pCodeData);

/* External Functions */
Dxf* dxfProcessDocument(FILE* dxfFile) {
    Dxf* root = makeDxf();
    StackItem stackItem = {root, Dxf_OBJ};
    stackPush(stackItem);
    
    Section* pCurrentSection = NULL;
    while (1) {
        CodeData codeData = readCodeData(dxfFile);   
        if ((codeData.code == 0) && (!strcmp(codeData.data, "EOF"))) {
            assert(SAFE_CAST_TO(Dxf, stackTop()));
            stackPop();
            assert(stackEmpty());
            break;
        }

        if ((codeData.code == 0) && (!strcmp(codeData.data, "SECTION"))) {
            /* Check Context */
            Dxf* pDxf = SAFE_CAST_TO(Dxf, stackTop());
            assert(pDxf); 
            
            /* Create an new section and push into stack */
            Section* newSection = makeSection();
            pCurrentSection = newSection;
            
            StackItem newItem = {newSection, Section_OBJ};
            stackPush(newItem);
            
            /* Add the new section to parent */
            if (pDxf->pSectionHead) {
                pDxf->pSectionTail->next = newSection;
                pDxf->pSectionTail = newSection;
            } else {
                pDxf->pSectionHead = pDxf->pSectionTail = newSection;
            }
            continue;
        }
        
        if ((codeData.code == 0) && (!strcmp(codeData.data, "ENDSEC"))) {
            if (pCurrentSection && (!strcmp(pCurrentSection->type, "ENTITIES"))) { 
                Entity* pEntity = SAFE_CAST_TO(Entity, stackTop());
                if (pEntity) {
                    pEntity->endCounter = codeData.counter;
                }
                stackPop();
            }
            Section* pSection = SAFE_CAST_TO(Section, stackTop());
            assert(pSection);
            pSection->endCounter = codeData.counter;
            
            stackPop();
            continue;
        }
        
        /* Process Code, Data under Entities Section */
        
        if (pCurrentSection && (!strcmp(pCurrentSection->type, "ENTITIES"))) {
            dxfProcessEntities(&codeData);
            continue;            
        }
        
        /*
            TODO: process other sections
        */
    } 
    return root;   
} 

static Entity* makeEntity() {
    Entity* ret = (Entity*) calloc(1, sizeof(Entity));
    return ret;
}

static Section* makeSection() {
    Section* ret = (Section*) calloc(1, sizeof(Section));
    return ret;
}

static Dxf* makeDxf() {
    Dxf* ret = (Dxf*) calloc(1, sizeof(Dxf));
    return ret;    
}   

static bool isSection(CodeData* pCodeData) {
    if (pCodeData->code != 2) return false;
    if (!strcmp(pCodeData->data, "HEADER")) return true;
    if (!strcmp(pCodeData->data, "TABLES")) return true;
    if (!strcmp(pCodeData->data, "CLASS")) return true;
    if (!strcmp(pCodeData->data, "BLOCKS")) return true;
    if (!strcmp(pCodeData->data, "ENTITIES")) return true;
    if (!strcmp(pCodeData->data, "OBJECTS")) return true;
    return false;
}

void stackPush(StackItem stackItem) {
    assert(stackSize < DEEPEST_STACK);
    objectStack[stackSize] = stackItem.objectPtr;
    objectTypeStack[stackSize] = stackItem.objectType;
    ++ stackSize;
}

bool stackEmpty(void) {
    return stackSize > 0;
}

void stackPop(void) {
    assert(!stackEmpty());
    -- stackSize;
}

StackItem stackTop(void) {
    assert(!stackEmpty());
    StackItem stackItem = {
        objectStack[stackSize - 1],
        objectTypeStack[stackSize - 1]
    };
    return stackItem;    
}

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

void dxfProcessEntities(CodeData* pCodeData) {
    if (pCodeData->code == 0) {
        Entity* pEntity = SAFE_CAST_TO(Entity, stackTop());
        if (pEntity) {
            pEntity->endCounter = pCodeData->counter;
            stackPop();
        }
        
        Section* entities = SAFE_CAST_TO(Section, stackTop());
        assert(entities);
        
        Entity* newEntity = makeEntity();       
        StackItem newItem = {newEntity, Entity_OBJ};
        stackPush(newItem);
        
        if (entities->pEntityHead) {
            entities->pEntityTail->next = newEntity;
            entities->pEntityTail = newEntity;
        } else {
            entities->pEntityHead = entities->pEntityTail = newEntity; 
        }
        return;
    }

    Entity* entity = SAFE_CAST_TO(Entity, stackTop());
    switch (pCodeData->code) {
    case 5:
        strcpy(entity->handle, pCodeData->data);
        break;
    case 100:
        strcpy(entity->subclass_maker, pCodeData->data);
        break;
    case 8:
        strcpy(entity->layer_name, pCodeData->data);
        break;
    case 6:
        strcpy(entity->linetype_name, pCodeData->data);
        break;
    case 62:
        strcpy(entity->color_number, pCodeData->data);
        break;
    case 370:
        strcpy(entity->line_weight, pCodeData->data);
        break;
    default:
        if (pCodeData->code >= 10 && pCodeData->code <= 18) {
            entity->points_x[entity->numOfPoins] = atof(pCodeData->data);
            ++ entity->numOfPoins;        
        } else if (pCodeData->code >= 20 && pCodeData->code <= 28) {
            entity->points_y[entity->numOfPoins] = atof(pCodeData->data);    
        } else if (pCodeData->code >= 30 && pCodeData->code <= 38) {
            entity->points_z[entity->numOfPoins] = atof(pCodeData->data);
        }
    }
}
