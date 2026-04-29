

#ifndef MYEXTENSION_ONDEMANDINIT_H
#define MYEXTENSION_ONDEMANDINIT_H

template <typename T>
struct OnDemandInit {
public:
  OnDemandInit() = default;
  inline explicit operator T *() const { return getPtr(); }
  T &operator*() const { return *getPtr(); }
  T *operator->() const { return getPtr(); }
  T *get() const { return getPtr(); }

  ~OnDemandInit()
  {
    ZoneScoped;
    if (initialized)
      getPtr()->~T();
  }
private:
  T * getPtr() const {
    if (!initialized) {
      initialized = true;
      return new(obj) T;
    }
    return (T*)&obj;
  }
  alignas(T) mutable uint8_t obj[sizeof(T)];
  mutable bool initialized = false;
  //T * obj = nullptr;
};

#endif //MYEXTENSION_ONDEMANDINIT_H
