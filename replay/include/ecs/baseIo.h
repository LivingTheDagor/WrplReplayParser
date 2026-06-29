#ifndef MYEXTENSION_BASEIO_H
#define MYEXTENSION_BASEIO_H

namespace ecs {
  // all these functions are various component serialization funcs.
  // network deserialization uses deserialize_init_component_typeless for Construction
  void serialize_entity_component_ref_typeless(const void *comp_data, component_index_t cidx,
                                               component_type_t type_name, type_index_t type_id,
                                               SerializerCb &serializer, ecs::EntityManager *mgr);
  void serialize_entity_component_ref_typeless(const ComponentRef &comp, SerializerCb &serializer,
                                               ecs::EntityManager *mgr);

  void serialize_entity_component_ref_typeless(const void *comp_data, component_type_t type_name,
                                               SerializerCb &serializer, ecs::EntityManager *mgr);

  void serialize_entity_component_ref_typeless(const void *comp_data, // if component type is boxed, then this is void**
                                                                      // (pointer to pointer to actual data)
                                               component_type_t type_name, SerializerCb &serializer,
                                               ecs::EntityManager *mgr);
  MaybeComponent deserialize_init_component_typeless(ecs::component_type_t type_name, ecs::component_index_t cidx,
                                                     const DeserializerCb &serializer, ecs::EntityManager *mgr);
  void serialize_child_component(const Component &comp, SerializerCb &serializer,
                                 ecs::EntityManager *mgr); // write type, than
  // serialize_entity_component_ref_typeless
  MaybeComponent
  deserialize_child_component(const DeserializerCb &serializer,
                              ecs::EntityManager *mgr); // Empty optional<> means non recoverable error (bad
  bool deserialize_component_typeless(ComponentRef &comp, const DeserializerCb &deserializer, ecs::EntityManager &mgr);

} // namespace ecs
#endif // MYEXTENSION_BASEIO_H
