

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
  // glibc only, process wide: grow the malloc heap in 64 MB steps, keep blocks up to 32 MB on the heap and
  // keep freed memory, to avoid a page fault per 4 KB of history and allow huge pages. Costs up to 64 MB
  // of address space, and freed memory stays with the process. false: leave malloc alone.
  bool tune_malloc = true;
  // glibc only, process wide, applied with tune_malloc: M_ARENA_MAX. 1 = one arena for all threads (lowest
  // memory). Set 2 when parsing several replays on threads of one process (faster, about 30 MB more per
  // parse); 0 = glibc default.
  int malloc_arena_max = 1;
  // Process wide: read the ground-model syncs of each decoded part ahead of the parser, on a thread per
  // part (see mpi/GMSyncPrescan.h). Faster with ground vehicles or ships, uses a second core. Same results.
  bool sync_prescan = true;
};

void initialize(const InitOptions &options);


#endif // MYEXTENSION_INITIALIZE_H
