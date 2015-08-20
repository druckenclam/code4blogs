#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dxf_utility.h"

/* This is a quick prototype for me to understand DXF format. */

#define DEBUG 0

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
    
    double points_x[NUM_OF_POINTS];   
    double points_y[NUM_OF_POINTS];
    double points_z[NUM_OF_POINTS];
    int numOfPointsX;
    int numOfPointsY;
    int numOfPointsZ;
    
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
    char comments[LONGEST_STRING];
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
static Entity* makeEntity(unsigned long long counter);
static Section* makeSection(unsigned long long counter);
static Dxf* makeDxf(void);
static bool isSection(CodeData* pCodeData);
static void stackPush(StackItem stackItem);
static bool stackEmpty(void);
static void stackPop(void);
static StackItem stackTop(void);
static struct CodeData readCodeData(FILE* dxfFile, bool disp);
static void dxfProcessEntities(CodeData* pCodeData);
void sectionConvert2JSON(Section* pSection, int indentLevel, bool last);
void entityConvert2JSON(Entity* pEntity, int indentLevel, bool last);

/* External Functions */
void dxfTokenizeDocument(FILE* dxfFile) {
    while (1) {
        CodeData codeData = readCodeData(dxfFile, true);   
        if ((codeData.code == 0) && (!strcmp(codeData.data, "EOF"))) {
            break;
        }
    }    
}

Dxf* dxfProcessDocument(FILE* dxfFile) {
    Dxf* root = makeDxf();
    StackItem stackItem = {root, Dxf_OBJ};
    stackPush(stackItem);
    
    Section* pCurrentSection = NULL;
    while (1) {
        CodeData codeData = readCodeData(dxfFile, false);   
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
            Section* newSection = makeSection(codeData.counter);
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
                    pEntity->endCounter = codeData.counter - 1;
                }
                stackPop();
            }
            Section* pSection = SAFE_CAST_TO(Section, stackTop());
            assert(pSection);
            pSection->endCounter = codeData.counter;
            
            stackPop();
            continue;
        }
        
        /* Store Section Type */
        if (isSection(&codeData)) {
            Section* pSection = SAFE_CAST_TO(Section, stackTop());
            assert(pSection);
            strcpy(pSection->type, codeData.data);
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

void dxfConvert2HTML(Dxf* pDxf) {
    Section* pSection = NULL;
    assert(pDxf);
    for (pSection = pDxf->pSectionHead; pSection; pSection = pSection->next) {
        Entity* pEntity = NULL;
        if (strcmp(pSection->type, "ENTITIES")) continue;
        printf(
            "<!DOCTYPE html>\n"
            "<html>\n"
            "   <header><title>Convert DXF file to HTML5 Canvas Drawing</title></header>\n"
            "   <body>\n"
            "       <canvas id=\"myCanvas\" width=\"640\" height=\"480\" style=\"border:5px;\">\n"
            "       Your browser does not support the HTML5 canvas tag.</canvas>\n"
            "       <script>\n"
            "           var c = document.getElementById(\"myCanvas\");\n"
            "           var ctx = c.getContext(\"2d\");\n"
        );
        for (pEntity = pSection->pEntityHead; pEntity; pEntity = pEntity->next) {
            if (strcmp(pEntity->type, "LINE")) continue;
            /* We are dealing with LINE here */
            /* Asume only two points for a LINE */
            printf("           ctx.moveTo(%d, %d)\n", (int)pEntity->points_x[0], 
                (int)pEntity->points_y[0]);
            printf("           ctx.lineTo(%d, %d)\n", (int)pEntity->points_x[1], 
                (int)pEntity->points_y[1]);
        }
        printf(
            "           ctx.stroke();\n"
            "       </script>\n"
            "   </body>\n"
            "</html>\n"
        );
        /* There is only one ENTITIES section */
        break;    
    }    
}

void dxfConvert2JSON(Dxf* pDxf) {
    assert(pDxf);
    Section* pSection = NULL;
    printf("{\n");
    printf("  \"comments\": \"%s\",\n", pDxf->comments);
    printf("  \"sections\": [\n");
    for (pSection = pDxf->pSectionHead; pSection; pSection = pSection->next) {
        sectionConvert2JSON(pSection, 2, pSection == pDxf->pSectionTail);    
    }
    printf("  ]\n");
    printf("}\n");
}

// Local Function definitions
void sectionConvert2JSON(Section* pSection, int indentLevel, bool last) {
    char space = ' ';
    Entity* pEntity = NULL;
    printf("%*c{\n", indentLevel * 2, space);
    printf("%*c\"_end_counter\": %llu,\n", (indentLevel + 1) * 2, space, 
        pSection->endCounter);
    printf("%*c\"_start_counter\": %llu,\n", (indentLevel + 1) * 2, space, 
        pSection->startCounter);
    if (!strcmp(pSection->type, "ENTITIES")) {
        printf("%*c\"entities\": [\n", (indentLevel + 1) * 2, space);
        for (pEntity = pSection->pEntityHead; pEntity; pEntity = pEntity->next) {
            entityConvert2JSON(pEntity, indentLevel + 2, 
                pEntity == pSection->pEntityTail); 
        }
        printf("%*c],\n", (indentLevel + 1) * 2, space);     
    } else {
        printf("%*c\"entities\": [],\n", (indentLevel + 1) * 2, space);
    }
    printf("%*c\"type\": \"%s\"\n", (indentLevel + 1) * 2, space, pSection->type);
    printf("%*c}", indentLevel * 2, space);
    if (last) {
        printf("\n");
    } else {
        printf(",\n");
    }   
}

void entityConvert2JSON(Entity* pEntity, int indentLevel, bool last) {
    char space = ' ';
    printf("%*c{\n", indentLevel * 2, space);
    printf("%*c\"_end_counter\": %llu,\n", (indentLevel + 1) * 2, space, 
        pEntity->endCounter);
    printf("%*c\"_start_counter\": %llu,\n", (indentLevel + 1) * 2, space, 
        pEntity->startCounter);
    printf("%*c\"color_number\": \"%s\",\n", (indentLevel + 1) * 2, space, 
        pEntity->color_number);
    printf("%*c\"handle\": \"%s\",\n", (indentLevel + 1) * 2, space, 
        pEntity->handle);
    printf("%*c\"layer_name\": \"%s\",\n", (indentLevel + 1) * 2, space, 
        pEntity->layer_name);
    printf("%*c\"linetype_name\": \"%s\",\n", (indentLevel + 1) * 2, space, 
        pEntity->linetype_name);
    printf("%*c\"line_weight\": \"%s\",\n", (indentLevel + 1) * 2, space, 
        pEntity->line_weight);
    if (!strcmp(pEntity->type, "LINE")) {
        int i;
        printf("%*c\"points_x\": {\n", (indentLevel + 1) * 2, space);
        for (i = 0; i < pEntity->numOfPointsX; ++i) {
            printf("%*c\"%d\": %lf", (indentLevel + 2) * 2, space, i + 10, 
                pEntity->points_x[i]);
            if (i == pEntity->numOfPointsX - 1) {
                printf("\n");
            } else {
                printf(",\n");
            }    
        }
        printf("%*c},\n", (indentLevel + 1) * 2, space);
        printf("%*c\"points_y\": {\n", (indentLevel + 1) * 2, space);
        for (i = 0; i < pEntity->numOfPointsY; ++i) {
            printf("%*c\"%d\": %lf", (indentLevel + 2) * 2, space, i + 20, 
                pEntity->points_y[i]);
            if (i == pEntity->numOfPointsY - 1) {
                printf("\n");
            } else {
                printf(",\n");
            }     
        }
        printf("%*c},\n", (indentLevel + 1) * 2, space);
    }
    printf("%*c\"subclass_maker\": \"%s\",\n", (indentLevel + 1) * 2, space, 
        pEntity->subclass_maker);
    printf("%*c\"type\": \"%s\"\n", (indentLevel + 1) * 2, space, pEntity->type);
    printf("%*c}", indentLevel * 2, space);
    if (last) {
        printf("\n");
    } else {
        printf(",\n");
    }  
}

/* Release memory */
void dxfDestroy(Dxf* pDxf) {
    /* TODO: Traverse objects and free memory */
}

Entity* makeEntity(unsigned long long counter) {
    Entity* ret = (Entity*) calloc(1, sizeof(Entity));
    ret->startCounter = counter;
    return ret;
}

Section* makeSection(unsigned long long counter) {
    Section* ret = (Section*) calloc(1, sizeof(Section));
    ret->startCounter = counter;
    return ret;
}

Dxf* makeDxf() {
    Dxf* ret = (Dxf*) calloc(1, sizeof(Dxf));
    return ret;    
}   

bool isSection(CodeData* pCodeData) {
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
    return stackSize == 0;
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

struct CodeData readCodeData(FILE* dxfFile, bool disp) {
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
    
    codeData.code = atoi(codeData.data);
    /*
        Read data
    */
    ret = fgets(codeData.data, LONGEST_STRING, dxfFile);
    assert(ret);
    codeData.data[strlen(codeData.data) - 1] = '\0';
    
    if (disp) {
        printf("%llu: \"%d\" = \"%s\"\n", codeData.counter, codeData.code, 
            codeData.data);
    }
    
    ++counter;
    return codeData;
}

void dxfProcessEntities(CodeData* pCodeData) {
    if (pCodeData->code == 0) {
        Entity* pEntity = SAFE_CAST_TO(Entity, stackTop());
        if (pEntity) {
            pEntity->endCounter = pCodeData->counter - 1;
            stackPop();
        }
        
        Section* entities = SAFE_CAST_TO(Section, stackTop());
        assert(entities);
        
        Entity* newEntity = makeEntity(pCodeData->counter);
        strcpy(newEntity->type, pCodeData->data);        
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
            entity->points_x[entity->numOfPointsX] = atof(pCodeData->data);
            ++ entity->numOfPointsX;        
        } else if (pCodeData->code >= 20 && pCodeData->code <= 28) {
            entity->points_y[entity->numOfPointsY] = atof(pCodeData->data);
            ++ entity->numOfPointsY;    
        } else if (pCodeData->code >= 30 && pCodeData->code <= 38) {
            entity->points_z[entity->numOfPointsZ] = atof(pCodeData->data);
            ++ entity->numOfPointsZ;
        }
    }
}
