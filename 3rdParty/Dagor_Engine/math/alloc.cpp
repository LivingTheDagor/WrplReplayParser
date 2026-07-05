#include <cstdlib>
#include "memory/dag_genMemAlloc.h"

GLOBAL_ALLOC G_ALLOC;

#define DECL_MEM(NM) IMemAlloc *NM = &G_ALLOC //-V1003

DECL_MEM(stdmem);
DECL_MEM(tmpmem);
DECL_MEM(inimem);
DECL_MEM(midmem);
DECL_MEM(strmem);
DECL_MEM(defaultmem);
DECL_MEM(uimem);
DECL_MEM(scriptmem);
DECL_MEM(globmem);