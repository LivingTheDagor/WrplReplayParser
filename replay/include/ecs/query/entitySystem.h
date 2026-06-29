//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once
#include "cstdint"
#include "generic/dag_span.h"
#include <ecs/ComponentTypes.h>
#include "ecs/entityId.h"
#include "ecs/query/event.h"
#include "ecsQuery.h"
#include "ecs/ecsHash.h"

namespace ecs {
  class QueryView;
  typedef eastl::fixed_function<sizeof(void *) * 2,
                                void(EntityManager &mgr, const Event &evt, const QueryView &components)>
    EventFuncType;
  // mgr added so an event can do actually complex stuff
  // typedef void (*EventFuncType)(EntityManager *mgr, const Event &evt, const QueryView &components);

  struct EntitySystemOps {
    EventFuncType onEvent; // I most definitely will not need an onUpdate system as I have about zero support for that
                           // at all and I probably wont care about rw to the Event


    EntitySystemOps(EventFuncType evf) : onEvent(evf) {}

    bool empty() const { return !onEvent; }
  };

  struct EntitySystemDesc;

  extern void remove_system_from_list(EntitySystemDesc *desc);

  struct EntitySystemDesc : public NamedQueryDesc {
    typedef void (*DeleteHandler)(EntitySystemDesc *desc);

    EntitySystemDesc(const char *n, const char *module, const EntitySystemOps &ops_,
                     dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro,
                     dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no, EventSet &&evm,
                     const char *tag_set = nullptr, const char *comp_set = nullptr, const char *before_set = nullptr,
                     const char *after_set = nullptr, bool dyn = false) :
      NamedQueryDesc(n, comps_rw, comps_ro, comps_rq, comps_no),
      ops(ops_),
      evSet(eastl::move(evm)),
      dynamic(dyn),
      beforeSet(before_set),
      afterSet(after_set),
      tagSet(tag_set),
      compChangeSet(comp_set),
      moduleName(module) {
      // check on intialization in entityManager
      emptyES = (comps_rw.size() == 0 && comps_ro.size() == 0 && comps_rq.size() == 0 && comps_no.size() == 0);
      next = tail;
      tail = this;
      generation++;
    }


    EntitySystemDesc(const char *n, const EntitySystemOps &ops_, dag::ConstSpan<ComponentDesc> comps_rw,
                     dag::ConstSpan<ComponentDesc> comps_ro, dag::ConstSpan<ComponentDesc> comps_rq,
                     dag::ConstSpan<ComponentDesc> comps_no, EventSet &&evm, const char *tag_set = nullptr,
                     const char *comp_set = nullptr, const char *before_set = nullptr, const char *after_set = nullptr,
                     bool dyn = false) :
      EntitySystemDesc(n, nullptr, ops_, comps_rw, comps_ro, comps_rq, comps_no, eastl::move(evm), tag_set, comp_set,
                       before_set, after_set, dyn) {}

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
    const char *getBefore() const { return beforeSet; }
    const char *getAfter() const { return afterSet; }
    const char *getTags() const { return tagSet; }
    const char *getCompSet() const { return compChangeSet; }
    const char *getModuleName() const { return moduleName; }

    void setEvSet(EventSet &&evs);

    const EventSet &getEvSet() const { return evSet; }

  protected:
    friend class EntityManager;
    friend class GState;

    template<class T>
    friend struct SortDescByPrio;

    EntitySystemOps ops; // operations that this components perform (func-table)
    EventSet evSet; // set of events types that this ES handles
    uint16_t quant = 0;
    bool dynamic = false, emptyES = false; // emptyES will be always called but with empty Query

    EntitySystemDesc *next = NULL; // slist
    static EntitySystemDesc *tail;
    static uint32_t generation;

    const char *beforeSet = nullptr; // CSV entity systems names
    const char *afterSet = nullptr; // CSV entity systems names
    const char *tagSet = nullptr; // CSV entity system tags
    const char *compChangeSet = nullptr; // CSV list of component change event submission
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
  }
}; // namespace ecs
