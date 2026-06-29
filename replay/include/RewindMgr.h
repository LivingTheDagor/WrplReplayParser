#pragma once

#include <vector>
// used simply because it allows for uint32_t as size_type
// luckily its default is already uint32_t
#include <dag/dag_vector.h>

// if you know the data will most likely be different on every iteration, then set compare to false
// or else it will try to compare every new object so it can not include repeated values
// this will be used alot in reflection, but for something like unit position not really
// skip_to is for stuff like
template<typename Derived, typename T, bool skip_to = false>
class RewindMgr {
public:
  struct TimeState {
    uint32_t time_ms;
    T data;

    bool operator==(const TimeState &) const = default;
  };

protected:
  dag::Vector<TimeState> timeStates;
  uint32_t curr_index = 0;

  inline uint32_t getTime(int index) {
    if (index < 0)
      return 0;
    if (index >= (int) timeStates.size())
      return 0xFFFFFFFF;
    return timeStates[index].time_ms;
  }

  Derived &self() { return *static_cast<Derived *>(this); }
  const Derived &self() const { return *static_cast<const Derived *>(this); }


  template<typename... Args>
  inline TimeState &goToIndex(uint32_t time_ms, Args... args) {
    auto curr_time = getTime(curr_index - 1);
    if (curr_time < time_ms) {
      auto iter = std::upper_bound(this->timeStates.begin() + curr_index, this->timeStates.end(), time_ms,
                                   [](uint32_t val, const TimeState &data) { return val < data.time_ms; });
      curr_index = (uint32_t) std::distance(this->timeStates.begin(), iter);
      self().forward(timeStates[curr_index - 1], std::forward<Args>(args)...);
      return timeStates[curr_index - 1];
    } else {
      auto iter = std::lower_bound(this->timeStates.begin(), this->timeStates.begin() + curr_index, time_ms,
                                   [](const TimeState &data, uint32_t val) { return data.time_ms < val; });

      curr_index = (uint32_t) std::distance(this->timeStates.begin(), iter);
      auto use_idx = curr_index;
      if (iter != timeStates.begin() && !(iter->time_ms <= time_ms)) {
        use_idx--;
      }
      self().backward(this->timeStates[use_idx], args...);
      return timeStates[use_idx];
    }
  }

  template<typename... Args>
  TimeState &iterateToIndex(uint32_t time_ms, Args &&...args) {
    const uint32_t sz = timeStates.size();
    G_ASSERT(sz != 0);

    if (curr_index > 0 && timeStates[curr_index - 1].time_ms > time_ms) {
      while (curr_index > 0 && timeStates[curr_index - 1].time_ms > time_ms) {
        --curr_index;
        self().backward(timeStates[curr_index], std::forward<Args>(args)...);
      }
      return timeStates[curr_index];
    } else {
      while (curr_index < sz && timeStates[curr_index].time_ms <= time_ms) {
        self().forward(timeStates[curr_index], std::forward<Args>(args)...);
        ++curr_index;
      }
      return timeStates[curr_index - 1];
    }
  }

public:
  const dag::Vector<TimeState> &getStates() const { return this->timeStates; }

  template<typename... Args>
  TimeState &rewindTo(uint32_t time_ms, Args... args) {
    curr_index = eastl::clamp(curr_index, (uint32_t) 0, (uint32_t) this->timeStates.size());
    if constexpr (skip_to) {
      return goToIndex(time_ms, args...);
    } else {
      return iterateToIndex(time_ms, args...);
    }
  }

  TimeState &emplaceNew(uint32_t time_ms) {
    this->curr_index = this->timeStates.size() + 1;
    auto &back = this->timeStates.emplace_back();
    back.time_ms = time_ms;
    return back;
  }

  TimeState &getState(uint32_t idx) {
    G_ASSERT(idx < timeStates.size());
    return timeStates[idx];
  }

  TimeState &CompareToPrevious() {
    if (timeStates.size() < 2)
      return timeStates.back();
    auto &prev = timeStates.back();
    auto &prev_prev = timeStates[timeStates.size() - 2];
    if (prev.data == prev_prev.data) {
      timeStates.pop_back();
      curr_index = timeStates.size() + 1;
      return timeStates.back();
    }
    return prev;
  }

  TimeState &RemovePrevious() {
    G_ASSERT(timeStates.size() > 0);
    timeStates.pop_back();
    curr_index = timeStates.size() + 1;
    return timeStates.back();
  }
};
