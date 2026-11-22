#pragma once
// The layout of a ground-model sync message (MPI GroundModelPositions), apart from what is done with it.
// Included after state/ParserState.h.
#include "daNet/bitCursor.h"
#include "daNet/delta/deltaCompression.h"
#include "dag_assert.h"

namespace gmsync {
  /// what a sync says about one vehicle in front of its data
  struct Unit {
    bool val1 = false;
    bool val2 = false;
    bool val3 = false;
    uint8_t turret_count = 0;
  };

  template<typename Stream>
  BITCURSOR_INLINE void skipSeekerData(Stream &bs) {
    // this will probably never be implemented unless if I actually go through and try to understand what this complex
    // bullshit is
    uint32_t val;
    bs.Read(val);
    auto read_offs = bs.GetReadOffset();
    bs.SetReadOffset(read_offs + val);
    // bool does_read_call_one = bs.ReadBit();
  }

#define GMSYNC_RET_FAIL(op) G_ASSERT_RETURN((op), false);
  /// Reads one sync message. Where a field starts depends on the bytes of the message only, never on the
  /// state of the parser, so the same walk serves the parser and a pass that runs ahead of it on another
  /// thread (GMSyncPrescan). What is done with a vehicle is up to `sink`:
  ///   sink.deltaComp()  the net::DeltaComp whose packet layout the message uses
  ///   sink.delta(uid, unit, header_ok, header, packed)  a vehicle with a delta packet: its header (header_ok
  ///       is false when the message ended inside it) and its packed bytes. Returns false to give up.
  ///   sink.plain(bs, uid, unit)  a vehicle with unpacked data at the position of bs; has to read it (how far
  ///       that goes is the one thing that does depend on the state). Returns false to give up.
  /// Returns false when a field is missing or the sink gave up.
  template<typename Sink>
  BITCURSOR_INLINE bool walk(danet::BitCursor &bs, Sink &sink) {
    uint16_t uid_lower;
    uint16_t uid_upper;
    float time_at;
    uint16_t packet_reliability_order;
    bs.Read(uid_lower);
    bs.Read(uid_upper);
    bs.Read(time_at);
    bs.Read(packet_reliability_order);
    for (; uid_lower < uid_upper; uid_lower++) {
      bool bool1;
      GMSYNC_RET_FAIL(bs.Read(bool1));
      if (bool1) {
        Unit unit{};
        bool bool2;
        uint8_t some_val_idk;
        GMSYNC_RET_FAIL(bs.Read(bool2)); // some weird flag setting based on bool value at unit + 0x90
        if (bs.ReadBit()) {
          GMSYNC_RET_FAIL(bs.template ReadBits<4>(&some_val_idk));
        }
        GMSYNC_RET_FAIL(bs.Read(unit.val2));
        GMSYNC_RET_FAIL(bs.Read(unit.val3));
        if (!unit.val2 || !unit.val3) {
          bool bool5;
          bool isCompressed;
          GMSYNC_RET_FAIL(bs.Read(bool5));
          GMSYNC_RET_FAIL(bs.Read(unit.val1));
          GMSYNC_RET_FAIL(bs.Read(unit.turret_count));
          GMSYNC_RET_FAIL(bs.Read(isCompressed));
          if (isCompressed) {
            net::DeltaComp::DeltaHeader header;
            const uint8_t *packed = nullptr;
            const bool header_ok = sink.deltaComp().readDeltaHeader(bs, header, packed);
            if (!sink.delta(uid_lower, unit, header_ok, header, packed))
              return false;
          } else {
            if (!sink.plain(bs, uid_lower, unit))
              return false;
          }
          bool bool7;
          GMSYNC_RET_FAIL(bs.Read(bool7));
          if (bool7) {
            Point4 tp4; // actually 4 float reads, but meght
            bs.Read(tp4);
          }
          bool bool8;
          GMSYNC_RET_FAIL(bs.Read(bool8));
          if (bool8 && unit.turret_count > 0) { // maybe has_shot_gun???
            for (int i = 0; i < unit.turret_count; i++) {
              {
                // none of this is used: step over all the records that are whole (see skipAmmoRecords). The
                // reads below are for what is left, at the very end of a stream.
                const dataSync::AmmoSkip skipped = dataSync::skipAmmoRecords(
                  bs.GetData(), bs.GetNumberOfBitsUsed(), bs.GetReadOffset(), uint32_t(unit.turret_count - i));
                bs.SetReadOffset(skipped.offset);
                i += (int) skipped.records;
                if (i >= unit.turret_count)
                  break;
              }
              bool t1;
              GMSYNC_RET_FAIL(bs.Read(t1));
              if (t1) {
                uint8_t v1;
                float v2;
                float v3;
                uint8_t v4;
                GMSYNC_RET_FAIL(bs.Read(v1));
                GMSYNC_RET_FAIL(bs.Read(v2));
                GMSYNC_RET_FAIL(bs.Read(v3));
                GMSYNC_RET_FAIL(bs.Read(v4));
                // new ammunition counts!!!!
                // first val is ammo 0, second is ammo 1, ...
                for (int j = 0; j < v4; j++) {
                  uint16_t vv;
                  bs.Read(vv);
                }
              }
            }
          }
          bool has_sensor_info;
          bool has_sensor_info_2;
          bs.Read(has_sensor_info);
          if (has_sensor_info) {
            bs.Read(has_sensor_info_2);
            if (has_sensor_info_2) {
              skipSeekerData(bs);
            }
          }
          bs.AlignReadToByteBoundary();
        }
      }
    }
    return true;
  }
#undef GMSYNC_RET_FAIL
} // namespace gmsync
