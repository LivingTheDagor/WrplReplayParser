//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "ecs/entityId.h"
#include "ecs/query/event.h"
#include "ecsQuery.h"
#include "ecs/ecsHash.h"

namespace ecs {
  class QueryView;

  typedef void (*EventFuncType)(const Event &evt, const QueryView &components);

  struct EntitySystemOps {
    EventFuncType onEvent; // I most definitely will not need an onUpdate system as I have about zero support for that at all and I probably wont care about rw to the Event


    EntitySystemOps(EventFuncType evf) : onEvent(evf) {}

    bool empty() const { return !onEvent; }
  };

  struct EntitySystemDesc;

  extern void remove_system_from_list(EntitySystemDesc *desc);

  struct EntitySystemDesc : public NamedQueryDesc {
    typedef void (*DeleteHandler)(EntitySystemDesc *desc);

    EntitySystemDesc(const char *n, const char *module, EntitySystemOps ops_, dag::ConstSpan<ComponentDesc> comps_rw,
                     dag::ConstSpan<ComponentDesc> comps_ro, dag::ConstSpan<ComponentDesc> comps_rq,
                     dag::ConstSpan<ComponentDesc> comps_no,
                     EventSet &&evm, int stm, const char *tag_set = nullptr,
                     bool dyn = false,
                     DeleteHandler on_delete = nullptr) :
        NamedQueryDesc(n, comps_rw, comps_ro, comps_rq, comps_no),
        ops(ops_),
        evSet(eastl::move(evm)),
        dynamic(dyn),
        onDelete(on_delete),
        tagSet(tag_set),
        moduleName(module) {
      // check on intialization in entityManager
      emptyES = (comps_rw.size() == 0 && comps_ro.size() == 0 && comps_rq.size() == 0 && comps_no.size() == 0);
      next = tail;
      tail = this;
      generation++;
    }

    EntitySystemDesc(const char *n, EntitySystemOps ops_, dag::ConstSpan<ComponentDesc> comps_rw,
                     dag::ConstSpan<ComponentDesc> comps_ro,
                     dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no, EventSet &&evm,
                     int stm,
                     const char *tag_set = nullptr, bool dyn = false,
                     DeleteHandler on_delete = nullptr) :
        EntitySystemDesc(n, nullptr, ops_, comps_rw, comps_ro, comps_rq, comps_no, eastl::move(evm), stm, tag_set,
                         dyn, on_delete) {}

    ~EntitySystemDesc();

    EntitySystemDesc &operator=(EntitySystemDesc &&) = default;

    void freeIfDynamic() {
      if (dynamic)
        delete this;
    }

    uint32_t getQuant() const { return quant; }

    bool isDynamic() const { return dynamic; }

    bool isEmpty() const { return emptyES; }

    static EntitySystemDesc *getTail() { return tail; }

    const EntitySystemOps getOps() const { return ops; }

    const char *getTags() const { return tagSet; }

    const char *getModuleName() const { return moduleName; }

    void setEvSet(EventSet &&evs);

    const EventSet &getEvSet() const { return evSet; }

  protected:
    friend class EntityManager;

    template<class T>
    friend
    struct SortDescByPrio;

    EntitySystemOps ops;      // operations that this components perform (func-table)
    EventSet evSet;           // set of events types that this ES handles
    uint16_t quant = 0;
    bool dynamic = false, emptyES = false; // emptyES will be always called but with empty Query

    EntitySystemDesc *next = NULL; // slist
    static EntitySystemDesc *tail;
    static uint32_t generation;

    DeleteHandler onDelete = nullptr;
    const char *tagSet = nullptr;        // CSV entity system tags
    const char *moduleName = nullptr;

    template<typename Lambda>
    friend void iterate_systems(Lambda fn);

    template<typename Lambda>
    friend EntitySystemDesc *find_if_systems(Lambda fn);
  };

  inline void EntitySystemDesc::setEvSet(EventSet &&evs) {
    evSet = eastl::move(evs);
    ++generation;
  }

  template<typename Lambda>
  inline void iterate_systems(Lambda fn) {
    for (EntitySystemDesc *system = EntitySystemDesc::tail; system;) {
      EntitySystemDesc *nextSys = system->next;
      fn(system);
      system = nextSys;
    }
  }

  template<typename Lambda>
  // Lambda return true
  inline EntitySystemDesc *find_if_systems(Lambda fn) {
    for (EntitySystemDesc *system = EntitySystemDesc::tail; system;) {
      EntitySystemDesc *nextSys = system->next;
      if (fn(system))
        return system;
      system = nextSys;
    }
    return nullptr;
  }

  inline EntitySystemDesc::~EntitySystemDesc() {
    // removes from list and calls dtor
    for (EntitySystemDesc *system = EntitySystemDesc::tail, *prevSys = nullptr; system;) {
      EntitySystemDesc *nextSys = system->next;
      if (system == this) {
        ++EntitySystemDesc::generation;
        if (prevSys)
          prevSys->next = nextSys;
        else
          EntitySystemDesc::tail = nextSys;
        break;
      } else
        prevSys = system;
      system = nextSys;
    }
    if (onDelete)
      onDelete(this);
  }

}; // namespace ecs
