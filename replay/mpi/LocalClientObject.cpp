#include "state/ParserState.h"
#include "mpi/LocalClientObject.h"

namespace mpi {
  Message *LocalClientObject::dispatchMpiMessage(MessageID mid) {
    if (mid == MPI_PACKETS::ReplayCockpitParams)
      return state->_new<CockpitMessage>(this);
    return nullptr;
  }

  void LocalClientObject::applyMpiMessage(const Message *m) {
    if (m->id == MPI_PACKETS::ReplayCockpitParams)
      if (state->keep.cockpit)
        state->CockpitEvents.push_back(((const CockpitMessage *) m)->cockpit);
  }

  bool CockpitMessage::readPayload(ParserState *state) {
    cockpit.time_ms = state->curr_time_ms;
    BitStream nested;
    uint16_t count = 0;
    if (!payload.Read(nested) || !nested.Read(count) ||
        count > nested.GetNumberOfUnreadBits() / (8 * (sizeof(uint16_t) + sizeof(float))))
      return false;
    cockpit.params.reserve(count);
    for (uint16_t i = 0; i < count; i++) {
      uint16_t id = 0;
      float value = 0;
      if (!nested.Read(id) || !nested.Read(value))
        return false;
      cockpit.params.emplace_back(id, value);
    }
    return true;
  }
} // namespace mpi
