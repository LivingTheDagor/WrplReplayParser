#include "res/grpLoader.h"

void GrpData::patchDescOnly(uint32_t hdr_label)
{
  void *base = dumpBase();
  nameMap.patch(base);
  resTable.patch(base);
  resData.patch(base);
  if (hdr_label == _MAKE4C('GRP2'))
    ResData::cvtRecInplaceVer2to3(resData.data(), resData.size());
}
void GrpData::patchData()
{
  void *base = dumpBase();
  ResData *rd = resData.data(), *rd_end = rd + resData.size();
  for (; rd != rd_end; rd++)
    rd->refResIdPtr.patchNonNull(base);
}

char *GrpData::dumpBase() const { return ((char *)this) - sizeof(GrpHeader); }

GrpData * parseGrp(IGenLoad &crd) {
  ZoneScoped;
  GrpHeader hdr{};
  crd.readInto(hdr);
  DG_ASSERT(hdr.label == _MAKE4C('GRP2'));
  GrpData * data;
  data = (GrpData*) malloc(hdr.fullDataSize);
  crd.read(data, (int)hdr.fullDataSize);
  data->patchDescOnly(hdr.label);
  data->patchData();
  return data;
}