#pragma once
#include "cstdint"
namespace ecs {

  class SerializerCb;
  class DeserializerCb;

  void write_string(ecs::SerializerCb &cb, const char *pStr, uint32_t max_string_len);
  int read_string(const ecs::DeserializerCb &cb, char *buf, uint32_t buf_size);

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


  class SerializerCb {
  public:
    virtual void write(const void *, size_t sz_in_bits, component_type_t hint) = 0;
    virtual ~SerializerCb() = default;
  };

  class DeserializerCb {
  public:
    virtual bool read(void *, size_t sz_in_bits, component_type_t hint) const = 0;
    virtual ~DeserializerCb() = default;
  };

  inline void write_compressed(SerializerCb &cb, uint32_t v) {
    uint8_t data[sizeof(v) + 1];
    int i = 0;
    for (; i < (int) sizeof(data); ++i) {
      data[i] = uint8_t(v) | (v >= (1 << 7) ? (1 << 7) : 0);
      v >>= 7;
      if (!v) {
        ++i;
        break;
      }
    }
    cb.write(data, (size_t) i * CHAR_BIT, 0);
  }

  inline bool read_compressed(const DeserializerCb &cb, uint32_t &v) {
    v = 0;
    for (int i = 0; i < (int) (sizeof(v) + 1); ++i) {
      uint8_t byte = 0;
      if (!cb.read(&byte, CHAR_BIT, 0))
        return false;
      v |= uint32_t(byte & ~(1 << 7)) << (i * 7);
      if ((byte & (1 << 7)) == 0)
        break;
    }
    return true;
  }

  // default serializer is just call as-is.
  // if we want we can register different serializer, with quantization and stuff

  class ComponentSerializer {
  public:
    virtual void serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint,
                           ecs::EntityManager *mgr) {
      cb.write(data, sz * CHAR_BIT, hint);
    }

    virtual bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                             ecs::EntityManager *mgr) {
      return cb.read(data, sz * CHAR_BIT, hint);
    }

    virtual ~ComponentSerializer() = default;
  };

} // namespace ecs
