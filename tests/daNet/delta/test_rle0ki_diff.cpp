// Differential tests: net::delta::rle0ki_decompress against rle0ki_decompress_old.
#include "daNet/delta/rle.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <vector>

using namespace net::delta;

namespace {

  // exact-size heap blocks so ASan sees any write past the end
  struct FreeDel {
    void operator()(uint8_t *p) const { free(p); }
  };
  typedef std::unique_ptr<uint8_t[], FreeDel> Buf;
  Buf mk(size_t n) { return Buf((uint8_t *) malloc(n ? n : 1)); }

  const size_t GUARD = 16;
  const uint8_t GUARD_BYTE = 0xEE;

  struct Result {
    int ret;
    std::vector<uint8_t> out;
    bool guardOk;
  };

  typedef int (*rle0ki_cb)(std::span<uint8_t>, std::span<uint8_t>);

  // decode into a cap byte buffer followed by GUARD guard bytes
  Result decode(rle0ki_cb cb, const std::vector<uint8_t> &srcv, size_t cap) {
    Buf src = mk(srcv.size());
    if (!srcv.empty())
      memcpy(src.get(), srcv.data(), srcv.size());
    Buf dst = mk(cap + GUARD);
    memset(dst.get(), GUARD_BYTE, cap + GUARD);
    Result r;
    r.ret = cb(std::span<uint8_t>(dst.get(), cap), std::span<uint8_t>(src.get(), srcv.size()));
    if (r.ret > 0)
      r.out.assign(dst.get(), dst.get() + r.ret);
    r.guardOk = true;
    for (size_t g = cap; g < cap + GUARD; g++)
      r.guardOk = r.guardOk && dst[g] == GUARD_BYTE;
    return r;
  }

  struct Mismatches {
    int count = 0;
    std::string first;
    void add(const char *what, size_t n, size_t cap, int ra, int rb) {
      if (count++ == 0)
        first = std::string(what) + " n " + std::to_string(n) + " cap " + std::to_string(cap) + " ret old " +
                std::to_string(ra) + " new " + std::to_string(rb);
    }
  };

  // the old decoder may OR one byte past a full buffer before it returns -1, so only the new decoder's guard is checked
  void compare(Mismatches &m, const char *what, const std::vector<uint8_t> &src, size_t cap) {
    Result a = decode(&rle0ki_decompress_old, src, cap);
    Result b = decode(&rle0ki_decompress, src, cap);
    if (a.ret != b.ret || a.out != b.out || !b.guardOk)
      m.add(what, src.size(), cap, a.ret, b.ret);
  }

  // builds a compressed stream nibble by nibble and keeps the 2-bit units it should decode to
  struct Builder {
    std::vector<uint8_t> nib, units;
    void lit(uint8_t v) { // KI2: one unit
      nib.push_back(uint8_t(2 | v << 2));
      units.push_back(v);
    }
    void lit3(uint8_t a, uint8_t b, uint8_t c) { // KI6 + KI8: three units, not all zero
      nib.push_back(uint8_t(1 | a << 2));
      nib.push_back(uint8_t(b | c << 2));
      units.insert(units.end(), {a, b, c});
    }
    void zeros(int n) { // ZR0: 3..6 zero units, ZR1: 7..10
      nib.push_back(n <= 6 ? uint8_t((n - 3) << 2) : uint8_t(3 | (n - 7) << 2));
      units.insert(units.end(), n, 0);
    }
    void zeros39() { // KI6 with a zero unit, then KI8 with all zero units: 39 more zero units (78 bits)
      nib.push_back(1);
      nib.push_back(0);
      units.insert(units.end(), 1 + 39, 0);
    }
    std::vector<uint8_t> bytes() const {
      std::vector<uint8_t> b;
      for (size_t i = 0; i < nib.size(); i += 2)
        b.push_back(uint8_t(nib[i] | (i + 1 < nib.size() ? nib[i + 1] << 4 : 0)));
      b.push_back(nib.size() % 2 ? 111 : 222);
      return b;
    }
    std::vector<uint8_t> expected() const {
      std::vector<uint8_t> b((units.size() + 3) / 4, 0);
      for (size_t i = 0; i < units.size(); i++)
        b[i / 4] |= uint8_t(units[i] << (2 * (i % 4)));
      return b;
    }
  };

  // both decoders must give the built output with an exact and a roomy buffer, and agree on short buffers
  void check_built(Mismatches &m, const char *what, const Builder &bld) {
    std::vector<uint8_t> src = bld.bytes(), want = bld.expected();
    for (size_t cap: {want.size(), want.size() + 9}) {
      Result a = decode(&rle0ki_decompress_old, src, cap);
      Result b = decode(&rle0ki_decompress, src, cap);
      if (a.ret != (int) want.size() || a.out != want || b.ret != (int) want.size() || b.out != want || !b.guardOk)
        m.add(what, src.size(), cap, a.ret, b.ret);
    }
    compare(m, what, src, want.size() - 1);
    compare(m, what, src, want.size() / 2);
  }

} // namespace

TEST_CASE("rle0ki: random streams match the old decoder", "[rle0ki]") {
  std::mt19937_64 rng(4242);
  static const uint8_t zeroish[] = {0x01, 0x00, 0x10, 0x11, 0x03, 0x33, 0x0f, 0xf0,
                                    0x30, 0xc1, 0x1c, 0xff, 0xff, 0x1f, 0xf1};
  Mismatches m;
  for (int iter = 0; iter < 100000; iter++) {
    size_t n = (iter % 50 == 0) ? 1 + rng() % 3000 : 1 + rng() % 96;
    std::vector<uint8_t> src(n);
    int mode = iter % 8;
    for (auto &c: src) {
      uint64_t r = rng();
      switch (mode) {
        case 0: c = (uint8_t) r; break;
        case 1: c = zeroish[r % sizeof zeroish]; break;
        case 2: c = (r & 7) ? zeroish[(r >> 8) % sizeof zeroish] : (uint8_t) (r >> 16); break;
        case 3: c = (uint8_t) (((r & 3) == 1 ? 1 : (r & 15)) | (((r >> 8) & 3) == 0 ? 0 : ((r >> 12) & 15) << 4)); break;
        case 4: c = (r & 1) ? 0x01 : (uint8_t) (r >> 8); break;
        case 5: c = (uint8_t) ((r & 0x55) | ((r >> 8) & 0x11)); break;
        case 6: c = (r % 10) < 7 ? 0xFF : (r & 0x100) ? (uint8_t) (r >> 16) : zeroish[(r >> 9) % sizeof zeroish]; break;
        default: c = (r % 64) ? 0xFF : (uint8_t) (r >> 8); break; // long runs of 0xFF
      }
    }
    src.back() = (rng() & 1) ? 222 : 111;
    if (iter % 97 == 0)
      src.back() = (uint8_t) rng(); // usually a bad parity byte
    size_t full = n * 10 + 16;
    compare(m, "full", src, full);
    compare(m, "10x", src, n * 10);
    compare(m, "short", src, rng() % (full + 1));
    compare(m, "tiny", src, rng() % 24);
  }
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}

TEST_CASE("rle0ki: round trip through rle0ki_compress", "[rle0ki]") {
  std::mt19937_64 rng(777);
  Mismatches m;
  int roundTrips = 0;
  for (int iter = 0; iter < 50000; iter++) {
    size_t n = 1 + rng() % 600;
    std::vector<uint8_t> raw(n, 0);
    int dens = 1 + (int) (rng() % 40); // percent of non-zero bytes, like a delta
    for (auto &c: raw)
      if ((int) (rng() % 100) < dens)
        c = (rng() & 3) ? (uint8_t) rng() : (uint8_t) (1u << (rng() % 8));
    std::vector<uint8_t> comp(n * 2 + 16);
    int cl = rle0ki_compress(std::span<uint8_t>(comp.data(), comp.size()), std::span<uint8_t>(raw.data(), raw.size()));
    if (cl < 0)
      continue;
    comp.resize((size_t) cl);
    roundTrips++;
    // the decoded size is rounded up to whole units, so it may be a little longer than raw, with zero padding
    Result b = decode(&rle0ki_decompress, comp, n + 8);
    bool ok = b.ret >= (int) n && b.guardOk && memcmp(b.out.data(), raw.data(), n) == 0;
    for (int k = (int) n; ok && k < b.ret; k++)
      ok = b.out[k] == 0;
    if (!ok)
      m.add("roundtrip", comp.size(), n + 8, -2, b.ret);
    compare(m, "rt n+8", comp, n + 8);
    compare(m, "rt 10x", comp, (size_t) cl * 10);
    compare(m, "rt exact", comp, n);
    compare(m, "rt short", comp, n - 1);
  }
  REQUIRE(roundTrips > 40000);
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}

TEST_CASE("rle0ki: zero runs crossing a 64-bit output word at every bit offset", "[rle0ki]") {
  Mismatches m;
  // prefix of k literal units puts the run start at bit 2k, so k = 0..63 covers every offset in a word
  for (int k = 0; k < 64 + 8; k++) {
    for (int run = 3; run <= 10; run++) {
      for (int tail = 0; tail < 3; tail++) {
        Builder b;
        for (int i = 0; i < k; i++)
          b.lit(uint8_t(1 + i % 3));
        b.zeros(run);
        for (int i = 0; i < tail; i++)
          b.lit(3);
        check_built(m, "short run", b);
      }
    }
    // several runs back to back, up to a few words of zeros
    for (int runs = 2; runs <= 14; runs++) {
      Builder b;
      for (int i = 0; i < k; i++)
        b.lit(uint8_t(1 + i % 3));
      for (int r = 0; r < runs; r++)
        b.zeros(3 + (r * 5 + k) % 8);
      b.lit(2);
      check_built(m, "run chain", b);
    }
  }
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}

TEST_CASE("rle0ki: zero runs longer than 56 bits", "[rle0ki]") {
  Mismatches m;
  for (int k = 0; k < 64 + 8; k++) {
    for (int count = 1; count <= 5; count++) {
      for (int mix = 0; mix < 3; mix++) {
        Builder b;
        for (int i = 0; i < k; i++)
          b.lit(uint8_t(1 + i % 3));
        for (int c = 0; c < count; c++) {
          b.zeros39(); // 80 bits of zeros
          if (mix == 1)
            b.zeros(3 + c % 8);
          if (mix == 2)
            b.lit3(uint8_t(c % 4), 1, 2);
        }
        if (k % 2)
          b.lit(1);
        check_built(m, "78-bit run", b);
      }
    }
  }
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}

TEST_CASE("rle0ki: runs of 0xFF bytes", "[rle0ki]") {
  std::mt19937_64 rng(99);
  Mismatches m;
  // every 0xFF byte is two ZR1 runs of 10 zero units
  for (int len = 1; len <= 64; len++) {
    for (int pre = 0; pre < 9; pre++) {
      for (uint8_t parity: {(uint8_t) 111, (uint8_t) 222}) {
        std::vector<uint8_t> src;
        for (int i = 0; i < pre; i++)
          src.push_back((uint8_t) rng());
        src.insert(src.end(), len, 0xFF);
        if (pre % 3 == 1)
          src.push_back(0x22); // literals after the run
        src.push_back(parity);
        size_t n = src.size();
        for (size_t cap: {n * 10, n * 5 + 1, n * 5, n * 5 - 1, (size_t) len * 5, (size_t) 1})
          compare(m, "0xFF run", src, cap);
      }
    }
  }
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}

TEST_CASE("rle0ki: truncated streams", "[rle0ki]") {
  std::mt19937_64 rng(31337);
  Mismatches m;
  for (int iter = 0; iter < 300; iter++) {
    size_t n = 1 + rng() % 300;
    std::vector<uint8_t> raw(n, 0);
    for (auto &c: raw)
      if (rng() % 4 == 0)
        c = (uint8_t) rng();
    std::vector<uint8_t> comp(n * 2 + 16);
    int cl = rle0ki_compress(std::span<uint8_t>(comp.data(), comp.size()), std::span<uint8_t>(raw.data(), raw.size()));
    if (cl < 0)
      continue;
    comp.resize((size_t) cl);
    for (int t = 0; t <= cl; t++) {
      // cut as is (the last byte is now a data byte), and cut with a parity byte put back on
      std::vector<uint8_t> cut(comp.begin(), comp.begin() + t);
      compare(m, "cut", cut, n + 8);
      for (uint8_t parity: {(uint8_t) 111, (uint8_t) 222}) {
        std::vector<uint8_t> fixed = cut;
        fixed.push_back(parity);
        compare(m, "cut+parity", fixed, n + 8);
        compare(m, "cut+parity short", fixed, (size_t) t);
      }
    }
  }
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}

TEST_CASE("rle0ki: empty input, parity only and bad parity", "[rle0ki]") {
  Mismatches m;
  for (size_t cap: {0, 1, 8}) {
    compare(m, "empty", {}, cap);
    compare(m, "parity only 111", {111}, cap);
    compare(m, "parity only 222", {222}, cap);
    compare(m, "bad parity", {0x12, 0x34, 0x00}, cap);
    compare(m, "bad parity", {0x12, 0x34, 223}, cap);
  }
  INFO("first mismatch: " << m.first);
  REQUIRE(m.count == 0);
}
