#include "network/eid.h"
#include "dag_assert.h"
#include "ecs/EntityManager.h"

namespace net {

  bool write_server_eid(ecs::entity_id_t eidVal, BitStream &bs) {
    // bs.WriteCompressed(eidVal);//unoptimized version
    // return true;
    // we optimize it by writing generation separately from idx
    ecs::EntityId id(eidVal);
    uint32_t index = id.index();
    uint32_t generation = id.get_generation();
    G_ASSERTF(generation <= UCHAR_MAX, "{}", eidVal);
    const bool isShortEid = index < (1 << 14);
    const bool isVeryCompressed = isShortEid && generation <= 1;
    if (isVeryCompressed) {
      uint16_t compressedData = 1;         // one bit
      compressedData |= (generation << 1); // one bit
      compressedData |= index << 2;        // 14 bit
      bs.Write(compressedData);            // 16 bits
    } else if (isShortEid) {
      // 3 bytes version will assume generation is less than 1<<7, and 4 bytes version that index is 1<<22 (which is currently limit!)
      uint16_t compressedIndex = 2; // less significant is zero meaning uncompressed, next one means that it's 3 byte version
      compressedIndex |= index << 2;
      bs.Write(compressedIndex);
      bs.Write((uint8_t) generation); // 16 bits
    } else // !isShortEid
    {
      G_ASSERTF(index < (1 << 22), "{}", eidVal);
      uint32_t compressedData = 0; // two zeroes at the end means uncompressed + 4byte version
      compressedData |= index << 2;
      compressedData |= generation << 24;
      bs.Write(uint16_t(compressedData));
      bs.Write(uint16_t(compressedData >> 16));
    }
    return true;
  }

  bool read_server_eid(ecs::entity_id_t &eidVal, const BitStream &bs) {
    // return bs.ReadCompressed(eidVal); //unoptimized version
    uint16_t first16Bit = 0;
    if (!bs.Read(first16Bit))
      return false;
    if (first16Bit & 1) // 2 byte version
      eidVal = ecs::make_eid(first16Bit >> 2, (first16Bit & 2) >> 1);
    else if (first16Bit & 2) // short eid: 3 byte version
    {
      uint8_t generation = 0;
      if (!bs.Read(generation))
        return false;
      eidVal = ecs::make_eid(first16Bit >> 2, generation);
    } else // long eid: 4 byte version
    {
      uint16_t tailData = 0;
      if (!bs.Read(tailData))
        return false;
      uint32_t compressedData = (uint32_t(tailData) << 16) | uint32_t(first16Bit);
      eidVal = ecs::make_eid((compressedData & 0x00ffffff) >> 2, compressedData >> 24);
    }
    return true;
  }

  void write_eid(BitStream &bs, ecs::EntityId eid) { write_server_eid((ecs::entity_id_t) eid, bs); }

  bool read_eid(const BitStream &bs, ecs::EntityId &eid) {
    ecs::entity_id_t serverEid;
    if (read_server_eid(serverEid, bs)) {
      eid = ecs::EntityId(serverEid);
      return true;
    }
    return false;
  }

} // net