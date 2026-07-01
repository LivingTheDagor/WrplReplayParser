#include "state/ParserState.h"
#include "ecs/ComponentTypesDefs.h"
#include "Unit.h"
#include "ecs/query/coreEvents.h"
#include "ecs/ecsCodegen.h"


static void on_tank_appear_es(const ecs::EventEntityCreated &evt,
                              const HeavyVehicleModelStorageComponent &unit_storage__tank, const int &uid,
                              unit::UnitRef &unit__ref, ecs::EntityManager &manager) {
  G_ASSERT(unit__ref.unit == nullptr);
  unit__ref.unit = new unit::Tank(static_cast<uint16_t>(uid));
  unit__ref.unit->LoadFromStorage(unit_storage__tank);
}

static void on_aircraft_appear_es(const ecs::EventEntityCreated &evt,
                                  const FlightModelWrapStorageComponent &unit_storage__aircraft, const int &uid,
                                  unit::UnitRef &unit__ref, ecs::EntityManager &manager) {
  G_ASSERT(unit__ref.unit == nullptr);
  unit__ref.unit = new unit::Aircraft(static_cast<uint16_t>(uid));
  unit__ref.unit->LoadFromStorage(unit_storage__aircraft);
}

static void on_unit_disappear_es(const ecs::EventEntityDestroyed &evt, unit::UnitRef &unit__ref) {
  if (unit__ref.unit)
    delete unit__ref.unit;
}

ECS_AFTER(on_aircraft_appear_es, on_tank_appear_es)
static void after_unit_appear_es(const ecs::EventEntityCreated &evt, unit::UnitRef &unit__ref, const int &uid,
                                 ecs::EntityManager &manager) {
  if (unit__ref.unit) {
    unit__ref.unit->created_at_ms = *manager.curr_time_ms;
  }
}


ECS_AFTER(after_unit_appear_es)
static void uid_entity_es(const ecs::EventEntityCreatedBasic &evt, const ecs::EntityId eid, const int &uid,
                          unit::UnitRef &unit__ref, ecs::EntityManager &manager) {
  if (uid == -1) {
    LOGE("Entity {:#x} has no uid, normal?", eid.get_handle());
    return;
  }
  ENTITY_LOGD3("Entity {:#x} requested uid {}", eid.get_handle(), uid);
  auto state = manager.owned_by;
  uint16_t unit_id = uid & 0x7FF;
  G_ASSERT(state->getUnitEid(unit_id) == ecs::INVALID_ENTITY_ID);
  state->setUnitData(unit_id, unit__ref.unit, eid);
  if (unit__ref.unit) {
    unit__ref.unit->curr_eid = eid;
  }
}

ECS_AFTER(after_unit_appear_es)
static void uid_entity_es(const ecs::EventEntityDestroyedBasic &evt, unit::UnitRef &unit__ref, const int &uid,
                          ecs::EntityManager &manager) {
  auto state = manager.owned_by;
  uint16_t unit_id = uid & 0x7FF;
  G_ASSERT(state->getUnitEid(unit_id) != ecs::INVALID_ENTITY_ID);
  state->setUnitData(unit_id, nullptr, ecs::INVALID_ENTITY_ID);
  if (unit__ref.unit) {
    unit__ref.unit->curr_eid = evt.get<0>();
    if (evt.get<1>()) {
      unit__ref.unit->destroyed_at_ms = *manager.curr_time_ms;
    }
  }
}

template<typename Callable>
ECS_REQUIRE(ecs::Tag playerUnit)
static void iterate_all_units_ecs_query(ecs::EntityManager &manager, Callable c);

void iterate_all_units(ParserState &state) {
  iterate_all_units_ecs_query(state.g_entity_mgr, [](const unit::UnitRef &unit__ref, const ecs::string &unit__className,
                                                     const ecs::EntityId eid) {
    if (unit__ref.unit) {
      auto &unit = *unit__ref.unit;
      // log_ext("", 0, DEFAULT_SINK_HANDLER, LOGLEVEL::INFO, fmt::format("{}, {}", unit__ref.unit->uid,
      // unit__className));
      uint32_t time_ms = unit.positions.empty() ? 0 : unit.positions[unit.positions.size() - 1].time_ms;
      LOGI("{} {} owned by {} (eid {:#x}) last path at {}({})", unit.uid, unit__className, unit.owner_pid,
           eid.get_handle(), ((float) time_ms) / 1000.f, unit.positions.size());
    } else {
      LOGI("unk unit {}", unit__className);
    }
    return ecs::QueryCbResult::Continue;
  });
}

std::vector<unit::Unit *> collect_all_units(ParserState &state) {
  std::vector<unit::Unit *> units{};
  iterate_all_units_ecs_query(
    state.g_entity_mgr,
    [&units](const unit::UnitRef &unit__ref, const ecs::string &unit__className, const ecs::EntityId eid) {
      if (unit__ref.unit && !unit__ref.unit->positions.empty()) {
        units.push_back(unit__ref.unit);
      }
      return ecs::QueryCbResult::Continue;
    });
  return units;
}


ECS_AFTER(after_unit_appear_es)
static void on_unit_appear_mpi_es(const ecs::EventEntityCreated &evt, const ecs::EntityId eid,
                                  const unit::UnitRef &unit__ref, ecs::EntityManager &manager) {
  ParserState &state = *manager.owned_by;
  // LOGI("attempting to complete queued dispatch for eid: {:#x}", eid.get_handle());
  auto data = state.get_queued_data(eid);
  // let's only dispatch when we have both the unit and data
  if (unit__ref.unit && data) {
    for (auto &queue_obj: *data) {
      auto &bs = queue_obj.bs;
      auto obj = (danet::ReflectableObject *) mpi::MpiQueueObject::UnitRef_Dispatch(queue_obj.oid, queue_obj.extUid,
                                                                                    &state, false);
      if (!obj)
        continue;
      uint32_t sz = 0;
      bs.Read(sz);
      if (sz & mpi::QueuePacketTypes::MPI) {
        sz ^= mpi::QueuePacketTypes::MPI;
        auto m = mpi::dispatch(bs, &state, false);
        if (m != nullptr) {
          mpi::send(m);
          if (m->delete_message)
            delete m;
        }
      } else if (sz & mpi::QueuePacketTypes::REFL) {
        sz ^= mpi::QueuePacketTypes::REFL;
        obj->deserialize(bs, sz, &state);
      }
    }
  }

  if (data) {
    // erases all data, including BitStream data
    data->clear();
    data->shrink_to_fit();
  }
}
