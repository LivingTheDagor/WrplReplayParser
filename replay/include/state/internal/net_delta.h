#pragma once
struct net_delta_t {
  net_delta_t(IMemAlloc *alloc) : allocator(alloc), netDelta(5, 0xd, alloc) {}
  IMemAlloc *allocator;
  net::DeltaComp netDelta{5, 0xd, allocator};
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
