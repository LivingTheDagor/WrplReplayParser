#pragma once
struct net_delta_t;
namespace gmsync {
  class Prescan;
  void detach(net_delta_t &nd);
} // namespace gmsync
struct net_delta_t {
  /// the layout of the delta packets of a ground-model sync (also read without a state, see mpi/GMSyncPrescan.h)
  static constexpr uint32_t HISTORY_BITS = 5, INDEX_BITS = 0xd;
  net_delta_t(IMemAlloc *alloc) : allocator(alloc), netDelta(HISTORY_BITS, INDEX_BITS, alloc) {}
  // A thread may be bringing the histories up to date for the parser (gmsync::take): it stops first.
  ~net_delta_t() { gmsync::detach(*this); }
  net_delta_t(const net_delta_t &) = delete;
  net_delta_t &operator=(const net_delta_t &) = delete;
  IMemAlloc *allocator;
  net::DeltaComp netDelta{HISTORY_BITS, INDEX_BITS, allocator};
  std::vector<net::DeltaComp::History> histories{};
  /// the part whose thread writes the histories now (see gmsync::take); parsing thread only
  gmsync::Prescan *gmsync_link = nullptr;
  /// parts to leave without the thread (it could not keep up); parsing thread only
  uint8_t gmsync_pause = 0;
  uint8_t gmsync_bad = 0; ///< parts in which it could not keep up
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
