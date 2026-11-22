#pragma once
struct net_delta_t {
  /// the layout of the delta packets of a ground-model sync (also read without a state, see mpi/GMSyncPrescan.h)
  static constexpr uint32_t HISTORY_BITS = 5, INDEX_BITS = 0xd;
  net_delta_t(IMemAlloc *alloc) : allocator(alloc), netDelta(HISTORY_BITS, INDEX_BITS, alloc) {}
  IMemAlloc *allocator;
  net::DeltaComp netDelta{HISTORY_BITS, INDEX_BITS, allocator};
  std::vector<net::DeltaComp::History> histories{};
  net::DeltaComp::History &getHistory(uint16_t uid) {
    if (uid >= histories.size()) {
      histories.resize(uid + 1);
    }
    auto &hist = histories[uid];
    if (!hist.isInited()) {
      // use_cache only ever matters in a network context
      netDelta.initHistory(hist, true, false);
    }
    return hist;
  }
};
