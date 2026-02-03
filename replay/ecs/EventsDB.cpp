// Copyright (C) Gaijin Games KFT.  All rights reserved.
// like 95% copied

#include "ecs/EntityManager.h"
namespace ecs {

  EventInfoLinkedList *EventInfoLinkedList::tail = nullptr;
  EventInfoLinkedList *EventInfoLinkedList::registered_tail = nullptr;
  static const char *EV_CAST_STR_TYPES[] = {"Unknowncast", "Unicast", "Broadcast", "Bothcast"};


  EventsDB::EventsDB() {
    for(auto ei = EventInfoLinkedList::tail; ei != nullptr; ei = ei->next) {
      registerEvent(ei->getEventType(), ei->getEventSize(), ei->getEventFlags(), ei->getEventName(), ei->getDestroyFunc(),
                    ei->getMoveOutFunc());
    }
  }

  bool EventsDB::registerEvent(event_type_t type, event_size_t sz, event_flags_t flags, const char *name, destroy_event *d,
                               move_out_event *m)
  {
    LOG("Registering Event of name {}", name);
    if (sz >= Event::max_event_size || sz < sizeof(Event))
    {
      EXCEPTION("Can't register Event <0x%X|%s> of size <%d>, size not in [%d,%d)", type, name, sz, sizeof(Event), +Event::max_event_size);
      //return false;
    }
    if ((flags & EVFLG_DESTROY) && (!d || !m))
    {
      EXCEPTION("Can't register Event <0x%X|%s>, which requires Destroy, but doesn't provides destroy/move functions", type, name);
      //if (!d) // if there is destroy function, it is possible to work without move. Event just can't be sent to loading entity.
      //  return false;
    }
    if ((d || m) && !(flags & EVFLG_DESTROY))
      LOGE("Event <0x%X|%s> provides destroy/move functions but is trivially destructible", type, name);
    int evCast = flags & EVFLG_CASTMASK;
    if (!(evCast == EVCAST_UNICAST || evCast == EVCAST_BROADCAST))
      EXCEPTION("Event <0x%X|%s> registered as %s instead of Unicast or Broadcast", type, name, EV_CAST_STR_TYPES[evCast]);

    event_id_t id = findEvent(type);
    const bool ret = (id != invalid_event_id);
    if (id == invalid_event_id)
    {
      id = (event_id_t)eventsInfo.size();
      eventsMap[type] = id;
      eventsInfo.emplace_back(eastl::move(sz), eastl::move(flags), eastl::move(type), std::string(name ? name : "#UnknownEvent#"));
      //event_scheme_hash_t schemeHash = invalid_event_scheme_hash;
      //eventsScheme.emplace_back(eastl::move(schemeHash), eastl::move(event_scheme_t{}));
    }
    else
    {
      G_ASSERT(!name || eventsInfo.get<EVENT_NAME>()[id] == name);
      if (name && eventsInfo.get<EVENT_NAME>()[id] != name)
      {
        EXCEPTION("Event hash collision found <0x%X|%s> collides with %s", type, eventsInfo.get<EVENT_NAME>()[id].c_str(), name);
        //return false;
      }
      // change info. We don't allow to change name, though.
      if (eventsInfo.get<EVENT_SIZE>()[id] != sz || eventsInfo.get<EVENT_FLAGS>()[id] != flags)
      {
        EXCEPTION("Event (0x%X|%s) has changed it size %d -> %d or flags %d -> %d", type,
               name ? name : eventsInfo.get<EVENT_NAME>()[id].c_str(), eventsInfo.get<EVENT_SIZE>()[id], sz,
               eventsInfo.get<EVENT_FLAGS>()[id], flags);
      }
      else
        LOGE("event (%s|0x%X) registered twice", name ? name : eventsInfo.get<EVENT_NAME>()[id].c_str(), type);

      eventsInfo.get<EVENT_SIZE>()[id] = sz;
      eventsInfo.get<EVENT_FLAGS>()[id] = flags;
    }

    if (d)
      eventsDestroyMap[type] = d;
    if (m)
      eventsMoveMap[type] = m;
    return ret;
  }

  void EventsDB::dump() const {
    for (const auto &em: eventsMap) {
      const auto &e = eventsInfo[em.second];
      G_UNUSED(e);
      LOGD("registered {} {}Event <{}|{:#x}> sz {}",
            eastl::get<EventsDB::EVENT_FLAGS>(e) & EVFLG_DESTROY ? "non-trivially-destructible" : "",
            EV_CAST_STR_TYPES[eastl::get<EventsDB::EVENT_FLAGS>(e) & EVFLG_CASTMASK],
            eastl::get<EventsDB::EVENT_NAME>(e).c_str(),
            eastl::get<EventsDB::EVENT_TYPE>(e), eastl::get<EventsDB::EVENT_SIZE>(e));
    }
  }

// out of line
  void EventsDB::destroy(Event &e) const
  {
    G_ASSERT(e.getFlags() & EVFLG_DESTROY);
    auto it = eventsDestroyMap.find(e.getType());
    if (it == eventsDestroyMap.end())
    {
      LOGE("event {:#x}|{} has no registered destroy func", e.getType(), e.getName());
      return;
    }
    it->second(e);
  }


  void EventsDB::moveOut(void *__restrict to, Event &&e) const
  {
    G_ASSERT(e.getFlags() & EVFLG_DESTROY);
    auto it = eventsMoveMap.find(e.getType());
    G_ASSERTF_AND_DO(it != eventsMoveMap.end(), return , "{:#x}|{}", e.getType(), e.getName());
    it->second(to, eastl::move(e));
  }

  const char *Event::getName() const
  {
      auto find_type = [this](const EventInfoLinkedList *l) { return l->getEventType() == type; };
      auto it = EventInfoLinkedList::find_if(EventInfoLinkedList::get_tail(), find_type);
      if (it != nullptr)
        return it->getEventName();
      return "#Unknown#";
  }

  ECS_REGISTER_EVENT(EventEntityCreated)
  ECS_REGISTER_EVENT(EventEntityDestroyed)
}
