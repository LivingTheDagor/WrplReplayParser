#include "state/ParserState.h"
#include "ecs/entityId.h"

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

ecs::EntityId ParserState::getUnitEid(uint16_t uid) {
  uid &= 0x7FF;
  if (uid == 0x7FF || uid >= this->uid_lookup.size()) {
    return ecs::INVALID_ENTITY_ID;
  }
  return this->uid_lookup[uid];
}

unit::Unit *ParserState::getUnitObj(uint16_t uid) {
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
  ZoneScoped;
  this->rewindToMs(0xFFFFFFFF);
  for (auto v: Zones) {
    delete v;
  }
  for (auto v: BattleMessages) {
    delete v;
  }
  for (auto v: missionAreas1) {
    delete v;
  }
}

bool ParserState::ParsePacket(ReplayPacket &pkt) {
  curr_time_ms = pkt.timestamp_ms;
  current_rewind_ms = curr_time_ms;
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
      this->chatMessages[this->chatMessages.size() - 1].FromBS(pkt.stream);
      this->chatMessages[this->chatMessages.size() - 1].time_ms = this->curr_time_ms;
      break;
    }
    case ReplayPacketType::MPI: {
      auto m = mpi::dispatch(pkt.stream, this, false);
      if (m != nullptr) {
        mpi::send(m);
        if (m->delete_message)
          delete m;
      }
      break;
    }
    case ReplayPacketType::NextSegment: {
      return true;
      // LOG("NextSegment");
      break;
    }
    case ReplayPacketType::ECS: {
      onPacket(&pkt);
      break;
    }
    case ReplayPacketType::Snapshot: break;
    case ReplayPacketType::ReplayHeaderInfo: break;
  }
  return true;
}

void ParserState::rewindToMs(uint32_t time_ms) {
  if (replay_length_ms == 0xFFFFFFFF) {
    LOGE("You cannot rewind until the replay has finshed parsing");
    return;
  }
  if (current_rewind_ms == time_ms)
    return;
  // LOGI("rewinding to {} from {}", time_ms, current_rewind_ms);
  current_rewind_ms = time_ms;
  this->g_entity_mgr.rewindTo(time_ms);
  for (auto &p: players)
    p.rewindToTime(time_ms);
  for (auto &p: teams)
    p.rewindToTime(time_ms);
  for (auto &p: Zones)
    p->rewindToTime(time_ms);
  for (auto &p: missionAreas2)
    if (p)
      p->rewindToTime(time_ms);
  curr_time_ms = time_ms;
}