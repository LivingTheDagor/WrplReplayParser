
// like 95% copied
#pragma once
#include <EASTL/bonus/tuple_vector.h> /// cursed thee stdlib for no tuple_vector

namespace ecs {

  // EventsDB stores all info related to a specific event, including what queries use this event
  // mind you, the EventsDB cannot create an event, it merely stores information about an event
  class EventsDB {
    typedef uint32_t event_id_t;
    static constexpr event_id_t invalid_event_id = std::numeric_limits<event_id_t>::max();
    std::unordered_map<event_type_t, event_id_t> eventsMap; // look up an events index from its hash
    std::unordered_map<event_type_t, destroy_event *> eventsDestroyMap; // contains a references to dtors.
    // I have no idea why this exists and isnt instead just a virtual dtor, maybe the weird class def plays into it?
    std::unordered_map<event_type_t, move_out_event *> eventsMoveMap;
    enum
    {
      EVENT_SIZE,
      EVENT_FLAGS,
      EVENT_TYPE,
      EVENT_NAME
    };
    eastl::tuple_vector<event_size_t, event_flags_t, event_type_t, std::string> eventsInfo;
    // not including event scheme as I think that's only because of das events
    bool registerEvent(event_type_t type, event_size_t sz, event_flags_t flag, const char *name, destroy_event *d, move_out_event *m);
  public:
    EventsDB();
    [[nodiscard]] const char *findEventName(event_type_t type) const;
    [[nodiscard]] event_id_t findEvent(event_type_t type) const;
    [[nodiscard]] event_flags_t getEventFlags(event_id_t) const;
    [[nodiscard]] event_size_t getEventSize(event_id_t) const;
    [[nodiscard]] const char *getEventName(event_id_t) const;
    [[nodiscard]] event_type_t getEventType(event_id_t) const; // for inspection
    [[nodiscard]] uint32_t getEventsCount() const { return (uint32_t)eventsInfo.size(); }
    void dump() const;
    void destroy(Event &) const;
    void moveOut(void *__restrict to, Event &&from) const;

  };

  inline EventsDB::event_id_t EventsDB::findEvent(event_type_t type) const
  {
    auto it = eventsMap.find(type);
    return it == eventsMap.end() ? invalid_event_id : it->second;
  }

  inline event_size_t EventsDB::getEventSize(event_id_t id) const
  {
    G_ASSERT_RETURN(eventsInfo.size() > id, 0);
    return eventsInfo.get<EVENT_SIZE>()[id];
  }

  inline const char *EventsDB::getEventName(event_id_t id) const
  {
    G_ASSERT_RETURN(eventsInfo.size() > id, "");
    return eventsInfo.get<EVENT_NAME>()[id].c_str();
  }

  inline event_type_t EventsDB::getEventType(event_id_t id) const
  {
    G_ASSERT_RETURN(eventsInfo.size() > id, 0);
    return eventsInfo.get<EVENT_TYPE>()[id];
  }


  inline event_flags_t EventsDB::getEventFlags(event_id_t id) const
  {
    G_ASSERT_RETURN(eventsInfo.size() > id, (event_flags_t)0);
    return eventsInfo.get<EVENT_FLAGS>()[id];
  }

  inline const char *EventsDB::findEventName(event_type_t type) const
  {
    event_id_t id = findEvent(type);
    return id == invalid_event_id ? nullptr : getEventName(id);
  }

}
