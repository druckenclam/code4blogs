#ifndef DXF_UTILITY_H
#define DXF_UTILITY_H
typedef struct Dxf Dxf;
Dxf* dxfProcessDocument(FILE* dxfFile);
void dxfConvert2JSON(Dxf* pDxf);
void dxfTokenizeDocument(FILE* dxfFile);
#endif
