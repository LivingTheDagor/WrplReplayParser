#include "init/initialize.h"
#include "FileSystem.h"
#include "ecs/ComponentTypesDefs.h"
#include "mpi/ObjectDispatcher.h"
#include "state/ParserState.h"
#include "mpi/codegen/ReflIncludes.h"

namespace mpi {
  class BaseListener : public IMessageListener {
    void receiveMpiMessage(const Message *msg, SystemID receiver) override {
      msg->obj->applyMpiMessage(msg);
    }
  };
  class DummyObject : public IObject {
    Message *dispatchMpiMessage(MessageID mid) override{return nullptr;}
    void applyMpiMessage(const Message *m) override {}
  };

  static BaseListener base;
  static DummyObject dummy;
  IObject * dispatcher(ObjectID, ObjectExtUID) {
    return &dummy;
  }
}

bool TranslationAllowed = false;

// runs basic init steps
void initialize(const std::string &VromfsPath, const std::string &logfile_path, bool fonts, bool lang, bool mis) {
  ZoneScoped;
  if(!logfile_path.empty())
    g_log_handler->set_default_sink_logfile(logfile_path);
  register_default_sigsev_handler();
  register_listener(&mpi::base);
  mpi::register_object_dispatcher(&mpi::ObjectDispatcher);
  fs::path basePath = VromfsPath;
  std::string p1 = (basePath / "aces.vromfs.bin").string();
  std::string p2 = (basePath / "game.vromfs.bin").string();
  std::string p3 = (basePath / "mis.vromfs.bin").string();
  std::string p4 = (basePath / "lang.vromfs.bin").string();
  std::string p5 = (basePath / "char.vromfs.bin").string();
  if(fonts) {
    std::string p6 = (basePath / "ui/fonts.vromfs.bin").string();
    EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p6), "{} does not exist", p6);
  }
  EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p1), "{} does not exist", p1);
  EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p2), "{} does not exist", p2);
  EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p5), "{} does not exist", p2);
  if(mis)
    file_mgr.loadVromfs(p3); // optional
  if (lang)
    TranslationAllowed = file_mgr.loadVromfs(p4); // optional, TODO
  //ecs::g_ecs_data->getTemplateDB()->DebugPrintTemplate("medic_box_item");
  hello();
  force_link_replication();
  force_link_cnet();
  auto wp_cost_f = file_mgr.getFile("config/wpcost.blk");
  if(wp_cost_f)
    wp_cost_f->loadBlk(ecs::g_ecs_data->wp_cost);
  //mpi::players.hello();
  ecs::g_ecs_data.get();
}
