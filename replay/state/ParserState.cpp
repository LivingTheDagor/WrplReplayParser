#include "state/ParserState.h"
#include "ecs/entityId.h"
#include "mpi/PositionSync.h"
#include "utils.h"

bool ChatMessage::FromBS(BitStream &bs) {
  bool ok = true;
  ok &= bs.Read(player_name);
  ok &= bs.Read(message);
  ok &= bs.Read(channel);
  ok &= bs.Read(is_local_message);
  ok &= bs.Read(is_quick_message);
  ok &= bs.Read(complaints);
  return ok;
}
ParserState::ParserState(uint32_t player_count) {
  initialize(player_count);
}
ParserState::ParserState(IReplay *replay) {
  auto header = replay->getHeader();
  if (!header)
    EXCEPTION("Invalid Replay: header is not available");
  initialize(header->player_count);
}
void ParserState::initialize(uint32_t player_count) {
  DG_ASSERT(this->players.size() == 0);
  this->players.reserve(player_count);
  for (size_t i = 0; i < player_count; i++) {
    this->players.emplace_back(this, (mpi::ObjectID)((0xe<<0xb)+i));
  }
  this->g_entity_mgr.curr_event = _new<ecs::EcsRewindEvent>();
}
#if LDAG_DBGLEVEL > 0
void IObjectRewindState::pushBackState(ParserState *state, uint32_t prev, uint32_t curr) {
  state->registerStateChange(this, prev, curr);
}
void ParserState::registerStateChange(IObjectRewindState *state, uint32_t back, uint32_t curr) {
  this->curr_ms_rewind_refs.emplace_back(RewindRef{state, curr, back});
}

#else
void IObjectRewindState::pushBackState(ParserState *state) {
  state->registerStateChange(this);
}
void ParserState::registerStateChange(IObjectRewindState *state) {
  this->curr_ms_rewind_refs.emplace_back(RewindRef{state});
}
#endif

class StateUpdateEvent : public IRewindEvent {
public:
  StateUpdateEvent(ParserState *state, std::pmr::vector<RewindRef> &states) : states(state->get_allocator()) {
    this->states = std::move(states);
  }
  ~StateUpdateEvent() override = default;
  void forward(ParserState &state) override {
    for (auto & st : states) {
#if LDAG_DBGLEVEL > 0
      st.state->rewindForward(st.forward_index);
#else
      st.state->rewindForward();
#endif
    }
  }
  void backward(ParserState &state) override {

    for (auto it = this->states.rbegin(); it != this->states.rend(); ++it) {
#if LDAG_DBGLEVEL > 0
      it->state->rewindBackward(it->rewind_index);
#else
      it->state->rewindBackward();
#endif
    }
  }

private:
  std::pmr::vector<RewindRef> states;

};

void ParserState::beforePacket(ReplayPacket &pkt) {
  if (pkt.timestamp_ms > this->state_update_start_time_ms) {
    if (!this->curr_ms_rewind_refs.empty()) {
      auto evt = this->_new<StateUpdateEvent>(this, this->curr_ms_rewind_refs);
      // just in case
      this->curr_ms_rewind_refs.resize(0);
      this->rewinder.add_action(*this, evt);
    }
    state_update_start_time_ms = pkt.timestamp_ms;
  }
  if (pkt.timestamp_ms > this->g_entity_mgr.last_time_modified) {
    this->rewinder.add_action(*this, this->g_entity_mgr.curr_event);
    this->g_entity_mgr.curr_event = this->_new<ecs::EcsRewindEvent>();
    this->g_entity_mgr.last_time_modified = pkt.timestamp_ms;
  }
}

ecs::EntityId ParserState::getUnitEid(uint16_t uid) const {
  uid &= 0x7FF;
  if (uid == 0x7FF || uid >= this->uid_lookup.size()) {
    return ecs::INVALID_ENTITY_ID;
  }
  return this->uid_lookup[uid];
}

unit::Unit *ParserState::getUnitObj(uint16_t uid) const {
  uid &= 0x7FF;
  if (uid == 0x7FF || uid >= this->uid_unit_lookup.size()) {
    return nullptr;
  }
  return this->uid_unit_lookup[uid];
}

void ParserState::setUnitData(uint16_t uid, unit::Unit *unit, ecs::EntityId eid) {
  uid &= 0x7FF;
  if (uid == 0x7FF) {
    return;
  }
  if (uid >= this->uid_lookup.size()) {
    this->uid_lookup.resize(uid + 1, ecs::INVALID_ENTITY_ID);
    this->uid_unit_lookup.resize(uid + 1, nullptr);
  }
  this->uid_lookup[uid] = eid;
  this->uid_unit_lookup[uid] = unit;
}

ParserState::~ParserState() {
  _in_destruction_state = this;
  _in_destruction_allocator = &this->allocator;
  is_dtor = true;
  ZoneScoped;
  this->rewindToMs(0xFFFFFFFF);
  for (auto v: BattleMessages) {
    _delete(v);
  }
  for (auto v : Zones) {
    _delete(v);
  }
  for (auto v: this->missionAreas1) {
    _delete(v);
  }
  for (auto v: this->missionAreas2) {
    _delete(v);
  }
  _delete(g_entity_mgr.curr_event);
}

bool ParserState::ParsePacket(ReplayPacket &pkt) {
  ZoneScoped;
  beforePacket(pkt);
  curr_time_ms = pkt.timestamp_ms;
  current_packet_index++;
  switch (pkt.type) {
    case ReplayPacketType::EndMarker: {
      replay_length_ms = pkt.timestamp_ms;
      return false;
    }
    case ReplayPacketType::StartMarker: {
      break;
    }
    case ReplayPacketType::AircraftSmall: {
      FMSync(*this, pkt.stream);
      break;
    }
    case ReplayPacketType::Chat: {
      this->chatMessages.resize(this->chatMessages.size() + 1);
      auto &chat_msg = this->chatMessages.back();
      chat_msg.FromBS(pkt.stream);
      chat_msg.time_ms = this->curr_time_ms;
      break;
    }
    case ReplayPacketType::MPI: {
      ZoneScopedN("MPI Dispatch");
      auto m = mpi::dispatch(pkt.stream, this, false);
      if (m != nullptr) {
        mpi::send(m);
        if (m->delete_message)
          this->allocator._delete(m);
      }
      break;
    }
    case ReplayPacketType::NextSegment: {
      return true;
      // LOG("NextSegment");
      break;
    }
    case ReplayPacketType::ECS: {
      conn.onPacket(&pkt, pkt.timestamp_ms);
      break;
    }
    case ReplayPacketType::Snapshot: break;
    case ReplayPacketType::ReplayHeaderInfo: break;
  }
  return true;
}

void ParserState::rewindToMs(uint32_t time_ms) {
  ZoneScoped;
  rewinder.rewind_to_ms(*this, time_ms);
}

StateRewinder::StateRewinder(ParserState *state) : actions_vector(state->getMem()) {
  this->state = state;
  actions_vector.reserve(1000);
  curr_index = 0;
}
StateRewinder::~StateRewinder() {
  for (auto v: actions_vector) {
    state->_delete(v.event);
  }
}
void StateRewinder::rewind_to_ms(ParserState &parser_state, uint32_t time_ms) {
  // if replay isn't fully parsed and we are destroying, then we can assume that we are at latest known point anyways.
  if (parser_state.replay_length_ms == 0xFFFFFFFF) {
    // we can sometimes call this when we haven't fully parsed the replay
    // so lets not make useless log messages
    if (!parser_state.is_dtor)
      LOGE("You cannot rewind until the replay has finshed parsing");
    return;
  }
  const uint32_t sz = actions_vector.size();
    if (sz == 0)
      return;

    if (curr_index > 0 && actions_vector[curr_index - 1].time_ms_at > time_ms) {
      while (curr_index > 0 && actions_vector[curr_index - 1].time_ms_at > time_ms) {
        --curr_index;
        parser_state.curr_time_ms = actions_vector[curr_index].time_ms_at;
        actions_vector[curr_index].event->backward(parser_state);
      }
      parser_state.curr_time_ms = curr_index > 0 ? actions_vector[curr_index - 1].time_ms_at : 0;
    } else {
      while (curr_index < sz && actions_vector[curr_index].time_ms_at <= time_ms) {
        parser_state.curr_time_ms = actions_vector[curr_index].time_ms_at;
        actions_vector[curr_index].event->forward(parser_state);
        ++curr_index;
      }
    }
}
void StateRewinder::add_action(ParserState &parser_state, IRewindEvent *action) {
  actions_vector.emplace_back( action);
  actions_vector.back().time_ms_at = parser_state.curr_time_ms;
  curr_index = actions_vector.size();
}

