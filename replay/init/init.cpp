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
#include "res/grpManager.h"

namespace mpi {
  class BaseListener : public IMessageListener {
  public:
    virtual ~BaseListener() = default;
    void receiveMpiMessage(const Message *msg, SystemID receiver) override { msg->obj->applyMpiMessage(msg); }
  };

  static BaseListener base;
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
void initialize(const std::string &game_path, const std::string &grp_dir, const std::string &logfile_path, bool fonts,
                bool lang, bool mis) {
  if (has_init)
    return;
  g_log_handler.initialize();
  LOGI("init started");
  has_init = true;
  ZoneScoped;
  if (grp_dir.empty()) {
    g_grp_manager.initialize((fs::path) game_path / "content" / "base" / "res" / "tanks");
  } else {
    g_grp_manager.initialize(grp_dir);
  }
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
    G_CHECKF(file_mgr.mountVromfs(p6), "{} does not exist", p6);
  }
  G_CHECKF(file_mgr.mountVromfs(p1), "{} does not exist", p1);
  G_CHECKF(file_mgr.mountVromfs(p2), "{} does not exist", p2);
  G_CHECKF(file_mgr.mountVromfs(p5), "{} does not exist", p5);
  if (mis)
    file_mgr.mountVromfs(p3); // optional
  if (lang)
    TranslationAllowed = file_mgr.mountVromfs(p4);
  if (TranslationAllowed) {
    ZoneScopedN("translate");
    translate::load_csv("lang/units_modifications.csv");
    translate::load_csv("lang/units.csv");
    translate::load_csv("lang/units_weaponry.csv");
    // Damage model parts are named here, under armor_class/<part>: driver, engine,
    // transmission, ammo, fuel_tank, cannon_breech and the rest of what a hit breaks.
    translate::load_csv("lang/menu.csv");
    // Awards, under streaks/<award>: the battle feed names them by id alone.
    translate::load_csv("lang/unlocks_streaks.csv");
    // The belt a gun comes with carries no block of its own and so no name of its own,
    // but the game does name it: modification/default_bullets, which it shows as
    // Default. That key lives here and nowhere else.
    translate::load_csv("lang/menu_options.csv");
    translate::load_csv("lang/missions_locations.csv");
    translate::load_csv("lang/missions_dynamic.csv");
    translate::load_csv("lang/missions_versus.csv");
    // Death reasons live here, under death/<reason>. KillMessage carries an index into
    // this list rather than a name, so the list is read in file order as well.
    translate::load_csv("lang/ui.csv");
    mpi::loadDeathReasons();
    file_mgr.unmountVromfs("lang.vromfs.bin");
  }
  hello();
  force_link_replication();
  force_link_cnet();
  ecs::g_ecs_data.initialize();
  G_ASSERT(dblk::load(ecs::g_ecs_data->wp_cost, "config/wpcost.blk"));
  G_ASSERT(dblk::load(ecs::g_ecs_data->unit_tags, "config/unittags.blk"));
  // mpi::players.hello();
  size_t pull_val = framework_primary_pulls;
  G_UNUSED(pull_val);
  LOGI("init finished");
}
