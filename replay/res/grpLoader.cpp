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

inline char *GrpData::dumpBase() const { return ((char *)this) - sizeof(GrpHeader); }

GrpData * parseGrp(IGenLoad &crd) {
  GrpHeader hdr{};
  crd.readInto(hdr);
  G_ASSERT(hdr.label == _MAKE4C('GRP2'));
  GrpData * data;
  data = (GrpData*) malloc(hdr.fullDataSize);
  crd.read(data, (int)hdr.fullDataSize);
  data->patchDescOnly(hdr.label);
  data->patchData();
  return data;
  /*const ResData *rd = data->resData.data(), *rd_end = rd + data->resData.size();
  for (; rd != rd_end; rd++) {
    auto name = data->getName(rd->resId);
    LOGI("resData: class={:#x} resId={} refResIdCnt={} name={}", rd->classId, rd->resId, rd->refResIdCnt, name);
    auto &entry = data->resTable[rd->resId];
    G_ASSERT(entry.classId == rd->classId);

    if (entry.classId == GeomNodeTreeGameResClassId) {
      crd.seekto(entry.offset);
      GeomNodeTree tree{};
      tree.load(crd);
      LOGI("GeomNodeTree loaded: nodes={}", tree.nodeCount());
      for (int i = 0; i < tree.nodeCount(); i++) {
        auto name = tree.getNodeName(dag::Index16(i));
        auto parent_idx = tree.getParentNodeIdx(dag::Index16(i)).index();
        //LOGI("{}; name={}; parent: {}", i, name, parent_idx);
      }
    }
  }*/
}