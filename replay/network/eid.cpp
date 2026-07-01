#include "network/eid.h"
#include "dag_assert.h"
#include "ecs/EntityManager.h"

namespace net {

bool write_server_eid(ecs::entity_id_t eidVal, BitStream &bs) {
    // bs.WriteCompressed(eidVal);//unoptimized version
    // return true;
    // we optimize it by writing generation separately from idx
    ecs::EntityId eid(eidVal);
    uint32_t index = eid.index();
    uint32_t generation = eid.get_generation();
    const bool isShortIdx = index < (1 << 14);
    if (isShortIdx && generation < 2) // 2 bytes
    {
      uint16_t compressedData = 1; // one bit
      compressedData |= (generation << 1); // one bit
      compressedData |= index << 2; // 14 bit
      bs.Write(compressedData); // 16 bits
    } else if (isShortIdx && generation <= (UCHAR_MAX + 2)) // 3 bytes
    {
      G_FAST_ASSERT(generation >= 2);
      uint16_t compressedIndex =
        2; // less significant is zero meaning uncompressed, next one means that it's 3 byte version
      compressedIndex |= index << 2;
      bs.Write(compressedIndex);
      bs.Write((uint8_t) (generation - 2));
    } else // 4 bytes
    {
      G_FAST_ASSERT(
        index <
        (1 << 20)); // Note: encoding allows 20 bits; replication is hard-capped at 16 by `entityIndexToReplicaIndex`
      uint32_t compressedData = 0; // two zeroes at the end means uncompressed + 4byte version
      compressedData |= index << 2;
      compressedData |= generation << 22;
      bs.Write(uint16_t(compressedData));
      bs.Write(uint16_t(compressedData >> 16));
    }
    return true;
}

bool read_server_eid(ecs::entity_id_t &eidVal, const BitStream &bs) {
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
    eidVal = ecs::make_eid(first16Bit >> 2, generation + 2);
  } else // long eid: 4 byte version
  {
    // G_ASSERT((first16Bit & 3) == 0); // Should we assert on corrupted net traffic?
    uint16_t second16Bit = 0;
    if (!bs.Read(second16Bit))
      return false;
    uint32_t compressedData = (uint32_t(second16Bit) << 16) | first16Bit;
    eidVal = ecs::make_eid((compressedData >> 2) & 0xfffff, compressedData >> 22);
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