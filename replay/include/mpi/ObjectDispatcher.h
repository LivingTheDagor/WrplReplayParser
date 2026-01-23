

#ifndef MYEXTENSION_OBJECTDISPATCHER_H
#define MYEXTENSION_OBJECTDISPATCHER_H

#include "codegen/ReflIncludes.h"
#include "types.h"

struct ParserState;
namespace mpi
{

  struct MainDispatch : public IObject {
    ParserState *state;
    MainDispatch(ParserState *state) : IObject(0x5802) { this->state=state;}
    enum MainEnum {
      SevereDamage = 0xf157,
      CriticalDamage = 0xf056,
      Kill = 0xf058,
      Awards = 0xf078,
      Action = 0xf028,
      Replication = 0xd039,
      ReflectionNoDecompress = 0xf0aa,
      Reflection1 = 0xf02d,
      Reflection2 = 0xd136,
      ACTUALLY_NOT_REFLECTION = 0xd137,
      Replicable = 0xd039,

    };
    Message *dispatchMpiMessage(MessageID mid) override;
    void applyMpiMessage(const Message *m) override;
    ~MainDispatch() override = default;
  };
  IObject * ObjectDispatcher(ObjectID oid, ObjectExtUID extUid,  ParserState*state);

}



#endif //MYEXTENSION_OBJECTDISPATCHER_H
