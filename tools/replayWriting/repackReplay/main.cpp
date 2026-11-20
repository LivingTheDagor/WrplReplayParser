
#include "example.h"

#include <iostream>

int main() {
  g_log_handler.initialize();
  // Register built-in modules and initialize Daslang.
  // NEED_ALL_DEFAULT_MODULES;
  NEED_MODULE(Module_BuiltIn);
  NEED_MODULE(Module_Replay);
  das::Module::Initialize();
  das::setDasRoot("D:/ReplayParser/3rdParty/daScript");

  // Compiler output.
  das::TextPrinter tout;

  // File access and module group.
  das::ModuleGroup libGroup{};
  auto fileAccess = das::make_smart<VirtualFileAccess>();

  // Compile the script.
  auto program =
    das::compileDaScript("D:/ReplayParser/tools/replayWriting/repackReplay/hello.das", fileAccess, tout, libGroup);

  if (program->failed()) {
    std::cerr << "Failed to compile script:\n";

    for (auto &err: program->errors) {
      tout << das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
    }

    libGroup.reset();
    das::Module::Shutdown();
    return 1;
  }

  // Create execution context.
  das::Context ctx(program->getContextStackSize());

  // Simulate (initialize) the compiled program.
  if (!program->simulate(ctx, tout)) {
    std::cerr << "Failed to initialize script.\n";

    for (auto &err: program->errors) {
      tout << das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
    }

    libGroup.reset();
    das::Module::Shutdown();
    return 1;
  }

  // Find and execute exported main().
  auto fn = ctx.findFunction("main");

  if (!fn) {
    std::cerr << "Could not find exported main() function.\n";
    libGroup.reset();
    das::Module::Shutdown();
    return 1;
  }

  ctx.evalWithCatch(fn, nullptr);

  if (auto exception = ctx.getException()) {
    std::cerr << "Runtime exception: " << exception << '\n';
    libGroup.reset();
    das::Module::Shutdown();
    return 1;
  }
  libGroup.reset();
  das::Module::Shutdown();
  return 0;
}
