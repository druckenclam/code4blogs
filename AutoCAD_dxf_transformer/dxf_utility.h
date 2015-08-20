#ifndef DXF_UTILITY_H
#define DXF_UTILITY_H
typedef struct Dxf Dxf;
Dxf* dxfProcessDocument(FILE* dxfFile);
void dxfTokenizeDocument(FILE* dxfFile);

void dxfConvert2JSON(Dxf* pDxf);
void dxfConvert2HTML(Dxf* pDxf);
void dxfDestroy(Dxf* pDxf);
#endif
