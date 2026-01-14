#include "init/initialize.h"
#include "FileSystem.h"
#include "ecs/ComponentTypesDefs.h"
#include "mpi/ObjectDispatcher.h"
#include "state/ParserState.h"

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



// runs basic init steps
void initialize(std::string &VromfsPath) {
  register_listener(&mpi::base);
  mpi::register_object_dispatcher(&mpi::ObjectDispatcher);
  ecs::g_ecs_data.Init();
  fs::path basePath = VromfsPath;
  std::string p1 = (basePath / "aces.vromfs.bin").string();
  std::string p2 = (basePath / "game.vromfs.bin").string();
  EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p1), "{} does not exist", p1);
  EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p2), "{} does not exist", p2);
  parseTemplates();
  //ecs::g_ecs_data->getTemplateDB()->DebugPrintTemplate("medic_box_item");
  hello();
  //mpi::players.hello();
}
