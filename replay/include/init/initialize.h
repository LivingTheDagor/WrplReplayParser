

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
};

void initialize(const InitOptions &options);


#endif // MYEXTENSION_INITIALIZE_H
