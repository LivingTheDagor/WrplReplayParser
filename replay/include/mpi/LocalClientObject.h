#pragma once
#include "mpi.h"
#include "GeneralObject.h"
#include <utility>
#include <vector>

namespace mpi {
  struct CockpitEvent {
    uint32_t time_ms = 0;
    std::vector<std::pair<uint16_t, float>> params{};
  };

  class CockpitMessage : public Message {
    bool readPayload(ParserState *state) override;

  public:
    CockpitMessage(IObject *o) : Message(o, MPI_PACKETS::ReplayCockpitParams) {}
    CockpitEvent cockpit{};
  };

  struct LocalClientObject : public IObject {
    LocalClientObject(ParserState *state) : IObject(state, 0x5803) {}
    Message *dispatchMpiMessage(MessageID mid) override;
    void applyMpiMessage(const Message *m) override;
    ~LocalClientObject() override = default;
  };
} // namespace mpi
