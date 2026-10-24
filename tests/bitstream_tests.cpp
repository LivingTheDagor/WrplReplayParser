#include "daNet/bitStream.h"
#include "idFieldSerializer.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

static int failures = 0;

static void check(bool value, const char *what) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    failures++;
  }
}

static BitStream copy_of(const BitStream &bs) { return BitStream(bs.GetData(), bs.GetNumberOfBytesUsed(), true); }

static uint32_t aligned(uint32_t bits) { return (bits + 7) & ~7u; }

// ReadBits gives whole bytes first and right-aligns a last partial byte. This builds the same layout from
// single-bit reads, which use none of the ReadBits fast paths.
static std::vector<uint8_t> read_bit_by_bit(const BitStream &bs, uint32_t bits) {
  std::vector<uint8_t> out((bits + 7) / 8, 0);
  for (uint32_t i = 0; i < bits; ++i) {
    bool bit = false;
    bs.Read(bit);
    const uint32_t byte = i / 8;
    const uint32_t width = byte == bits / 8 ? bits % 8 : 8;
    if (bit)
      out[byte] |= uint8_t(1u << (width - 1 - i % 8));
  }
  return out;
}

static void read_bits_matches_single_bits() {
  std::mt19937 rng(12);
  // Short buffers reach the slow path that the 8-byte window cannot use near the end.
  for (uint32_t size: {1u, 3u, 9u, 16u, 40u}) {
    std::vector<uint8_t> data(size);
    for (auto &b: data)
      b = uint8_t(rng());
    const BitStream bs(data.data(), size, false);
    for (uint32_t start = 0; start < 8; ++start)
      for (uint32_t bits = 1; bits <= 64 && start + bits <= size * 8; ++bits) {
        bs.SetReadOffset(start);
        std::vector<uint8_t> fast((bits + 7) / 8, 0);
        check(bs.ReadBits(fast.data(), bits), "ReadBits inside the buffer");
        check(bs.GetReadOffset() == start + bits, "ReadBits moves the cursor by its width");
        bs.SetReadOffset(start);
        check(fast == read_bit_by_bit(bs, bits), "ReadBits equals single-bit reads");
      }
  }
}

static void field_sequence_round_trip() {
  std::mt19937 rng(7);
  std::vector<std::pair<uint64_t, uint32_t>> fields;
  BitStream bs;
  for (int i = 0; i < 2000; ++i) {
    const uint32_t bits = 1 + rng() % 64;
    const uint64_t value = (uint64_t(rng()) << 32 | rng()) & (bits == 64 ? ~0ull : (1ull << bits) - 1);
    // WriteBits takes the same layout as ReadBits gives: whole bytes, then a right-aligned partial byte.
    uint8_t raw[8]{};
    for (uint32_t b = 0; b < bits; ++b)
      if (value >> (bits - 1 - b) & 1)
        raw[b / 8] |= uint8_t(1u << ((b / 8 == bits / 8 ? bits % 8 : 8) - 1 - b % 8));
    bs.WriteBits(raw, bits);
    fields.emplace_back(value, bits);
  }
  uint32_t cursor = 0;
  for (auto [value, bits]: fields) {
    std::vector<uint8_t> want = read_bit_by_bit(bs, bits);
    bs.SetReadOffset(cursor);
    uint8_t got[8]{};
    check(bs.ReadBits(got, bits), "read back a written field");
    check(std::equal(want.begin(), want.end(), got), "field value survives the round trip");
    cursor += bits;
    check(bs.GetReadOffset() == cursor, "cursor after each field");
  }
  check(bs.GetNumberOfUnreadBits() == 0, "no bits left after the last field");
}

static void compressed_boundaries() {
  const uint32_t values[] = {0, 1, 127, 128, 16383, 16384, (1u << 21) - 1, 1u << 21, (1u << 28) - 1, 1u << 28,
                             0xFFFFFFFFu};
  const uint32_t bytes[] = {1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
  for (uint32_t prefix = 0; prefix < 8; ++prefix)
    for (size_t i = 0; i < std::size(values); ++i) {
      BitStream bs;
      for (uint32_t p = 0; p < prefix; ++p)
        bs.Write(true);
      bs.WriteCompressed(values[i]);
      bs.Write(uint8_t(0xA5));
      bs.SetReadOffset(prefix);
      uint32_t v = 0;
      check(bs.ReadCompressed(v) && v == values[i], "compressed value round trip");
      check(bs.GetReadOffset() == prefix + bytes[i] * 8, "compressed value width");
      uint8_t marker = 0;
      check(bs.Read(marker) && marker == 0xA5, "data after a compressed value");

      BitStream cut(bs.GetData(), (prefix + bytes[i] * 8 + 7) / 8 - 1, false);
      cut.SetReadOffset(prefix);
      check(!cut.ReadCompressed(v), "a cut compressed value fails");
    }
}

static void nested_streams() {
  const uint32_t lengths[] = {0, 1, 7, 8, 9, 15, 16, 17, 127, 128, 129, 1000};
  for (uint32_t prefix = 0; prefix < 8; ++prefix)
    for (uint32_t len: lengths) {
      BitStream child;
      for (uint32_t b = 0; b < len; ++b)
        child.Write(b % 3 == 0);
      BitStream outer;
      for (uint32_t p = 0; p < prefix; ++p)
        outer.Write(false);
      outer.Write(child);
      outer.Write(uint8_t(0x5A));

      outer.SetReadOffset(prefix);
      BitStream got;
      check(outer.Read(got), "read a nested stream");
      check(got.GetNumberOfBitsUsed() == len, "nested stream length");
      bool same = true;
      for (uint32_t b = 0; b < len; ++b) {
        bool bit = false;
        same &= got.Read(bit) && bit == (b % 3 == 0);
      }
      check(same, "nested stream content");
      const uint32_t header = prefix + (len < 128 ? 8 : 16);
      check(outer.GetReadOffset() == aligned(header) + aligned(len), "cursor after a nested stream");
      uint8_t marker = 0;
      check(outer.Read(marker) && marker == 0x5A, "data after a nested stream");

      if (len >= 8) {
        BitStream cut(outer.GetData(), (aligned(header) + aligned(len)) / 8 - 1, false);
        cut.SetReadOffset(prefix);
        check(!cut.Read(got), "a cut nested stream fails");
      }
    }
}

static void strings() {
  for (size_t len: {size_t(0), size_t(1), size_t(127), size_t(200)}) {
    BitStream bs;
    const std::string s(len, 'x');
    bs.Write(s);
    std::string got;
    check(bs.Read(got) && got == s, "string round trip");
    BitStream cut(bs.GetData(), bs.GetNumberOfBytesUsed() - 1, false);
    check(len == 0 || !cut.Read(got), "a cut string fails");
  }
  BitStream huge;
  huge.WriteCompressed(0xFFFFFFF0u);
  huge.Write(uint16_t(0x4141));
  std::string got;
  check(!huge.Read(got) && got.empty(), "a 4 GB string length fails before it allocates");
}

// The game writes a 255-field table as: offset, count, body, field ids, size table.
static BitStream table255(const std::vector<std::pair<uint16_t, uint32_t>> &fields) {
  BitStream bs;
  IdFieldSerializer255 w;
  bs.Write(uint16_t(0));
  bs.Write(uint16_t(0));
  for (auto [id, bits]: fields) {
    for (uint32_t b = 0; b < bits; ++b)
      bs.Write(bool(b & 1));
    w.setFieldSize(bits);
    w.setFieldId(id);
  }
  w.writeFieldsIndex(bs, 0);
  w.writeFieldsSize(bs, 0);
  return bs;
}

static void put_u16(BitStream &bs, uint32_t byte, uint16_t v) { memcpy(bs.GetData() + byte, &v, 2); }

static void field_table_255() {
  std::vector<std::pair<uint16_t, uint32_t>> fields{{1, 1}, {2, 8}, {3, 13}, {9, 32}, {40, 200}};
  BitStream bs = table255(fields);
  IdFieldSerializer255 r;
  BitSize_t end = 0;
  check(r.readFieldsSizeAndCount(bs, end) == fields.size(), "255 table field count");
  check(r.readFieldsIndex(bs), "255 table field ids");
  bool same = true;
  for (uint16_t i = 0; i < fields.size(); ++i)
    same &= r.getFieldId(i) == fields[i].first && r.getFieldSize(i) == fields[i].second;
  check(same, "255 table ids and sizes");
  check(bs.GetReadOffset() == 32, "the body starts after the header");
  check(end == bs.GetNumberOfBitsUsed(), "the table ends at the stream end");

  std::vector<std::pair<uint16_t, uint32_t>> many;
  for (uint16_t i = 0; i < 255; ++i)
    many.emplace_back(i + 1, 1 + i % 9);
  BitStream full = table255(many);
  IdFieldSerializer255 rf;
  check(rf.readFieldsSizeAndCount(full, end) == 255 && rf.readFieldsIndex(full), "255 fields read");

  for (uint16_t count: {uint16_t(256), uint16_t(4095)}) {
    BitStream bad = copy_of(bs);
    uint16_t header = 0;
    memcpy(&header, bad.GetData() + 2, 2);
    put_u16(bad, 2, uint16_t((header & 0xF000) | count));
    IdFieldSerializer255 rb;
    check(rb.readFieldsSizeAndCount(bad, end) == 0 && !rb.readFieldsIndex(bad), "more than 255 fields fails");
  }
  for (uint16_t offset: {uint16_t(0), uint16_t(3), uint16_t(0xFFFF)}) {
    BitStream bad = copy_of(bs);
    put_u16(bad, 0, offset);
    IdFieldSerializer255 rb;
    check(rb.readFieldsSizeAndCount(bad, end) == 0, "a size table outside the stream fails");
  }
  {
    // the size table claims more bits than the body holds
    BitStream small = table255({{1, 8}});
    BitStream bad = copy_of(small);
    const uint32_t table = bad.GetNumberOfBytesUsed() - 1;
    bad.GetData()[table] = uint8_t(5 << 5); // size selector 5 = 64 bits
    IdFieldSerializer255 rb;
    check(rb.readFieldsSizeAndCount(bad, end) == 0, "sizes larger than the body fail");
  }
  {
    BitStream shifted = copy_of(bs);
    shifted.SetReadOffset(1);
    IdFieldSerializer255 rb;
    check(rb.readFieldsSizeAndCount(shifted, end) == 0, "an unaligned table fails");
  }
}

static void field_table_32() {
  BitStream bs;
  IdFieldSerializer32 w;
  bs.Write(uint16_t(0));
  bs.WriteCompressed(uint32_t(0b10110));
  for (uint32_t bits: {3u, 16u, 70u}) {
    for (uint32_t b = 0; b < bits; ++b)
      bs.Write(true);
    w.setFieldSize(bits);
  }
  w.writeFieldsSize(bs);
  IdFieldSerializer32 r;
  uint32_t fields = 0;
  check(r.readFieldsSizeAndFlagChecked(bs, fields) && fields == 0b10110, "32 table field mask");
  check(r.getFieldSize(0) == 3 && r.getFieldSize(1) == 16 && r.getFieldSize(2) == 70, "32 table sizes");
  check(bs.GetReadOffset() == 24, "the 32 table body starts after the mask");

  BitStream bad = copy_of(bs);
  put_u16(bad, 0, 0xFFFF);
  check(!r.readFieldsSizeAndFlagChecked(bad, fields), "a 32 size table outside the stream fails");
}

static void past_the_end() {
  BitStream bs;
  bs.Write(uint8_t(1));
  bs.SetReadOffset(20);
  check(bs.GetNumberOfUnreadBits() == 0, "no unread bits past the end");
  bs.IgnoreBits(5);
  check(bs.GetReadOffset() == 20, "IgnoreBits does not move past the end");
  uint8_t b = 0;
  check(!bs.ReadBits(&b, 1), "ReadBits past the end fails");
  bool bit = false;
  check(!bs.Read(bit), "a bit read past the end fails");
  bs.SetReadOffset(0);
  uint8_t big[8]{};
  check(!bs.ReadBits(big, 0xFFFFFFF8u), "a read that wraps the offset fails");
  check(bs.GetReadOffset() == 0, "a failed read does not move the cursor");
}

int main() {
  read_bits_matches_single_bits();
  field_sequence_round_trip();
  compressed_boundaries();
  nested_streams();
  strings();
  field_table_255();
  field_table_32();
  past_the_end();
  std::printf("bitstream_tests: %d failures\n", failures);
  return failures ? 1 : 0;
}
