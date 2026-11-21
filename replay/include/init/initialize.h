

#ifndef MYEXTENSION_INITIALIZE_H
#define MYEXTENSION_INITIALIZE_H
#include <string>
#include "res/grpManager.h"

extern bool TranslationAllowed;

struct InitOptions {
  std::string game_path;
  std::string grp_dir;
  bool load_grps = true;

  GrpConfig grp_config;
  std::string logfile_path;
  bool fonts = false;
  bool lang = true;
  bool mis = true;
  // if any entries, then only these languages will be loaded. if empty, all languages will be loaded
  std::vector<Languages> allowed_languages{};
  // glibc only, process wide: let the malloc heap grow in 64 MB steps, keep blocks up to 32 MB on the heap and
  // keep freed heap memory instead of handing it back after every replay part. A parse writes a few hundred MB
  // of history; in small steps every 4 KB page of it is a separate page fault (about 10 % of the run time of
  // one ground battle), in large steps the kernel can back the heap with huge pages (transparent huge pages
  // set to "always"). Costs: up to 64 MB of extra address space, and freed memory stays with the process.
  // Set to false to leave malloc alone (GLIBC_TUNABLES / MALLOC_*_ environment settings then apply as usual).
  bool tune_malloc = true;
};

void initialize(const InitOptions &options);


#endif // MYEXTENSION_INITIALIZE_H
