
#include "ecs/ComponentTypesDefs.h"
#include "ecs/BitStreamDeserializer.h"
#include "idFieldSerializer.h"
#include "ecs/EntityManager.h"
#include "network/eid.h"
#include <math/dag_mathUtils.h>
#include "gameMath/quantization.h"

void hello() { std::cout << ""; }

namespace ecs {
  //extern int MAX_STRING_LENGTH;
}
class PartIdSerializer final : public ecs::ComponentSerializer {
  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<dm::PartId>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    cb.write(data, 6, 0);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<dm::PartId>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    return cb.read(data, 6, 0);
  }
};

static PartIdSerializer part_id_serializer;
ECS_REGISTER_POD_TYPE(ecs::Tag, nullptr);
ECS_REGISTER_POD_TYPE(dm::PartId, &part_id_serializer);
ECS_REGISTER_POD_TYPE(ecs::EntityId, nullptr);
ECS_REGISTER_POD_TYPE(bool, nullptr);
ECS_REGISTER_POD_TYPE(uint8_t, nullptr);
ECS_REGISTER_POD_TYPE(uint16_t, nullptr);
ECS_REGISTER_POD_TYPE(uint32_t, nullptr);
ECS_REGISTER_POD_TYPE(uint64_t, nullptr);
ECS_REGISTER_POD_TYPE(int8_t, nullptr);
ECS_REGISTER_POD_TYPE(int16_t, nullptr);
ECS_REGISTER_POD_TYPE(int, nullptr);
ECS_REGISTER_POD_TYPE(int64_t, nullptr);
ECS_REGISTER_POD_TYPE(float, nullptr);
ECS_REGISTER_POD_TYPE(E3DCOLOR, nullptr);
ECS_REGISTER_POD_TYPE(Point2, nullptr);
ECS_REGISTER_POD_TYPE(Point3, nullptr);
ECS_REGISTER_POD_TYPE(Point4, nullptr);
ECS_REGISTER_POD_TYPE(IPoint2, nullptr);
ECS_REGISTER_POD_TYPE(IPoint3, nullptr);
ECS_REGISTER_POD_TYPE(IPoint4, nullptr);
ECS_REGISTER_POD_TYPE(DPoint3, nullptr);
ECS_REGISTER_POD_TYPE(TMatrix, nullptr);
ECS_REGISTER_POD_TYPE(vec4f, nullptr);
ECS_REGISTER_POD_TYPE(bbox3f, nullptr);
ECS_REGISTER_POD_TYPE(mat44f, nullptr);
ECS_REGISTER_POD_TYPE(BBox3, nullptr);


class StringSerializer final : public ecs::ComponentSerializer {
public:
  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<ecs::string>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    ecs::write_string(cb, ((const ecs::string *) data)->c_str(), ecs::MAX_STRING_LENGTH);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<ecs::string>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    char buf[ecs::MAX_STRING_LENGTH];
    int len = ecs::read_string(cb, buf, sizeof(buf));
    if (len < 0)
      return false;
    *((ecs::string *) data) = ecs::string(buf, len);
    return true;
  }
};

static StringSerializer string_serializer;
ECS_REGISTER_CTM_TYPE(ecs::string, &string_serializer);

class ObjectSerializer final : public ecs::ComponentSerializer {
public:
  typedef ecs::Object SerializedType;

  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    cb.write(data, sz, hint);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    return cb.read(data, sz,
                   hint); // ACTUUUUUAL DESERIALIZATION HAPPENS INSIDE OF BitStreamDeserializer, dont let this fool you
  }
};

static ObjectSerializer object_serializer;
ECS_REGISTER_CTM_TYPE(ecs::Object, &object_serializer);

class ArraySerializer final : public ecs::ComponentSerializer {
public:
  typedef ecs::Array SerializedType;

  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    const SerializedType &arr = *((const SerializedType *) data);
    ecs::write_compressed(cb, arr.size());
    for (auto &it: arr)
      ecs::serialize_child_component(it, cb, mgr);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    SerializedType &arr = *((SerializedType *) data);
    arr.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(cb, cnt))
      return false;
    arr.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
      if (ecs::MaybeComponent mbcomp = ecs::deserialize_child_component(cb, mgr))
        arr.push_back(std::move(*mbcomp));
      else
        return false;
    }
    return true;
  }
};

static ArraySerializer array_serializer;
ECS_REGISTER_CTM_TYPE(ecs::Array, &array_serializer);

template<typename SerializedType>
class ListSerializer final : public ecs::ComponentSerializer {
public:
  using ItemType = typename SerializedType::value_type;

  static constexpr size_t itemSizeInBits = CHAR_BIT * ecs::ComponentTypeInfo<ItemType>::size;
  static constexpr ecs::component_type_t itemComponentType = ecs::ComponentTypeInfo<ItemType>::type;

  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    const SerializedType &list = *((const SerializedType *) data);
    ecs::write_compressed(cb, (uint32_t)list.size());
    for (const ItemType &item: list)
      cb.write(&item, itemSizeInBits, itemComponentType);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<SerializedType>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    SerializedType &list = *((SerializedType *) data);
    list.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(cb, cnt))
      return false;
    list.resize(cnt);
    for (size_t i = 0; i < list.size(); ++i) {
      ItemType tmp;
      if (!cb.read(&tmp, itemSizeInBits, itemComponentType))
        return false;
      list[i] = tmp;
    }
    return true;
  }
};


class PartIdListSerializer final : public ecs::ComponentSerializer {
public:
  using ItemType = typename ecs::List<dm::PartId>::value_type;

  static constexpr size_t itemSizeInBits = CHAR_BIT * ecs::ComponentTypeInfo<ItemType>::size;
  static constexpr ecs::component_type_t itemComponentType = ecs::ComponentTypeInfo<ItemType>::type;

  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<ecs::List<dm::PartId>>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    const ecs::List<dm::PartId> &list = *((const ecs::List<dm::PartId> *) data);
    ecs::write_compressed(cb, (uint32_t)list.size());
    cb.write(list.data(), (list.size() << 3) * 6, 0);
    //for (const ItemType &item: list)
    //  cb.write(&item, 6, itemComponentType);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<ecs::List<dm::PartId>>::type);
    G_UNUSED(hint);
    G_UNUSED(sz);
    ecs::List<dm::PartId> &list = *((ecs::List<dm::PartId> *) data);
    list.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(cb, cnt))
      return false;
    list.resize(cnt << 3);
    //return cb.read(list.data(), (cnt<<3)*6, 0);
    for (size_t i = 0; i < list.size(); ++i) {
      ItemType tmp;
      if (!cb.read(&tmp, 6, 0))
        return false;
      list[i] = tmp;
    }
    return true;
  }
};


static struct PropsIdListSerializer final : public ecs::ComponentSerializer {
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<props::PropsIdList>::type);
    G_UNUSED(hint);
    props::PropsIdList &propList = *(props::PropsIdList*) data;
    write_compressed(cb,(uint32_t)propList.size());
    for(auto & prop : propList) {
      write_compressed(cb, prop.data);
    }
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<props::PropsIdList>::type);
    G_UNUSED(hint);
    props::PropsIdList &propList = *(props::PropsIdList*) data;
    uint32_t t;
    bool isOk = read_compressed(cb, t);
    propList.resize(t);
    for(auto & prop : propList) {
      isOk &= read_compressed(cb, prop.data);
    }
    return isOk;
  }
} PropsIdList_list_serializer;

#define DECL_LIST_TYPE(nm, lt, t)                          \
  static ListSerializer<nm::lt> lt##_list_serializer; \
  ECS_REGISTER_CTM_TYPE(nm::lt, &lt##_list_serializer)

ECS_REGISTER_CTM_TYPE(props::PropsIdList, &PropsIdList_list_serializer);
static PartIdListSerializer PartIdList_list_serializer;
ECS_REGISTER_CTM_TYPE(dm::PartIdList, &PartIdList_list_serializer);



static ListSerializer<ecs::UInt8List> UInt8List_list_serializer; ECS_REGISTER_CTM_TYPE(ecs::UInt8List, &UInt8List_list_serializer);
DECL_LIST_TYPE(ecs, UInt16List, uint16_t);
DECL_LIST_TYPE(ecs, UInt32List, uint32_t);
DECL_LIST_TYPE(ecs, UInt64List, uint64_t);
DECL_LIST_TYPE(ecs, StringList, ecs::string);
DECL_LIST_TYPE(ecs, EidList, ecs::EntityId);
DECL_LIST_TYPE(ecs, FloatList, float);
DECL_LIST_TYPE(ecs, Point2List, Point2);
DECL_LIST_TYPE(ecs, Point3List, Point3);
DECL_LIST_TYPE(ecs, Point4List, Point4);
DECL_LIST_TYPE(ecs, IPoint2List, IPoint2);
DECL_LIST_TYPE(ecs, IPoint3List, IPoint3);
DECL_LIST_TYPE(ecs, IPoint4List, IPoint4);
DECL_LIST_TYPE(ecs, BoolList, bool);
DECL_LIST_TYPE(ecs, TMatrixList, TMatrix);
DECL_LIST_TYPE(ecs, ColorList, E3DCOLOR);
DECL_LIST_TYPE(ecs, Int8List, int8_t);
DECL_LIST_TYPE(ecs, Int16List, int16_t);
DECL_LIST_TYPE(ecs, IntList, int);
DECL_LIST_TYPE(ecs, Int64List, int64_t);
#undef DECL_LIST_TYPE


#define LOG_NON_ORTHO 0

// instead of 48 bytes we typically write 20/24 (12 position + 8 quat + (may be) 4 scale) for (scaled) ortho-normal matrices
// which is twice less
static struct TransformSerializer final : public ecs::ComponentSerializer {
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TMatrix>::type);
    G_UNUSED(hint);
    const TMatrix &transform = *(const TMatrix *) data;
    float len0 = lengthSq(transform.getcol(0));
    const bool isOrthoUni =
        (fabsf(lengthSq(transform.getcol(1)) - len0) < 1e-3f && fabsf(lengthSq(transform.getcol(2)) - len0) < 1e-3f &&
         fabsf(dot(transform.getcol(0), transform.getcol(1))) < 1e-5f &&
         fabsf(dot(transform.getcol(0), transform.getcol(2))) < 1e-5f &&
         fabsf(dot(transform.getcol(1), transform.getcol(2))) < 1e-5f &&
         dot(transform.getcol(0) % transform.getcol(1), transform.getcol(2)) > 0.f);
    cb.write(&isOrthoUni, 1, 0);
    if (isOrthoUni) {
      len0 = sqrtf(len0);
      const bool hasScale = fabsf(len0 - 1.0f) > 1e-5f;
      gamemath::QuantizedQuat62 quanitizedQuat(
          normalize(matrix_to_quat(hasScale ? transform * (1.f / len0) : transform)));
      cb.write(&quanitizedQuat.qquat, 62, 0);
      cb.write(&hasScale, 1, 0);
      if (hasScale)
        cb.write(&len0, sizeof(float) * 8, 0);
    } else {
      cb.write(transform[0], sizeof(float) * 9 * CHAR_BIT, 0);
    }
    cb.write(transform[3], sizeof(float) * 3 * CHAR_BIT, 0); // do not quanitized position
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<TMatrix>::type);
    G_UNUSED(hint);
    TMatrix &transform = *(TMatrix *) data;
    bool isOrthoUni = false;
    bool isOk = cb.read(&isOrthoUni, 1, 0);
    if (isOrthoUni) {
      gamemath::QuantizedQuat62 quanitizedQuat;
      isOk &= cb.read(&quanitizedQuat.qquat, 62, 0);
      transform = quat_to_matrix(quanitizedQuat.unpackQuat());
      bool hasScale = false;
      isOk &= cb.read(&hasScale, 1, 0);
      if (hasScale) {
        float scale;
        isOk &= cb.read(&scale, sizeof(float) * 8, 0);
        transform *= scale;
      }
    } else
      isOk &= cb.read(transform[0], sizeof(float) * 9 * 8, 0);

    isOk &= cb.read(transform[3], sizeof(float) * 3 * 8, 0); // do not quanitized transform
    return isOk;
  }
} transform_serializer;

static struct PropsIdSerializer final : public ecs::ComponentSerializer {
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<props::PropsId>::type);
    G_UNUSED(hint);
    props::PropsId &prop = *(props::PropsId*) data;
    write_compressed(cb,prop.data);
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    G_ASSERT(hint == ecs::ComponentTypeInfo<props::PropsId>::type);
    G_UNUSED(hint);
    props::PropsId &prop = *(props::PropsId*) data;
    bool isOk = read_compressed(cb, prop.data);
    return isOk;
  }
} prop_serializer;

static struct RendInstSerializer final : public ecs::ComponentSerializer
{
  static constexpr uint32_t ri_type_bits = 12;
  static constexpr uint32_t ri_type_full_bits = 16;
  static constexpr uint32_t ri_inst_base_bits = 11,
      ri_inst_other_bits = 12, // enough for storing 8M instances
  ri_inst_total_bits = ri_inst_base_bits + ri_inst_other_bits;
  static constexpr uint32_t short_bits = 1 + ri_inst_base_bits + ri_type_bits;
  static constexpr uint32_t long_bits = 1 + ri_inst_total_bits + ri_type_full_bits;
  void serialize(ecs::SerializerCb &cb, const void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<rendinst::riex_handle_t>::type);
    G_STATIC_ASSERT(ri_inst_total_bits <= rendinst::ri_instance_type_shift);
    G_STATIC_ASSERT(ri_type_bits <= ri_type_full_bits);
    G_STATIC_ASSERT(ri_type_full_bits + rendinst::ri_instance_type_shift <= 64);
    G_UNUSED(hint);

    rendinst::riex_handle_t handle = (*(const rendinst::riex_handle_t *)data) + 1; // so invalid became zero.
    uint32_t riType = rendinst::handle_to_ri_type(handle), riInst = rendinst::handle_to_ri_inst(handle);
    G_ASSERT(riInst < (1 << ri_inst_total_bits)); // we save 9 bits.
    if (riInst < (1 << ri_inst_base_bits) && riType < (1 << ri_type_bits))
    {
      // 24 bits (ri_inst_base_bits : ri_type_bits : 0)
      G_STATIC_ASSERT(short_bits % 8 == 0);
      uint32_t word24 = riInst | (riType << ri_inst_base_bits);
      cb.write(&word24, short_bits, 0);
    }
    else
    {
      // 40 bits (ri_inst_total_bits : 1 : ri_type_full_bits)
      G_STATIC_ASSERT(long_bits % 8 == 0);
      uint64_t word40 = riInst | (1 << ri_inst_total_bits) | (uint64_t(riType) << (ri_inst_total_bits + 1));
      cb.write(&word40, long_bits, 0);
    }
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t, ecs::component_type_t hint, ecs::EntityManager *mgr) override
  {
    G_ASSERT(hint == ecs::ComponentTypeInfo<rendinst::riex_handle_t>::type);
    G_UNUSED(hint);
    rendinst::riex_handle_t &handle = *((rendinst::riex_handle_t *)data);
    uint32_t riType = 0, riInst = 0;
    uint32_t word24 = 0;
    bool isOk = cb.read(&word24, short_bits, 0);
    if (DAGOR_UNLIKELY(word24 & (1 << ri_inst_total_bits)))
    {
      G_STATIC_ASSERT(short_bits + ri_type_full_bits == long_bits);
      riInst = word24 & ((1 << ri_inst_total_bits) - 1);
      isOk &= cb.read(&riType, ri_type_full_bits, 0);
    }
    else
    {
      riType = word24 >> ri_inst_base_bits;
      riInst = word24 & ((1 << ri_inst_base_bits) - 1);
    }
    if (!riInst && !riType)
      handle = rendinst::RIEX_HANDLE_NULL;
    else
      handle = rendinst::make_handle(riType, riInst) - 1;
    return isOk;
  }
} rendinst_serializer;


namespace ecs {
  void FieldSerializerDictIO::serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
    EXCEPTION("Called serialize of FieldSerializerDictIO, not implemented");
  }

  bool FieldSerializerDictIO::deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
    auto *actual = (FieldSerializerDict *) data;
    uint32_t blk_sz;
    if (!read_compressed(cb, blk_sz))
      return false;

    auto *m_data = malloc(blk_sz);
    cb.read(m_data, blk_sz, 0);
    BitStream bs{(unsigned char *) m_data, blk_sz, false};
    IdFieldSerializer255 IdFieldSerilizer;
    uint32_t end;
    uint32_t count = IdFieldSerilizer.readFieldsSizeAndCount(bs, end);
    if (!IdFieldSerilizer.readFieldsIndex(bs)) {
      free(m_data);
      return false;
    }

    for (uint16_t i = 0; i < count; ++i) {
      auto fieldId = IdFieldSerilizer.getFieldId(i);
      auto f_size = IdFieldSerilizer.getFieldSize(i);
      (actual->data)[fieldId] = std::vector<unsigned char>(BITS_TO_BYTES(f_size));

      bs.ReadBits((actual->data)[fieldId].data(), f_size);
    }
    free(m_data);
    return true;
  }

  void RocketSerializer::serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {

    EXCEPTION("Called serialize of RocketSerializer, not implemented");
  }

  bool RocketSerializer::deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
    auto &Rocket_data = *(Rocket *) data;
    read_compressed(cb, Rocket_data.uleb_1);
    cb.read(&Rocket_data.eid, 4 * 8, ecs::ComponentTypeInfo<ecs::EntityId>::type); // 0x57fcec2a translates to EntityEid
    cb.read(&Rocket_data.eid2, 4 * 8, ecs::ComponentTypeInfo<ecs::EntityId>::type); // 0x57fcec2a translates to EntityEid
    cb.read(&Rocket_data.u1_1, sizeof(Rocket_data.u1_1) * 8, 0);
    cb.read(&Rocket_data.u4_1, sizeof(Rocket_data.u4_1) * 8, 0);
    cb.read(&Rocket_data.u4_2, sizeof(Rocket_data.u4_2) * 8, 0);
    cb.read(&Rocket_data.u12_1, sizeof(Rocket_data.u12_1) * 8, 0);
    cb.read(&Rocket_data.u16_1, sizeof(Rocket_data.u16_1) * 8, 0);
    cb.read(&Rocket_data.u12_2, sizeof(Rocket_data.u12_2) * 8, 0);
    cb.read(&Rocket_data.u12_3, sizeof(Rocket_data.u12_3) * 8, 0);
    cb.read(&Rocket_data.u1_2, sizeof(Rocket_data.u1_2) * 8, 0);
    cb.read(&Rocket_data.u1_3, sizeof(Rocket_data.u1_3) * 8, 0);
    cb.read(&Rocket_data.u4_3, sizeof(Rocket_data.u4_3) * 8, 0);
    cb.read(&Rocket_data.u4_4, sizeof(Rocket_data.u4_4) * 8, 0);
    cb.read(&Rocket_data.u12_4, sizeof(Rocket_data.u12_4) * 8, 0);
    cb.read(&Rocket_data.u12_5, sizeof(Rocket_data.u12_5) * 8, 0);
    cb.read(&Rocket_data.u1_4, sizeof(Rocket_data.u1_4) * 8, 0);
    cb.read(&Rocket_data.u4_5, sizeof(Rocket_data.u4_5) * 8, 0);
    uint16_t size1;
    cb.read(&size1, 2 * 8, 0);
    size1 = (uint16_t)((size1 + 7) & 0xfffffff8);
    Rocket_data.v1.resize(size1);
    cb.read(Rocket_data.v1.data(), size1, 0);
    uint16_t size2;
    cb.read(&size2, 2 * 8, 0);
    size2 = (uint16_t)((size2 + 7) & 0xfffffff8);
    Rocket_data.v1.resize(size2);
    cb.read(Rocket_data.v1.data(), size2, 0);
    cb.read(&Rocket_data.u1_5, sizeof(Rocket_data.u1_5) * 8, 0);
    cb.read(&Rocket_data.u4_6, sizeof(Rocket_data.u4_6) * 8, 0);
    cb.read(&Rocket_data.u1_6, sizeof(Rocket_data.u1_6) * 8, 0);
    cb.read(&Rocket_data.u8_1, sizeof(Rocket_data.u8_1) * 8, 0);
    return true;
  }


  void RendInstDescSerializer::serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
    auto &InstDescData = *(rendinst::RendInstDesc *) data;
    write_compressed(cb, InstDescData.v1);
    write_compressed(cb, InstDescData.v2);
    if(InstDescData.v2)
      write_compressed(cb, InstDescData.v2);
  }

  bool RendInstDescSerializer::deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
    auto &InstDescData = *(rendinst::RendInstDesc *) data;
    bool ok = read_compressed(cb, InstDescData.v1);
    ok &= read_compressed(cb, InstDescData.v2);
    if(InstDescData.v2)
      ok &= read_compressed(cb, InstDescData.v3);
    return ok;
  }


  static FieldSerializerDictIO field_serializer_dict;
  static RendInstDescSerializer rend_inst_desc_serializer;
  ECS_REGISTER_CTM_TYPE(BarrageBalloonStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(LightVehicleModelStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(FortificationModelStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(WalkerVehicleStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(HumanStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(InfantryTroopStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(WarShipModelStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(HeavyVehicleModelStorageComponent, &field_serializer_dict);
  ECS_REGISTER_CTM_TYPE(FlightModelWrapStorageComponent, &field_serializer_dict);

  static RocketSerializer rocket_serializer;
  ECS_REGISTER_CTM_TYPE(Rocket, &rocket_serializer);
  ECS_REGISTER_CTM_TYPE(Payload, &rocket_serializer);
  ECS_REGISTER_CTM_TYPE(Bomb, &rocket_serializer);
  ECS_REGISTER_CTM_TYPE(Jettisoned, &rocket_serializer);
  ECS_REGISTER_CTM_TYPE(Torpedo, &rocket_serializer);

  //ECS_REGISTER_CTM_TYPE(ProjectilePhysObject, &UInt8List_list_serializer);
  ECS_REGISTER_CTM_TYPE(LootModelRes, nullptr);
  ECS_REGISTER_CTM_TYPE(RiExtraComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(rendinst::RendInstDesc, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimV20::AnimcharBaseComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimV20::AnimcharRendComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimatedPhys, nullptr);
  ECS_REGISTER_CTM_TYPE(EffectorData, nullptr);
  ECS_REGISTER_CTM_TYPE(HumanAnimcharSound, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimcharSound, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimcharResourceReferenceHolder, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimcharNodesMat44, nullptr);
  ECS_REGISTER_CTM_TYPE(dm::ExplosiveProperties, nullptr);
  ECS_REGISTER_CTM_TYPE(dm::splash::Properties, nullptr);
  ECS_REGISTER_CTM_TYPE(AmmoStowageMassToSplashList, nullptr);
  ECS_REGISTER_CTM_TYPE(AmmoStowageSlotCollAndGeomNodesList, nullptr);
  ECS_REGISTER_CTM_TYPE(dm::DamagePartProps, nullptr);
  ECS_REGISTER_CTM_TYPE(GameObjects, nullptr);
  ECS_REGISTER_CTM_TYPE(CollisionResource, nullptr);
  ECS_REGISTER_CTM_TYPE(HumanAnimCtx, nullptr);
  ECS_REGISTER_CTM_TYPE(PlaneActor, nullptr);
  ECS_REGISTER_CTM_TYPE(OffenderData, nullptr);
  ECS_REGISTER_CTM_TYPE(TwoPhysicalTracks, nullptr);
  ECS_REGISTER_CTM_TYPE(BreachOffenderDataList, nullptr);
  ECS_REGISTER_CTM_TYPE(unit::UnitRef, nullptr);
  ECS_REGISTER_CTM_TYPE(ResizableDecals, nullptr);
  ECS_REGISTER_CTM_TYPE(UniqueBufHolder, nullptr);
  ECS_REGISTER_MANAGED_TYPE(EnviEmitterId, nullptr, ecs::InplaceCreator<EnviEmitterId>);
  ECS_REGISTER_CTM_TYPE(EffectRef, nullptr);
  ECS_REGISTER_CTM_TYPE(pathfinder::CustomNav, nullptr);
  ECS_REGISTER_CTM_TYPE(walkerai::AgentObstacles, nullptr);
  ECS_REGISTER_CTM_TYPE(walkerai::Target, nullptr);
  ECS_REGISTER_CTM_TYPE(rendinstfloating::PhysFloatingModel, nullptr);
  ECS_REGISTER_CTM_TYPE(ProjectileImpulse, nullptr);
  ECS_REGISTER_CTM_TYPE(CollisionObject, nullptr);
  ECS_REGISTER_CTM_TYPE(SoundEventsPreload, nullptr);
  ECS_REGISTER_CTM_TYPE(SoundVarId, nullptr);
  ECS_REGISTER_CTM_TYPE(SoundStream, nullptr);
  ECS_REGISTER_CTM_TYPE(SoundEvent, nullptr);
  ECS_REGISTER_CTM_TYPE(PhysVars, nullptr);
  ECS_REGISTER_CTM_TYPE(AimComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(UnitCrewLayout, nullptr);
  ECS_REGISTER_CTM_TYPE(unitDmPartFx::UnitDmPartFx, nullptr);
  ECS_REGISTER_CTM_TYPE(UnitFx, nullptr);
  ECS_REGISTER_CTM_TYPE(FuelTanks, nullptr);
  ECS_REGISTER_CTM_TYPE(VehiclePhysActor, nullptr);
  ECS_REGISTER_CTM_TYPE(UnitByEid, nullptr);
  ECS_REGISTER_MANAGED_TYPE(HumanActor, nullptr, ecs::InplaceCreator<HumanActor>);
  ECS_REGISTER_CTM_TYPE(GridHolder, nullptr);
  ECS_REGISTER_CTM_TYPE(GridObjComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(Bullet, nullptr);
  ECS_REGISTER_CTM_TYPE(shells::ShellRef, nullptr);
  ECS_REGISTER_CTM_TYPE(SmokeGridObject, nullptr);
  ECS_REGISTER_CTM_TYPE(BackgroundModelRes, nullptr);
  ECS_REGISTER_CTM_TYPE(FuelLeakEffectMgr, nullptr);
  ECS_REGISTER_CTM_TYPE(ShipSinkingFxMgr, nullptr);
  ECS_REGISTER_CTM_TYPE(unitPhysCls::PhysObjClsNodeActivationMgr, nullptr);
  ECS_REGISTER_CTM_TYPE(TrackSound, nullptr);
  ECS_REGISTER_CTM_TYPE(HudSkinElem, nullptr);
  ECS_REGISTER_CTM_TYPE(GroundEffectManager, nullptr);
  ECS_REGISTER_CTM_TYPE(LensFlareRenderer, nullptr);
  ECS_REGISTER_CTM_TYPE(OutlineContexts, nullptr);
  ECS_REGISTER_CTM_TYPE(GpuObjectRiResourcePreload, nullptr);
  ECS_REGISTER_CTM_TYPE(DestructedModelRes, nullptr);
  ECS_REGISTER_CTM_TYPE(WormVisual, nullptr);

  ECS_REGISTER_CTM_TYPE(SimpleObjectModelResList, nullptr);
  ECS_REGISTER_CTM_TYPE(SimpleObjectModelRes, nullptr);
  ECS_REGISTER_CTM_TYPE(Camera, nullptr);
  ECS_REGISTER_CTM_TYPE(SmokeFx, nullptr);
  ECS_REGISTER_CTM_TYPE(DagorAssetMgr, nullptr);
  ECS_REGISTER_CTM_TYPE(CameraFxEntity, nullptr);
  ECS_REGISTER_CTM_TYPE(AreaFxEntity, nullptr);
  ECS_REGISTER_CTM_TYPE(SpotLightEntity, nullptr);
  ECS_REGISTER_CTM_TYPE(OmniLightEntity, nullptr);
  ECS_REGISTER_CTM_TYPE(TheEffect, nullptr);
  ECS_REGISTER_CTM_TYPE(RiExtraGen, nullptr);
  ECS_REGISTER_CTM_TYPE(MountedGun, nullptr);
  ECS_REGISTER_CTM_TYPE(LightningFX, nullptr);
  ECS_REGISTER_CTM_TYPE(freeAreaCheck::CheckTracesMgr, nullptr);
  ECS_REGISTER_CTM_TYPE(SoundEventGroup, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimationFilterTags, nullptr);
  ECS_REGISTER_CTM_TYPE(FrameFeatures, nullptr);
  ECS_REGISTER_CTM_TYPE(ai::AgentDangers, nullptr);
  ECS_REGISTER_CTM_TYPE(TargetSignatureDetectorContainer, nullptr);
  ECS_REGISTER_CTM_TYPE(GuidanceLockPtr, nullptr);
  ECS_REGISTER_CTM_TYPE(GuidancePtr, nullptr);
  ECS_REGISTER_POD_TYPE(FastPhysTag, nullptr);;
  ECS_REGISTER_CTM_TYPE(AnimIrqToEventComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(ecs::SharedComponent<::ecs::Array>, nullptr);
  ECS_REGISTER_CTM_TYPE(ecs::SharedComponent<::ecs::Object>, nullptr);
  ECS_REGISTER_CTM_TYPE(ecs::SharedComponent<CapsuleApproximation>, nullptr);
  ECS_REGISTER_CTM_TYPE(ecs::SharedComponent<SharedPrecomputedWeaponPositions>, nullptr);
  ECS_REGISTER_CTM_TYPE(PhysRagdoll, nullptr);
  ECS_REGISTER_CTM_TYPE(PhysBody, nullptr);
  ECS_REGISTER_CTM_TYPE(LightingAnimchar, nullptr);
  ECS_REGISTER_CTM_TYPE(HeatSourceId, nullptr);
  ECS_REGISTER_CTM_TYPE(EntityActions, nullptr);
  ECS_REGISTER_CTM_TYPE(GrenadeThrower, nullptr);
  ECS_REGISTER_POD_TYPE(props::PropsId, &prop_serializer);

  ECS_REGISTER_CTM_TYPE(daweap::Gun, nullptr);
  ECS_REGISTER_CTM_TYPE(daweap::LaunchDesc, nullptr);
  ECS_REGISTER_CTM_TYPE(daweap::GunLaunchEvents, nullptr);
  ECS_REGISTER_CTM_TYPE(daweap::GunDeviation, nullptr);
  ECS_REGISTER_CTM_TYPE(daweap::ShellResourceLoader, nullptr);

  ECS_REGISTER_CTM_TYPE(GlobalVisibleCoversMapMAX, nullptr);
  ECS_REGISTER_CTM_TYPE(GlobalVisibleCoversMap4, nullptr);
  ECS_REGISTER_CTM_TYPE(HumanVisibleCoversMap, nullptr);
  ECS_REGISTER_CTM_TYPE(CoversComponent, nullptr);
  ECS_REGISTER_CTM_TYPE(Footstep, nullptr);
  ECS_REGISTER_CTM_TYPE(FootstepFx, nullptr);
  ECS_REGISTER_CTM_TYPE(MotionMatchingController, nullptr);
  ECS_REGISTER_CTM_TYPE(AnimationDataBase, nullptr);
  ECS_REGISTER_CTM_TYPE(CapsulesAOHolder, nullptr);
  ECS_REGISTER_CTM_TYPE(ecs::TemplatesListToInstantiate, nullptr);;
  ECS_REGISTER_CTM_TYPE(BehaviourTree, nullptr);
}

class ErrorSerializer final : public ecs::ComponentSerializer {
public:
  void serialize(ecs::SerializerCb &cb, const void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    EXCEPTION("Unable");
  }

  bool deserialize(const ecs::DeserializerCb &cb, void *data, size_t sz, ecs::component_type_t hint, ecs::EntityManager *mgr) override {
    EXCEPTION("Unable");
  }
};

static ErrorSerializer error_serializer;
ECS_REGISTER_CTM_TYPE(dafg::NodeHandle, nullptr);
ECS_REGISTER_CTM_TYPE(dag::Vector<dafg::NodeHandle>, &error_serializer);


ECS_AUTO_REGISTER_COMPONENT(gpu_objects::riex_handles, "gpu_object_placer__surface_riex_handles", nullptr)
ECS_AUTO_REGISTER_COMPONENT(SoundEventGroup, "sound_event_group", nullptr)
//ECS_AUTO_REGISTER_COMPONENT(SightAvoid, "sight_avoid", nullptr)
//ECS_AUTO_REGISTER_COMPONENT(BehaviourTree, "beh_tree", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ai::AgentDangers, "agent_dangers", nullptr)
ECS_AUTO_REGISTER_COMPONENT(pathfinder::CustomNav, "walker_custom_nav", nullptr)
ECS_AUTO_REGISTER_COMPONENT(walkerai::AgentObstacles, "agent_obstacles", nullptr)
//ECS_AUTO_REGISTER_COMPONENT(walkerai::EntityAgent, "walker_agent", nullptr)
ECS_AUTO_REGISTER_COMPONENT(walkerai::Target, "ai_target", nullptr)
ECS_AUTO_REGISTER_COMPONENT(rendinstfloating::PhysFloatingModel, "floatingRiGroup__riPhysFloatingModel", nullptr)
//ECS_AUTO_REGISTER_COMPONENT(GunShellEjection, "gun_shell_ejection", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ProjectileImpulse, "projectile_impulse", nullptr)
ECS_AUTO_REGISTER_COMPONENT(CollisionResource, "collres", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "collres__res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(PhysVars, "phys_vars", nullptr)
ECS_AUTO_REGISTER_COMPONENT(FastPhysTag, "animchar_fast_phys", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "animchar_fast_phys__res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimatedPhys, "anim_phys", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<SharedPrecomputedWeaponPositions>, "precomp_weapon_pos", nullptr)
ECS_AUTO_REGISTER_COMPONENT(DPoint3, "camera__look_at", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::EntityId, "gun__owner", nullptr)
ECS_AUTO_REGISTER_COMPONENT(bool, "is_watched_sound", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<::ecs::Array>, "human_steps_sound__irqs", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<::ecs::Object>, "human_melee_sound__irqs", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<::ecs::Object>, "human_voice_sound__irqs", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<::ecs::Object>, "sound_irqs", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimIrqToEventComponent, "anim_irq_to_event_converter", nullptr)
ECS_AUTO_REGISTER_COMPONENT(float, "animchar_render__dist_sq", nullptr)
ECS_AUTO_REGISTER_COMPONENT(vec4f, "animchar_render__root_pos", nullptr)
ECS_AUTO_REGISTER_COMPONENT(uint16_t, "animchar_visbits", nullptr)
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_attaches_bbox_precalculated", nullptr)
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_attaches_bbox", nullptr)
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_shadow_cull_bbox", nullptr)
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_bbox", nullptr)
ECS_AUTO_REGISTER_COMPONENT(vec4f, "animchar_bsph_precalculated", nullptr)
ECS_AUTO_REGISTER_COMPONENT(vec4f, "animchar_bsph", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimV20::AnimcharRendComponent, "animchar_render", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimcharNodesMat44, "animchar_node_wtm", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimV20::AnimcharBaseComponent, "saved_animchar", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimV20::AnimcharBaseComponent, "animchar", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "animchar__res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::Object, "animchar__animState", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AnimcharResourceReferenceHolder, "animchar__resRefHolder", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::FireDamageComponent, "fire_damage", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::FireDamageState, "fire_damage_state", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GlobalVisibleCoversMapMAX, "globalVisibleCoversMapMAX", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GlobalVisibleCoversMap4, "globalVisibleCoversMap4", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HumanVisibleCoversMap, "humanVisibleCoversMap", nullptr)
ECS_AUTO_REGISTER_COMPONENT(CoversComponent, "covers", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GridObjComponent, "grid_obj", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GridHolder, "grid_holder", nullptr)
ECS_AUTO_REGISTER_COMPONENT(rendinst::riex_handle_t, "ri_extra__handle", &rendinst_serializer)
ECS_AUTO_REGISTER_COMPONENT(RiExtraComponent, "ri_extra", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "msg_sink", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HumanAnimCtx, "human_anim", nullptr)
ECS_AUTO_REGISTER_COMPONENT(TMatrix, "transform", &transform_serializer)
ECS_AUTO_REGISTER_COMPONENT(PlaneActor, "plane_net_phys", nullptr)
ECS_AUTO_REGISTER_COMPONENT(OffenderData, "damage_area__offender", nullptr)
ECS_AUTO_REGISTER_COMPONENT(TrackSound, "track_sound", nullptr)
ECS_AUTO_REGISTER_COMPONENT(PhysRagdoll, "ragdoll", nullptr)
ECS_AUTO_REGISTER_COMPONENT(PhysBody, "phys_body", nullptr)
ECS_AUTO_REGISTER_COMPONENT(LightingAnimchar, "animchar_storage", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HeatSourceId, "heat_source__id", nullptr)
ECS_AUTO_REGISTER_COMPONENT(EffectRef, "ecs_fx__effect_ref", nullptr)
ECS_AUTO_REGISTER_COMPONENT(CameraFxEntity, "camera_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AreaFxEntity, "area_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(SpotLightEntity, "spot_light", nullptr)
ECS_AUTO_REGISTER_COMPONENT(OmniLightEntity, "omni_light", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GroundEffectManager, "ground_effect__manager", nullptr)
ECS_AUTO_REGISTER_COMPONENT(TheEffect, "effect", nullptr)
ECS_AUTO_REGISTER_COMPONENT(FootstepFx, "footstep_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Footstep, "footstep", nullptr)
ECS_AUTO_REGISTER_COMPONENT(FuelLeakEffectMgr, "fuel_leak_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ShipSinkingFxMgr, "ship_sinking_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(BackgroundModelRes, "background_model_res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(RiExtraGen, "ri_extra_gen", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<CapsuleApproximation>, "capsule_approximation", nullptr)
ECS_AUTO_REGISTER_COMPONENT(MountedGun, "mounted_gun", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::EidList, "human_weap__gunEids", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<::ecs::Object>, "fastThrowSlots", nullptr)
ECS_AUTO_REGISTER_COMPONENT(props::PropsId, "gun__propsId", nullptr)
ECS_AUTO_REGISTER_COMPONENT(daweap::ShellResourceLoader, "shell_resource_loader", nullptr)
ECS_AUTO_REGISTER_COMPONENT(daweap::Gun, "gun", nullptr)
ECS_AUTO_REGISTER_COMPONENT(mat44f, "weapon_rearsight_node__nodeTm", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::Array, "gun__firingModeNames", nullptr)
ECS_AUTO_REGISTER_COMPONENT(daweap::GunLaunchEvents, "gun_launch_events", nullptr)
ECS_AUTO_REGISTER_COMPONENT(daweap::GunDeviation, "gun_deviation", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GuidanceLockPtr, "guidance_lock", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GuidancePtr, "guidance", nullptr)
ECS_AUTO_REGISTER_COMPONENT(GrenadeThrower, "grenade_thrower", nullptr)
ECS_AUTO_REGISTER_COMPONENT(EntityActions, "actions", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HudSkinElem, "hud_skin_elem", nullptr)
ECS_AUTO_REGISTER_COMPONENT(SimpleObjectModelResList, "simpleObject__model_res_list", nullptr)
ECS_AUTO_REGISTER_COMPONENT(SimpleObjectModelRes, "simpleObject__model_res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(LootModelRes, "loot__model_res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(DPoint3, "camera__accuratePos", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Camera, "shooter_cam", nullptr)
ECS_AUTO_REGISTER_COMPONENT(TargetSignatureDetectorContainer, "target_signature_detector", nullptr)
ECS_AUTO_REGISTER_COMPONENT(SmokeFx, "smoke__smoke_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(SmokeGridObject, "smoke__grid_object", nullptr)
ECS_AUTO_REGISTER_COMPONENT(shells::ShellRef, "shell_ref", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Rocket, "rocket_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ProjectilePhysObject, "proj__physObj", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Payload, "payload_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Jettisoned, "jettisoned_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Bullet, "bullet_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Bomb, "bomb_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(Torpedo, "torpedo_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(WormVisual, "worm_visual", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "ship_cover_parts__innerPartsId", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "ship_breaches__partId", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "ship_cover_parts__partId", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartId, "ship_breaches__fatalPartId", nullptr)
ECS_AUTO_REGISTER_COMPONENT(BreachOffenderDataList, "ship_breaches__offender", nullptr)
ECS_AUTO_REGISTER_COMPONENT(TwoPhysicalTracks, "physical_tracks", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HumanActor, "human_net_phys", nullptr)
ECS_AUTO_REGISTER_COMPONENT(VehiclePhysActor, "vehicle_net_phys", nullptr)
ECS_AUTO_REGISTER_COMPONENT(DestructedModelRes, "destructed_model_res", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AimComponent, "aim_component", nullptr)
ECS_AUTO_REGISTER_COMPONENT(unit::UnitRef, "unit__ref", nullptr)
ECS_AUTO_REGISTER_COMPONENT(BarrageBalloonStorageComponent, "unit_storage__barrage_balloon", nullptr)
ECS_AUTO_REGISTER_COMPONENT(LightVehicleModelStorageComponent, "unit_storage__light_vehicle", nullptr)
ECS_AUTO_REGISTER_COMPONENT(FortificationModelStorageComponent, "unit_storage__fortification", nullptr)
ECS_AUTO_REGISTER_COMPONENT(WalkerVehicleStorageComponent, "unit_storage__walker", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HumanStorageComponent, "unit_storage__human", nullptr)
ECS_AUTO_REGISTER_COMPONENT(InfantryTroopStorageComponent, "unit_storage__infantry_troop", nullptr)
ECS_AUTO_REGISTER_COMPONENT(WarShipModelStorageComponent, "unit_storage__ship", nullptr)
ECS_AUTO_REGISTER_COMPONENT(HeavyVehicleModelStorageComponent, "unit_storage__tank", nullptr)
ECS_AUTO_REGISTER_COMPONENT(FlightModelWrapStorageComponent, "unit_storage__aircraft", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "repair_system_parts_id", nullptr)
ECS_AUTO_REGISTER_COMPONENT(UnitFx, "unit_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(unitDmPartFx::UnitDmPartFx, "unit_dm_part_fx", nullptr)
ECS_AUTO_REGISTER_COMPONENT(UnitCrewLayout, "unit_crew", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "tank_engine__radiator_parts", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "tank_engine__engine_parts", nullptr)
ECS_AUTO_REGISTER_COMPONENT(OffenderData, "tank_engine__overheatOffender", nullptr)
ECS_AUTO_REGISTER_COMPONENT(unitPhysCls::PhysObjClsNodeActivationMgr, "phys_obj_cls_activation", nullptr)
ECS_AUTO_REGISTER_COMPONENT(FuelTanks, "fuel_tanks", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "ammo_slots__partId", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::PartIdList, "ammo_stowage__relatedPartsIds", nullptr)
ECS_AUTO_REGISTER_COMPONENT(OffenderData, "ammo_stowage__delayedOffender", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::ExplosiveProperties, "ammo_stowage__explosiveProperties", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::splash::Properties, "ammo_stowage__splashProperties", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AmmoStowageMassToSplashList, "ammo_stowages__powderMassToSplash", nullptr)
ECS_AUTO_REGISTER_COMPONENT(AmmoStowageSlotCollAndGeomNodesList, "ammo_slots__node", nullptr)
ECS_AUTO_REGISTER_COMPONENT(dm::DamagePartProps, "damagePartProps", nullptr)
ECS_AUTO_REGISTER_COMPONENT(rendinst::RendInstDesc, "ri_extra__riSyncDesc", &ecs::rend_inst_desc_serializer)
ECS_AUTO_REGISTER_COMPONENT(GameObjects, "game_objects", nullptr)
ECS_AUTO_REGISTER_COMPONENT(freeAreaCheck::CheckTracesMgr, "free_area_check_traces", nullptr)
ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "trace_mesh_faces", nullptr) // TODO
ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "gun_shell_phys_ejection", nullptr) // TODO
ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "player__support_units_status_notification_mgr", nullptr) // TODO
ECS_AUTO_REGISTER_COMPONENT(CapsulesAOHolder, "capsules_ao", nullptr);