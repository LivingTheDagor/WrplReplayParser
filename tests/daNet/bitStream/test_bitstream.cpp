#include "Logger.h"
#include "dag_assert.h"
#define BITSTREAM_TESTS
#include "daNet/bitStream.h"
#include "daNet/daNetTypes.h"
#include "math/dag_Point3.h"
#include <catch2/catch_test_macros.hpp>

#include <chrono>
uint64_t old_time = 0;
uint64_t new_time = 0;

template<uint32_t T>
uint32_t get_rand() {
  return static_cast<uint32_t>(rand() % T);
}

uint32_t get_rand(uint32_t val) { return static_cast<uint32_t>(rand() % val); }

void profile_bitread(uint32_t byte_count, size_t max_bitread) {
  std::vector<uint8_t> bytes(byte_count);
  for (size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = (uint8_t) get_rand<256>();
  }
  danet::BitStream old_bs{bytes.data(), byte_count, false};
  danet::BitStream new_bs{bytes.data(), byte_count, false};
  size_t MAX_BITS = max_bitread;

  while (old_bs.GetReadOffset() < old_bs.GetWriteOffset()) {
    std::array<std::uint8_t, 256> read_to_old{};
    std::array<std::uint8_t, 256> read_to_new{};
    uint32_t curr_bits = get_rand((uint32_t) MAX_BITS);
    if (curr_bits + old_bs.GetReadOffset() > old_bs.GetWriteOffset()) {
      curr_bits = old_bs.GetWriteOffset() - old_bs.GetReadOffset();
    }

    old_bs.ReadBitsOld(read_to_old.data(), curr_bits);
    new_bs.ReadBits(read_to_new.data(), curr_bits);
    memset(read_to_old.data(), 0, read_to_old.size());
    memset(read_to_new.data(), 0, read_to_new.size());
    old_bs.SetReadOffset(old_bs.GetReadOffset() - curr_bits);
    new_bs.SetReadOffset(new_bs.GetReadOffset() - curr_bits);
    auto curr_bytes = BITS_TO_BYTES(curr_bits);
    auto new_start = std::chrono::high_resolution_clock::now();
    new_bs.ReadBits(read_to_new.data(), curr_bits);
    auto new_end = std::chrono::high_resolution_clock::now();
    auto old_start = std::chrono::high_resolution_clock::now();
    old_bs.ReadBitsOld(read_to_old.data(), curr_bits);
    auto old_end = std::chrono::high_resolution_clock::now();
    old_time += std::chrono::duration_cast<std::chrono::nanoseconds>(old_end - old_start).count();
    new_time += std::chrono::duration_cast<std::chrono::nanoseconds>(new_end - new_start).count();
    // G_CHECKF(old_bs.GetReadOffset() == new_bs.GetReadOffset(), "read offset mismatch; expected {:#x} but got {:#x}",
    //          old_bs.GetReadOffset(), new_bs.GetReadOffset());
    CHECK(old_bs.GetReadOffset() == new_bs.GetReadOffset());
    for (size_t i = 0; i < curr_bytes; ++i) {
      CHECK(read_to_old[i] == read_to_new[i]);
      // G_CHECKF(read_to_old[i] == read_to_new[i], "read fail at index; expected {:#x} but got {:#x}", i,
      // read_to_new[i],
      //          read_to_old[i]);
    }
  }
}

TEST_CASE("BitStream read bits") {
  g_log_handler.initialize();
  for (int i = 0; i < 25; ++i) {
    profile_bitread(20000, 16 + 8 * i);
    // LOGI("iter {} max {}", i, 16 + 8 * i);
    // LOGI("Old bit read time: {} ms", (double) old_time / 1000000);
    // LOGI("New bit read time: {} ms", (double) new_time / 1000000);
    old_time = 0;
    new_time = 0;
  }
}


/*std::vector<uint8_t> bytes(20);
for (size_t i = 0; i < bytes.size(); ++i) {
  bytes[i] = (uint8_t) get_rand<256>();
}
BitStream bs{bytes.data(), bytes.size(), false};
uint8_t force_offs;
bs.ReadBits(&force_offs, 5);
LOGI("force_offs: {}", force_offs);
uint8_t read1b;
G_CHECK(bs.Read(read1b));
LOGI("read1b: {}", read1b);
uint16_t read2b;
G_CHECK(bs.Read(read2b));
LOGI("read2b: {}", read2b);
uint8_t read3b[3];
G_CHECK(bs.Read(read3b));
LOGI("read3b: {}; {}; {}", read3b[0], read3b[1], read3b[2]);
uint32_t read4b;
G_CHECK(bs.Read(read4b));
LOGI("read4b: {}", read4b);
uint8_t read5b[5];
G_CHECK(bs.Read(read5b));
LOGI("read5b: {}; {}; {}; {}; {}", read5b[0], read5b[1], read5b[2], read5b[3], read5b[4]);
uint8_t read6b[6];
G_CHECK(bs.Read(read6b));
LOGI("read6b: {}; {}; {}; {}; {}; {}", read6b[0], read6b[1], read6b[2], read6b[3], read6b[4], read6b[5]);
uint8_t read7b[7];
G_CHECK(bs.Read(read7b));
LOGI("read7b: {}; {}; {}; {}; {}; {}; {}", read7b[0], read7b[1], read7b[2], read7b[3], read7b[4], read7b[5],
     read7b[6]);
uint64_t read8;
G_CHECK(bs.Read(read8));
LOGI("read8: {}", read8);
Point3 read_point{};
G_CHECK(bs.Read(read_point));
LOGI("read_point: {}; {}; {}", read_point[0], read_point[1], read_point[2]);*/
