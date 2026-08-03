#pragma once
#include "ResId.h"
#include "memory/dag_genMemAlloc.h"
#include "math/dag_geomTree.h"
#include "ioSys/dag_fileIo.h"
#include "ioSys/dag_memIo.h"

struct ResEntry
{
  uint32_t classId;
  uint32_t offset;
  uint16_t resId, _resv;
};

struct ResDataV2
{
  uint32_t classId;
  uint16_t resId, _resv;
  uint32_t refResIdOfs, refResIdCnt, _resv1, _resv2;
};
struct ResData
{
  uint32_t classId;
  uint16_t resId, refResIdCnt;
  PatchablePtr<uint16_t> refResIdPtr;

  static void cvtRecInplaceVer2to3(ResData *_data, unsigned size)
  {
    volatile ResData *data = _data; // we update data inplace using overlapping pointers, prevent unsafe optimizations
    for (auto old = (volatile ResDataV2 *)data; size > 0; old++, data++, size--)
    {
      data->classId = old->classId;
      data->resId = old->resId;
      data->refResIdCnt = old->refResIdCnt;
      ((ResData *)data)->refResIdPtr.setPtr((void *)(uintptr_t)old->refResIdOfs);
    }
  }
};

struct GrpData
{
  PatchableTab<int> nameMap;
  PatchableTab<ResEntry> resTable;
  PatchableTab<ResData> resData;

  inline char *dumpBase() const;
  void patchDescOnly(uint32_t hdr_label);
  void patchData();
  const char *getName(int idx) const { return nameMap[idx] != -1 ? dumpBase() + nameMap[idx] : NULL; }
};


struct GrpHeader
{
  uint32_t label;
  uint32_t descOnlySize;
  uint32_t fullDataSize;
  uint32_t restFileSize;
};

GrpData * parseGrp(IGenLoad &crd);

