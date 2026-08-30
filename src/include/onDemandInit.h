

#ifndef MYEXTENSION_ONDEMANDINIT_H
#define MYEXTENSION_ONDEMANDINIT_H

#include "dag_assert.h"

template<typename T>
struct OnDemandInit {
public:
  OnDemandInit() = default;
  inline explicit operator T *() const { return getPtr(); }
  T &operator*() const { return *getPtr(); }
  T *operator->() const { return getPtr(); }
  T *get() const { return getPtr(); }

  template<typename... Args>
  void initialize(Args &&...args) {
    if (initialized)
      return;
    initialized = true; // some of my objs, notably GState, needs itself during init (I know, I know)
    new (obj) T(std::forward<Args>(args)...);
  }

  ~OnDemandInit() {
    ZoneScoped;
    if (initialized)
      reinterpret_cast<T *>(obj)->~T();
  }

private:
  T *getPtr() const {
    DG_ASSERT(initialized == true);
    return (T *) &obj;
  }
  alignas(T) mutable uint8_t obj[sizeof(T)];
  mutable bool initialized = false;
};

#endif // MYEXTENSION_ONDEMANDINIT_H
