#include <chrono>
#include <iostream>
#include <thread>
#include "FileSystem.h"
#include "ecs/ComponentTypesDefs.h"
#include "init/initialize.h"
#include "mpi/ObjectDispatcher.h"
#include "mpi/codegen/ReflIncludes.h"
#include "state/ParserState.h"
#include "translate.h"

namespace mpi {
  class BaseListener : public IMessageListener {
    void receiveMpiMessage(const Message *msg, SystemID receiver) override { msg->obj->applyMpiMessage(msg); }
  };
  class DummyObject : public IObject {
    Message *dispatchMpiMessage(MessageID mid) override { return nullptr; }
    void applyMpiMessage(const Message *m) override {}
  };

  static BaseListener base;
  static DummyObject dummy;
  IObject *dispatcher(ObjectID, ObjectExtUID) { return &dummy; }
} // namespace mpi

bool TranslationAllowed = false;

void print_all_files_full_paths(const fs::path &root = fs::current_path()) {
  for (const auto &entry: fs::recursive_directory_iterator(root)) {
    if (entry.is_regular_file()) {
      std::cout << fs::absolute(entry.path()).string() << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}
static bool has_init = false;
// runs basic init steps
void initialize(const std::string &game_path, const std::string &logfile_path, bool fonts, bool lang, bool mis) {
  if (has_init)
    return;
  has_init = true;
  ZoneScoped;
  // print_all_files_full_paths();
  if (!logfile_path.empty())
    g_log_handler->set_default_sink_logfile(logfile_path);
  g_log_handler->start_thread();
  register_default_sigsev_handler();
  register_listener(&mpi::base);
  file_mgr.add_mount(game_path);
  mpi::register_object_dispatcher(&mpi::ObjectDispatcher);
  std::string p1 = ("aces.vromfs.bin");
  std::string p2 = ("game.vromfs.bin");
  std::string p3 = ("mis.vromfs.bin");
  std::string p4 = ("lang.vromfs.bin");
  std::string p5 = ("char.vromfs.bin");
  if (fonts) {
    std::string p6 = ("ui/fonts.vromfs.bin");
    EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p6), "{} does not exist", p6);
  }
  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p1), "{} does not exist", p1);
  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p2), "{} does not exist", p2);
  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p5), "{} does not exist", p5);
  if (mis)
    file_mgr.mountVromfs(p3); // optional
  if (lang)
    TranslationAllowed = file_mgr.mountVromfs(p4);
  if (TranslationAllowed) {
    translate::load_csv("lang/units_modifications.csv");
    translate::load_csv("lang/units.csv");
    translate::load_csv("lang/units_weaponry.csv");
    file_mgr.unmountVromfs("lang.vromfs.bin");
  }
  hello();
  force_link_replication();
  force_link_cnet();
  G_ASSERT(dblk::load(ecs::g_ecs_data->wp_cost, "config/wpcost.blk"));
  // mpi::players.hello();
  ecs::g_ecs_data.get();
  size_t pull_val = framework_primary_pulls;
  G_UNUSED(pull_val);
}
