#pragma once
#include "dag/dag_vector.h"
#include "StateAllocator.h"
struct ParserState;
class StateRewinder;
class IRewindState {};

class IRewindEvent {
public:
  virtual ~IRewindEvent() = default;
  virtual void forward(ParserState &state) = 0;
  virtual void backward(ParserState &state) = 0;
};

class RewindAction {
  uint32_t time_ms_at{0xFFFFFFFF};
  IRewindEvent *event{};
  friend StateRewinder;

public:
  explicit RewindAction(IRewindEvent *event) : event(event) {};
};

class StateRewinder {
private:
  ParserState *state;
  StateRewinder(ParserState *state);
  ~StateRewinder();

  dag::Vector<RewindAction, StateAllocator::DagAllocType> actions_vector;
  uint32_t curr_index{};

  void rewind_to_ms(ParserState &parser_state, uint32_t time_ms);

  void add_action(ParserState &state, IRewindEvent *action);

  friend ParserState;
};
struct RewindRef;

class IObjectRewindState {
public:
  virtual ~IObjectRewindState() = default;
  virtual void *getPtr() = 0;
  virtual void *reserveOneV() = 0;
  virtual void deleteLastV() = 0;
  virtual void checkAndPushV(ParserState *state) = 0;

protected:
#if LDAG_DBGLEVEL > 0
  void pushBackState(ParserState *state, uint32_t prev, uint32_t curr);
  virtual void rewindForward(uint32_t expected_idx) = 0;
  virtual void rewindBackward(uint32_t expected_idx) = 0;
#else
  void pushBackState(ParserState *state);
  virtual void rewindForward() = 0;
  virtual void rewindBackward() = 0;
#endif
  friend ParserState;
  friend class StateUpdateEvent;
};

/// do_compare tells the state to compare the previous state with the current state and drop it if they are equal
/// take_ownership tells the state to delete the objects when the state is destroyed.
///   assumes type T is a pointer with take_ownership set to true
/// create_default tells the state to created a default, zeroed state as the first value
template<typename T, bool do_compare = true, bool take_ownership = false, bool create_default = true>
class ObjectRewindState : public IObjectRewindState {
public:
  void *reserveOneV() override;
  void deleteLastV() override;
  void checkAndPushV(ParserState *state) override;
  void *getPtr() override;
  struct TimeState {
    uint32_t time_ms;
    T data;

    bool operator==(const TimeState &other) const { return data == other.data; }
  };

private:
  // dag::Vector<TimeState, StateAllocator::DagAllocType> time_states;

  dag::Vector<TimeState> time_states;
  uint32_t curr_index{};
  TimeState *state = nullptr;
#if LDAG_DBGLEVEL > 0
  bool hasReserved = false;
#endif

public:
  bool hasData() const { return state != nullptr; }

  const T *curr() const { return &state->data; }

  const TimeState *currState() const { return state; }

  const auto &history() const { return time_states; }

  uint32_t getCurrIndex() const { return curr_index; }

  explicit ObjectRewindState(); // ParserState *state
  ~ObjectRewindState() override;

protected:
#if LDAG_DBGLEVEL > 0
  void rewindForward(uint32_t expected_idx) override;
  void rewindBackward(uint32_t expected_idx) override;
#else
  void rewindForward() override;
  void rewindBackward() override;
#endif

public:
  /// reserves one state
  /// checkAndPush or deleteLast MUST be called next
  T *reserveOne();

  /// if we know the last state allocated with reserveOne is invalid, then call this
  void deleteLast();

  /// checks if the previous state reserved by reserveOne
  /// uses do_compare to determine if it should drop
  void checkAndPush(ParserState *state);
};

template<typename T, bool do_compare, bool take_ownership, bool create_default>
ObjectRewindState<T, do_compare, take_ownership, create_default>::~ObjectRewindState() {
  if constexpr (take_ownership) {
    auto g_state = _in_destruction_allocator;
    for (auto &s: this->time_states) {
      g_state->_delete(s.data);
    }
  }
}
