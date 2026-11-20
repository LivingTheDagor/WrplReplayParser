#include "res/grpManager.h"

#include "VROMFs.h"
#include "res/grpLoader.h"
#include <filesystem>
#include <iostream>
#include <utility>
#include "tracy/Tracy.hpp"

namespace fs = std::filesystem;

bool doesItemApply(std::string_view item) {
  ZoneScoped;
  // The visual skeleton, which carries the turret nodes, and the damage one, whose
  // nodes are what the hit packets number by index. The wreck and the x ray skeletons
  // are of no use here.
  if (!item.ends_with("_skeleton") || item.ends_with("_dmg_skeleton") || item.ends_with("_xray_skeleton"))
    return false;
  return true;
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


void grpManager::load_files_from_dir(const fs::path &root, const Directory &dir) {
  ZoneScoped;
  for (auto &f: dir.objects) {
    if (f.second->getFSObjectType() == isFile) {
      if (!f.first.ends_with(".grpcache.bin"))
        continue;
      auto index = std::dynamic_pointer_cast<FileIndex>(f.second);
      if (!index)
        continue;
      auto file = index->getFile(index);
      std::string_view name_only{f.first.c_str(), f.first.size() - strlen(".grpcache.bin")};
      LFileGeneralLoadCB cb{file.get()};
      auto &grp_data = this->data.getNext(cb, root / fmt::format("{}.grp", name_only));
      G_ASSERT(fs::exists(grp_data.path));
      auto grp = grp_data.data.get();
      const ResData *rd = grp->resData.data(), *rd_end = rd + grp->resData.size();
      for (; rd != rd_end; rd++) {
        auto name = grp->getName(rd->resId);
        if (!doesItemApply(name))
          continue;
        auto &entry = grp->resTable[rd->resId];
        G_ASSERT(entry.classId == rd->classId);
        // for now, we are only parsing this, so lets make sure of that
        // our previous 'doesItemApply' should make this not happen anyways
        G_ASSERT(entry.classId == GeomNodeTreeGameResClassId);
        auto &ref = this->refs.getNext(&grp_data, rd);
        this->res_map[name] = &ref;
      }
    }
  }
}
grpManager::grpManager() {}
void grpManager::initialize(const fs::path &fs_path, const GrpConfig &config) {
  ZoneScopedN("grpManager::initialize");
  if (!fs::exists(fs_path)) {
    LOGE("grpManager::initialize: path does not exist: {}", fs_path.string());
    return;
  }
  auto vromfs_path = fs_path / "grp_hdr.vromfs.bin";
  VROMFs grp_vromfs{vromfs_path.string()};
  auto dir = grp_vromfs.getDirectory();
  if (config.load_root) {
    load_files_from_dir(fs_path, dir);
  }
  if (config.load_aircrafts) {
    auto aircrafts_dir = dir.getDirectory("aircrafts");
    if (aircrafts_dir)
      load_files_from_dir(fs_path / "aircrafts", *aircrafts_dir);
  }
  if (config.load_characters) {
    auto characters_dir = dir.getDirectory("characters");
    if (characters_dir)
      load_files_from_dir(fs_path / "characters", *characters_dir);
  }
  if (config.load_equipment) {
    auto equipment_dir = dir.getDirectory("equipment");
    if (equipment_dir)
      load_files_from_dir(fs_path / "equipment", *equipment_dir);
  }
  if (config.load_ships) {
    auto ships_dir = dir.getDirectory("ships");
    if (ships_dir)
      load_files_from_dir(fs_path / "ships", *ships_dir);
  }
  if (config.load_tanks) {
    auto tanks_dir = dir.getDirectory("tanks");
    if (tanks_dir)
      load_files_from_dir(fs_path / "tanks", *tanks_dir);
  }
}
grpManager::~grpManager() {}
bool grpManager::getTree(const std::string &name, GeomNodeTree &load_into) {
  ZoneScopedN("grpManager::getTree");
  auto entry = this->res_map.find(name);
  if (entry == this->res_map.end()) {
    return false;
  }
  auto data_ = entry->second->getLoadData();
  G_ASSERT(entry->second->res->classId == GeomNodeTreeGameResClassId);
  MemGeneralLoadCB cb{data_.data(), static_cast<int>(data_.size())};
  load_into.load(cb);
  return true;
}

GrpRef::GrpRef(IGenLoad &cb, fs::path p) : data(parseGrp(cb)), path(std::move(p)) {}
FullFileLoadCB GrpRef::getCb() {
  ZoneScopedN("grpRef2::getCb");
  return FullFileLoadCB{path.string()};
}
std::span<uint8_t> DataRef::getLoadData() {
  ZoneScopedN("DataRef::getLoadData");
  int64_t len = data_len.load(std::memory_order_acquire);
  if (len == -1) {
    std::lock_guard<std::mutex> lock(write_lock);
    len = data_len.load(std::memory_order_relaxed);
    if (len == -1) {

      FullFileLoadCB cb = parent->getCb();

      auto size = cb.fileHandle->length();
      auto grp = parent->data.get();
      auto &entry = grp->resTable[res->resId];
      G_ASSERT(entry.classId == res->classId);
      // for now, we are only parsing this, so lets make sure of that
      // our previous 'doesItemApply' should make this not happen anyways
      G_ASSERT(entry.classId == GeomNodeTreeGameResClassId);
      size_t item_size = getEntrySize(grp, res, size);
      cb.seekto(entry.offset);
      raw_data = std::make_unique<uint8_t[]>(item_size);
      cb.read(raw_data.get(), (int) item_size);
      data_len.store(item_size, std::memory_order_release);
      return {raw_data.get(), item_size};
    }
  }
  return {raw_data.get(), (size_t) len};
}
grpManager g_grp_manager{};
