#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include "FileSystem.h"

class GeomNodeTree;

class grpManager {
  struct resData {
    uint64_t index, size;
    uint64_t class_id;
  };
public:
  grpManager();
  void initialize(const fs::path &fs_path);
  ~grpManager();

  bool getTree(const std::string &name, GeomNodeTree &load_into);

private:
  std::vector<void*> data{};
  std::unordered_map<std::string, resData> res_map;
#if LDAG_DBGLEVEL > 0
  bool has_init;
#endif
};

extern grpManager g_grp_manager;