

#ifndef MYEXTENSION_COMPONENTCONSTRUCTOR_H
#define MYEXTENSION_COMPONENTCONSTRUCTOR_H

#include "dag_assert.h"
#include "EASTL/string.h"
#include "utils.h"
#include "rewind.h"
#include <utility>

namespace ecs {
  typedef std::string string;
}

#include "ComponentPrinting.h"

namespace ecs {

  class ComponentTypeManager {
  public:
    virtual void create(void *, EntityManager &, EntityId, component_index_t) {} // allocate (inplace new).


    virtual void *create(EntityManager &, EntityId, component_index_t) { return nullptr; } // allocate (inplace new).
    // We don't have to pass
    // EntityId. But
    // some 1.0/2.0 CMs need
    // that. This is called only
    // for NON_TRIVIAL_CREATE
    virtual void destroy(void *) {} // doesn't make sense to pass EntityId, as it is already killed

    /// copy assignment. This can be non-optimal implementated (destroy(to); return copy(to, from);).But has to has copy
    virtual bool copy(void * /*to*/, const void * /*from*/, component_index_t,
                      ecs::EntityId = INVALID_ENTITY_ID) const {
      return false;
    } // copy constructor. This is called only for NON_TRIVIAL_CREATE, if it is initialized in template

    // comparison. If not implemented, we assume it is always equal.
    virtual bool is_equal(const void * /*to*/, const void * /*from*/) const { return true; }

    /// copy assignment. This can be non-optimal implementated (destroy(to); return copy(to, from);).But has to has copy
    // virtual bool assign(void * /*to*/, const void * /*from*/) const {
    //   return false;
    // } // assign. returns false if NOT assigned (destination is not changed)

    // this is optimized version. Should be conservatively same as if (!is_equal) {return assign();} return false;(i.e.
    // it can presume changes; but if is_equal returns false IT HAS to return result of assign. return true if changed
    // (not equal). "to" after it should be equal to from (i.e. operator = )
    // virtual bool replicateCompare(void *to, const void *from) const {
    //  if (!is_equal(to, from))
    //    return assign(to, from);
    //  return false;
    //}

    virtual void move(void * /*to*/, void * /*from*/) const {
    } // move constructor. This is called only for NON_TRIVIAL_MOVE. Currently
    // we don't support non memcpy moveable types

    virtual void swap(void *, void *) {} // swaps two pointers via std::swap

    virtual std::string toString(void *, int) const { return ""; }

    virtual size_t getMemSize() { return sizeof(*this); }

    // virtual void
    // clear() {} // this is called on each EntityManager::clear(), after all components were removed. That is used for
    //// unrecommended and rare case when type manager has state
    virtual ~ComponentTypeManager() = default;
  };

  template<bool cond, int v, int d = 0>
  using select_int = std::conditional_t<cond, std::integral_constant<int, v>, std::integral_constant<int, d>>;

  template<typename T>
  struct CtorFlavorSelector {
    static constexpr int value =
      std::disjunction<select_int<std::is_constructible<T, EntityId, EntityManager &>::value, 2>,
                       select_int<std::is_constructible<T, EntityId>::value, 1>,
                       select_int<std::is_constructible<T>::value, 0>>::value;
  };

  template<typename T>
  struct ConstructInplaceType {
    template<class U = T>
    static std::enable_if_t<CtorFlavorSelector<U>::value == 2, U *>
    constructInplaceCandidate(void *p, EntityManager &mgr, EntityId eid, ecs::component_index_t) {
      return new (p) U(eid, mgr);
    }

    template<class U = T>
    static std::enable_if_t<CtorFlavorSelector<U>::value == 1, U *>
    constructInplaceCandidate(void *p, EntityManager &, EntityId eid, ecs::component_index_t) {
      return new (p) U(eid);
    }

    template<class U = T>
    static std::enable_if_t<CtorFlavorSelector<U>::value == 0, U *>
    constructInplaceCandidate(void *p, EntityManager &, EntityId, ecs::component_index_t) {
      return new (p) U;
    } // Note: intentionally not zero init

    template<class U = T>
    static std::enable_if_t<CtorFlavorSelector<U>::value == 2, U *> constructCandidate(EntityManager &mgr, EntityId eid,
                                                                                       ecs::component_index_t) {
      return new U(eid, mgr);
    }

    template<class U = T>
    static std::enable_if_t<CtorFlavorSelector<U>::value == 1, U *> constructCandidate(EntityManager &, EntityId eid,
                                                                                       ecs::component_index_t) {
      return new U(eid);
    }

    template<class U = T>
    static std::enable_if_t<CtorFlavorSelector<U>::value == 0, U *> constructCandidate(EntityManager &, EntityId,
                                                                                       ecs::component_index_t) {
      return new U();
    } // Note: intentionally not zero init
  };

  template<class T>
  class InplaceCreator : public ComponentTypeManager {
  public:
    size_t getMemSize() override { return sizeof(*this); }

    void create(void *data, EntityManager &mgr, EntityId eid, component_index_t cidx) override {
      ConstructInplaceType<T>::constructInplaceCandidate(data, mgr, eid, cidx);
    } // allocate (inplace new).
    void *create(EntityManager &mgr, EntityId eid, component_index_t cidx) override {
      return ConstructInplaceType<T>::constructCandidate(mgr, eid, cidx);
    }

    bool copy(void *p, const void *from, component_index_t, ecs::EntityId) const override {
      if constexpr (std::is_copy_constructible_v<T>) {
        new (p) T(*static_cast<const T *>(from));
        return true;
      } else {
        return false;
      }
    }

    bool is_equal(const void *to, const void *from) const override {
      if constexpr (std::equality_comparable<T>) {
        return *(const T *) to == *(const T *) from; // my plans will require that equality is checkable
      }
      return false;
    }

    void destroy(void *p) override {
      G_FAST_ASSERT(p);
      std::destroy_at((T *) p);
    }

    void move(void *to, void *from) const override {
      G_STATIC_ASSERT(std::is_move_constructible<T>::value);
      T *p_to = (T *) to, *p_from = (T *) from;
      std::construct_at(p_to, std::move(*p_from));
    }


    void swap(void *p1, void *p2) override {
      // G_STATIC_ASSERT(std::is_swappable_v<T>);
      std::swap(*static_cast<T *>(p1), *static_cast<T *>(p2));
    }


    std::string toString(void *p, int indent = 0) const override {
      G_FAST_ASSERT(p);
      return toStringImpl<T>(p, indent);
      // ComponentPrinter<T>::print(static_cast<T*>(p));
    }
  };

  // a specialized CTM that just has print() defined, only used for ease of
  template<class T>
  class ReducedCreator : public ComponentTypeManager {
  public:
    size_t getMemSize() override { return sizeof(*this); }

    void create(void *, EntityManager &, EntityId, component_index_t) override {
      EXCEPTION("Called create1 CTM for %s", typeid(T).name());
    }

    void *create(EntityManager &, EntityId, component_index_t) override {
      EXCEPTION("Called create2 CTM for %s", typeid(T).name());
    }

    bool copy(void *, const void *, component_index_t, ecs::EntityId) const override {
      EXCEPTION("Called copy CTM for %s", typeid(T).name());
    }

    void move(void *, void *) const override { EXCEPTION("Called move CTM for %s", typeid(T).name()); }

    bool is_equal(const void *, const void *) const override {
      EXCEPTION("Called is_equal CTM for %s", typeid(T).name());
    }

    void destroy(void *) override { EXCEPTION("Called destroy CTM for %s", typeid(T).name()); }

    void swap(void *p1, void *p2) override { // swap is implemented only because it makes the logic easier
      G_STATIC_ASSERT(std::is_swappable_v<T>);
      std::swap(*static_cast<T *>(p1), *static_cast<T *>(p2));
    }

    std::string toString(void *p, int indent = 0) const override {
      G_FAST_ASSERT(p);
      return toStringImpl<T>(p, indent);
    }
  };

  template<typename T>
  struct CTMFactory {
    static ComponentTypeManager *create() { return new T(); }

    static void destroy(ComponentTypeManager *ptr) { delete ptr; }
  };
} // namespace ecs
#endif // MYEXTENSION_COMPONENTCONSTRUCTOR_H
