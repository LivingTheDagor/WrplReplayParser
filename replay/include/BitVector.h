

#ifndef MYEXTENSION_BITVECTOR_H
#define MYEXTENSION_BITVECTOR_H
#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <initializer_list>

// this honestly should be replaced with the eastl::bitvector or whatever it is because this is LITERALLY based on that
class BitVector : public std::vector<bool> {
public:
  explicit BitVector(bool default_value) { default_val = default_value; }
  bool test(size_t loc, bool default_) {
    if (loc < this->size())
      return (*this)[loc];
    return default_;
  }

  void set(size_t loc, bool value) {
    if (loc >= this->size())
      resize(loc + 1, default_val);
    (*this)[loc] = value;
  }

  bool get(size_t loc) {
    if (loc >= this->size())
      EXCEPTION("Invalid index");
    return (*this)[loc];
  }

private:
  bool default_val = false;
};
#endif // MYEXTENSION_BITVECTOR_H
