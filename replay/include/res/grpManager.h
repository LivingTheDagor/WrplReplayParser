#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include "FileSystem.h"
#include "grpLoader.h"

#include <blockAllocator.h>

class GeomNodeTree;
struct GrpData;

class FullFileLoadCB;
class IGenLoad;
struct GrpRef {
  GrpDataPtr data;
  fs::path path;

  GrpRef(IGenLoad &cb, fs::path p);

  FullFileLoadCB getCb();
};

struct DataRef {
  std::unique_ptr<uint8_t[]> raw_data;
  std::atomic<int64_t> data_len = -1; // -1 means not loaded
  std::mutex write_lock;
  GrpRef *parent;
  const ResData *res;
  DataRef(GrpRef *parent_, const ResData *res_id_) : parent(parent_), res(res_id_) {}
  std::span<uint8_t> getLoadData();
};

struct GrpConfig {
  // correspond to various locations in the content/base/res dir. by default, only tank dir is loaded
  bool load_root = false;
  bool load_aircrafts = false;
  bool load_characters = false;
  bool load_equipment = false;
  bool load_ships = false;
  bool load_tanks = true;
};

class grpManager {
  struct resData {
    uint64_t index, size;
    uint64_t class_id;
  };

  void load_files_from_dir(const fs::path &root, const Directory &dir);

public:
  grpManager();
  // fs_path should point to the content/base/res dir. this dir holds the vromfs we will plan to load
  void initialize(const fs::path &fs_path, const GrpConfig &config = {});
  ~grpManager();

  bool getTree(const std::string &name, GeomNodeTree &load_into);

private:
  BlockAllocator<GrpRef, 100> data;
  BlockAllocator<DataRef, 100> refs;
  std::unordered_map<std::string, DataRef *> res_map;
  // std::vector<void *> data{};
  // std::unordered_map<std::string, resData> res_map;
#if LDAG_DBGLEVEL > 0
  bool has_init{};
#endif
};

extern grpManager g_grp_manager;
