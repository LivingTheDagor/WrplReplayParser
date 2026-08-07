#include "res/grpManager.h"

#include "res/grpLoader.h"
#include <filesystem>
#include <iostream>
#include "tracy/Tracy.hpp"

namespace fs = std::filesystem;

struct grpRef {
  GrpData *grp;
  FullFileLoadCB cb;
  grpRef(const fs::path &name) : grp(nullptr), cb{name.string()} {
    ZoneScopedN("grpRef::grpRef");
    grp = parseGrp(cb);
  }
  ~grpRef() {
    ZoneScopedN("grpRef::~grpRef");
    free(grp);
  }
};

bool doesItemApply(std::string_view item) {
  ZoneScoped;
  // must only be 'main' skeleton
  if (item.ends_with("_skeleton") && !item.ends_with("_dm_skeleton") && !item.ends_with("_dmg_skeleton") &&
      !item.ends_with("_xray_skeleton")) {
    return true;
  }
  return false;
}

size_t getEntrySize(GrpData *data, const ResData *curr, size_t file_size) {
  ZoneScoped;
  auto &entry = data->resTable[curr->resId];
  if (curr->resId + 1 == data->resTable.size()) {
    return file_size - entry.offset;
  }
  auto &next_entry = data->resTable[(curr->resId + 1)];
  return next_entry.offset - entry.offset;
}


grpManager::grpManager() {}
void grpManager::initialize(const fs::path &fs_path) {
  ZoneScopedN("grpManager::initialize");
  if (!fs::exists(fs_path)) {
    LOGE("grpManager::initialize: path does not exist: {}", fs_path.string());
    return;
  }
  for (const auto &fs_entry: fs::directory_iterator(fs_path)) {
    ZoneScopedN("grpManager::initialize::parse_fs_entry");
    if (fs_entry.is_regular_file()) {
      const fs::path &file = fs_entry.path();
      if (file.extension() == ".grp") {
        grpRef grp_data{file};
        auto grp = grp_data.grp;
        const ResData *rd = grp->resData.data(), *rd_end = rd + grp->resData.size();
        for (; rd != rd_end; rd++) {
          auto name = grp->getName(rd->resId);
          if (!doesItemApply(name))
            continue;
          auto &entry = grp->resTable[rd->resId];
          DG_ASSERT(entry.classId == rd->classId);
          // for now, we are only parsing this, so lets make sure of that
          // our previous 'doesItemApply' should make this not happen anyways
          DG_ASSERT(entry.classId == GeomNodeTreeGameResClassId);
          size_t item_size = getEntrySize(grp, rd, grp_data.cb.fileHandle->length());
          void *ptr = malloc(item_size);
          grp_data.cb.seekto(entry.offset);
          grp_data.cb.read(ptr, (int) item_size);
          this->data.push_back(ptr);
          this->res_map[name] = {this->data.size() - 1, item_size, entry.classId};
          // LOGI("loaded skeleton: {} from grp: {}", name, file.string());
        }
      }
    }
  }
}
grpManager::~grpManager() {
  for (auto d: this->data) {
    free(d);
  }
}
bool grpManager::getTree(const std::string &name, GeomNodeTree &load_into) {
  auto entry = this->res_map.find(name);
  if (entry == this->res_map.end()) {
    return false;
  }
  DG_ASSERT(entry->second.class_id == GeomNodeTreeGameResClassId);
  MemGeneralLoadCB cb{this->data[entry->second.index], static_cast<int>(entry->second.size)};
  load_into.load(cb);
  return true;
}

grpManager g_grp_manager{};
