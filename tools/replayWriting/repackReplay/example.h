#pragma once
#include <daScript/daScript.h>

#include "Logger.h"


#include <iostream>

// A normal C++ function exposed to Daslang.
inline void replay_print(const char *message) { std::cout << "[ReplayParser] " << message << '\n'; }

class Module_Replay : public das::Module {
public:
  Module_Replay() : Module("replay") {
    das::ModuleLibrary lib(this);
    // Expose the C++ function.
    das::addExtern<DAS_BIND_FUN(replay_print)>(*this, lib, "print_message", das::SideEffects::modifyExternal,
                                               "replay_print")
      ->args({"message"});
  }
};

REGISTER_MODULE(Module_Replay);

class VirtualFileAccess : public das::FsFileAccess {
public:
  // VirtualFileSystem &vfs;

  // explicit VirtualFileAccess(VirtualFileSystem &fs) : vfs(fs) {}

  das::FileInfo *getNewFileInfo(const das::string &fileName) override {
    LOG("VirtualFileAccess::getNewFileInfo: {}", fileName);
    // First: check your VFS.
    // if (auto file = vfs.open(fileName)) {
    // return makeFileInfoFromVfsFile(*file);
    //}

    // Second: fall back to normal filesystem.
    return das::FsFileAccess::getNewFileInfo(fileName);
  }
};
