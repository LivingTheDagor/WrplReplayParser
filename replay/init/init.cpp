#include <chrono>
#include <iostream>
#include <thread>
#include "FileSystem.h"
#include "VROMFs.h"
#include "ecs/componentTypesDefs.h"
#include "init/initialize.h"
#include "mpi/ObjectDispatcher.h"
#include "mpi/GMSyncPrescan.h"
#include "mpi/codegen/ReflIncludes.h"
#include "state/ParserState.h"
#include "translate.h"
#include "res/grpManager.h"
#include <future>
#if defined(__GLIBC__)
#include <malloc.h>
#endif

size_t get_state_pulls();

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

std::shared_future<void> load_vromfs_async(const std::string &vromfs_path, bool required) {
  if (!file_mgr.getFile(vromfs_path)) {
    G_CHECKF(!required, "{} does not exist", vromfs_path);
    return {};
  }
  return std::async(std::launch::async, [vromfs_path] {
    TracyMessage(vromfs_path.c_str(), vromfs_path.size());
    auto vromfs = std::make_unique<VROMFs>(vromfs_path);
    file_mgr.addVromfs(std::move(vromfs));
  });
}

namespace translate {
  void add_allowed_language(Languages lang);
}

static bool has_init = false;
// runs basic init steps
void initialize(const InitOptions &options) {
  if (has_init)
    return;
#if defined(__GLIBC__)
  if (options.tune_malloc) {
    // see InitOptions::tune_malloc. Done first, so the allocations of init profit as well.
    mallopt(M_TOP_PAD, 64 << 20);
    mallopt(M_MMAP_THRESHOLD, 32 << 20);
    mallopt(M_TRIM_THRESHOLD, 256 << 20);
    // One malloc arena for all threads. The loading threads of init, the decode-ahead thread and the sync
    // prescan threads would each get an arena of their own; with the trim threshold above, the memory one
    // of them frees stays with that arena and is of no use to the others (about 100 MB per process).
    mallopt(M_ARENA_MAX, 1);
  }
#endif
  g_log_handler.initialize();
  file_mgr.in_init = true;
  LOGI("init started");
  has_init = true;
  ZoneScoped;
  for (const auto lang: options.allowed_languages) {
    translate::add_allowed_language(lang);
  }
  file_mgr.add_mount(options.game_path);
  std::future<void> grp_init;
  // VROMFs vfrs{"content/base/res/grp_hdr.vromfs.bin"};
  // auto dir = vfrs.getDirectory();
  // auto tanks = dir.getDirectory("tanks");
  // auto idx = tanks->getFile("ussr_zsu_57_2.grpcache.bin");
  // auto raw = idx->getFile(idx)->readRaw();
  //  dir.print(0);
  if (options.load_grps) {
    const fs::path grp_path =
      options.grp_dir.empty() ? fs::path(options.game_path) / "content" / "base" / "res" : fs::path(options.grp_dir);

    grp_init = std::async(std::launch::async, [grp_path] { g_grp_manager.initialize(grp_path); });
  }

  if (!options.logfile_path.empty())
    g_log_handler->set_default_sink_logfile(options.logfile_path);
  g_log_handler->start_thread();
  register_default_sigsev_handler();
  register_listener(&mpi::base);
  mpi::register_object_dispatcher(&mpi::MpiQueueObject::ObjectDispatcher);
  gmsync::enable(options.sync_prescan);
  // p1, p2, and p5 are required, p3 and p4 are optional

  const std::string aces_v = ("aces.vromfs.bin");
  const std::string game_v = ("game.vromfs.bin");
  const std::string mis_v = ("mis.vromfs.bin");
  const std::string lang_v = ("lang.vromfs.bin");
  const std::string char_v = ("char.vromfs.bin");
  const std::string fonts_v = ("ui/fonts.vromfs.bin");
  std::shared_future<void> lang_future;
  std::shared_future<void> font_future;
  if (options.fonts) {
    font_future = load_vromfs_async(fonts_v, false);
  }
  if (options.lang) {
    lang_future = load_vromfs_async(lang_v, false);
    if (lang_future.valid()) {
      TranslationAllowed = true;
    }
  }
  auto aces_future = load_vromfs_async(aces_v, true);

  std::shared_future<void> mis_future;
  if (options.mis)
    mis_future = load_vromfs_async(mis_v, false);

  auto game_future = load_vromfs_async(game_v, true);
  auto char_future = load_vromfs_async(char_v, true);

  std::future<void> translate_future{};
  std::shared_future<void> g_ecs_future{};
  std::future<void> blk_future{};
  if (TranslationAllowed) {
    translate_future = std::async(std::launch::async, [&lang_future] {
      G_ASSERT(lang_future.valid());
      lang_future.get();
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
    });
  }
  hello();
  force_link_replication();
  force_link_cnet();

  g_ecs_future = std::async(std::launch::async, [&aces_future, &game_future] {
    G_ASSERT(game_future.valid());
    G_ASSERT(aces_future.valid());
    game_future.get();
    aces_future.get();
    ecs::g_ecs_data.initialize();
  });
  blk_future = std::async(std::launch::async, [&char_future, &g_ecs_future] {
    G_ASSERT(char_future.valid());
    char_future.get();
    DataBlock wp_cost{};
    DataBlock unit_tags{};
    G_ASSERT(dblk::load(wp_cost, "config/wpcost.blk"));
    G_ASSERT(dblk::load(unit_tags, "config/unittags.blk"));
    G_ASSERT(g_ecs_future.valid());
    g_ecs_future.get();
    ecs::g_ecs_data->wp_cost = std::move(wp_cost);
    ecs::g_ecs_data->unit_tags = std::move(unit_tags);
  });
  // mpi::players.hello();
  size_t pull_val = get_state_pulls();
  G_UNUSED(pull_val);
  if (translate_future.valid())
    translate_future.get();
  if (g_ecs_future.valid())
    g_ecs_future.get();
  if (blk_future.valid())
    blk_future.get();

  if (mis_future.valid())
    mis_future.get();
  if (grp_init.valid())
    grp_init.get();
  if (font_future.valid())
    font_future.get();
  file_mgr.in_init = false;
  file_mgr.unmountVromfs("lang.vromfs.bin");
  LOGI("init finished");
}
