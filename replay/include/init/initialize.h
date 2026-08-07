

#ifndef MYEXTENSION_INITIALIZE_H
#define MYEXTENSION_INITIALIZE_H
#include <string>

extern bool TranslationAllowed;

void initialize(const std::string &game_path, const std::string &grp_dir = "", const std::string &logfile_path = "",
                bool fonts = false, bool lang = true, bool mis = true);


#endif // MYEXTENSION_INITIALIZE_H
