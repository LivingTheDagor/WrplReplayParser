#include "daNet/delta/rle.h"
#include <catch2/catch_test_macros.hpp>

typedef int (*rle0ki_cb)(std::span<uint8_t>, std::span<uint8_t>);
void run_decompression(danet::BitStream &in, danet::BitStream &out, rle0ki_cb cb) {
  int maxSize = in.GetNumberOfBytesUsed() * 10; // compression can't be better than 10:1
  out.SetWriteOffset(0);
  out.reserveBits(BYTES_TO_BITS(maxSize));
  int len = -1;
  std::span<uint8_t> dest(out.GetData(), maxSize);
  len = cb(dest, in.getSlice());
  CHECK(len != -1);
  out.SetWriteOffset(BYTES_TO_BITS(len));
}

std::vector<uint8_t> generate_random_buff(size_t size) {
  std::vector<uint8_t> buff(size);
  for (size_t i = 0; i < size; ++i) {
    buff[i] = rand() % 256;
  }
  return buff;
}

void buff_eq(std::span<uint8_t> a, std::span<uint8_t> b) {
  REQUIRE(a.size() == b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    REQUIRE(a[i] == b[i]);
  }
}

TEST_CASE("compare new vs old rle0ki decompress", "[rle0ki]") {
  rle0ki_cb cb1 = &net::delta::rle0ki_decompress;
  rle0ki_cb cb2 = &net::delta::rle0ki_decompress_old;
  for (int i = 128; i < 1024; i++) {
    for (int j = 0; j < 10; j++) {
      auto rand_buff = generate_random_buff(i);
      danet::BitStream decomp_base{rand_buff.data(), rand_buff.size(), false};
      auto ret_bs = net::delta::compress(decomp_base);
      danet::BitStream decomp_into{};
      run_decompression(ret_bs, decomp_into, cb1);
      buff_eq(decomp_base.getSlice(), decomp_into.getSlice());
      run_decompression(ret_bs, decomp_into, cb2);
    }
  }
}
