

#ifndef WTFILEUTILS_QUERYVIEW_H
#define WTFILEUTILS_QUERYVIEW_H

// a QueryView holds a reference to the RW and RO components a specific query requests
// it can reference all the needed components of entities in a single chunk
class QueryView {

  void ** SOARefs; // references the arrays based on the component layout found in the ecs::ComponentDesc[] this event uses
  ecs::EntityId *eid_refs; // we use eid to determine if an entity exists in a slot or not.
  // im assuming dagor engine actually defragments an archetype before running a query
  uint32_t index_start; // where in the chunk entities start
  uint32_t index_end; // where in the chunk entities end
};
#endif //WTFILEUTILS_QUERYVIEW_H
