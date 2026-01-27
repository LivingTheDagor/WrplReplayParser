//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/*
  usage example :

  class TestUnit : public ReflectableObject
  {
  public:

    DECL_REFLECTION(TestUnit);

    //
    // reflection - serialize/deserialize changed var in serializeChangedReflectables/deserializeReflectables
    //

    // standart number declaration
    // Warning : unsigned type prefix ignored, if you need unsigned type use RVF_UNSIGNED flag explicitly
    REFL_VAR(int, test);

    // could freely interleaves with any other vars or methods
    String nonReflectVar;

    // extended syntax var def, params are:
    // type, var_name, addition flags, num bits to encode (0 for sizeof) and custom coder func (NULL for auto dispatch)
    REFL_VAR_EXT(float, extSyntaxVar, 0, 0, my_coder_func);

    // exclude prev var from reflection
    EXCLUDE_VAR(extSyntaxVar);

    // code float in range 0.f, 500.f in 16 bits
    // sizeof(theFloat) == sizeof(ReflectionVarMeta) + sizeof(float)*3
    REFL_FLOAT_MIN_MAX(theFloat, 0.f, 500.f, 0, 16)

  };

  //
  // for replication define as
  //
  class MyObject : public ReplicatedObject
  {
  public:

    DECL_REPLICATION(MyObject)
  };

  // in *.cpp

  IMPLEMENT_REPLICATION(MyObject)

  // serialize data needed for object creation (this data passed on remote machine in createReplicatedObject())
  virtual void serializeReplicaCreationData(BitStream& bs) const {}

  // this function create new client object (or get existing if it alreay exist (for example on host migration))
  ReplicatedObject* MyObject::createReplicatedObject(BitStream& bs) { return something; }

  to make serialization work:

  1) create object on server as usual
  2) call on server for that object MyObject->genReplicationEvent(myBitStream) and send that bitStream
      to remote machine manually (message id must be writed manually before function call)
  3) on remote machine (client) call ReplicatedObject::onRecvReplicationEvent for received bitStream
    (automatically will be called SomeObject::createReplicatedObject with serialized on server data)

  (same principle is apply for genReplicationEventForAll/onRecvReplicationEventForAll)

  Warning: change flag is not raised when you explicitly change field member in vector type (x,y,z...)
    like that

    reflVec.x = 1.f; // wrong

    do

    reflVec.getForModify().x = 1.f; // right

    instead
*/
#include <string.h>
#include "bitStream.h"
#include <cstddef> // for offsetof macros
#include "fmt/base.h"
#include "fmt/format.h"

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>

#include "mpi.h"

// operation codes for coder func
#define DANET_REFLECTION_OP_ENCODE 0 // encode data
#define DANET_REFLECTION_OP_DECODE 1 // decode data

#define DANET_REFLECTION_MAX_VARS_PER_OBJECT 255
#define DANET_REPLICATION_MAX_CLASSES        256

class Point2;

class DPoint2;

class IPoint2;

class Point3;

class DPoint3;

class IPoint3;

class Point4;

class IPoint4;

class TMatrix;

class Quat;


// it's by design out of namespace (do not force user use our namespace name for reflection var flags declaration)
enum DanetReflectionVarFlags {
  RVF_CHANGED = 1 << 0,
  // from serialization, but still deserializes (split on two flags?)
  RVF_EXCLUDED = 1 << 1,
  // to distinguish newly deserialized vars in ReflObjOnVarsDeserialized() callback (cleared after that)
  RVF_DESERIALIZED = 1 << 2,
  // for float - (-1.f .. 1.f), together with RVF_UNSIGNED - (0.f .. 1.f)
  RVF_NORMALIZED = 1 << 3,
  RVF_UNSIGNED = 1 << 4,
  // like RVF_CALL_HANDLER, but works even with RVF_EXCLUDED (or ReflectableObject::EXCLUDED) flag
  RVF_CALL_HANDLER_FORCE = 1 << 5,

  RVF_UNUSED3 = 1 << 6,

  // marked this flags on deserialize, then called onBeforeVarsDeserialization(), if this method clears this flag - that var will not
  // be deserialized
  RVF_NEED_DESERIALIZE = 1 << 7,
  // this flag used for float & vectors vars, for auto clamp
  RVF_MIN_MAX_SPECIFIED = 1 << 8,

  RVF_UNUSED0 = 1 << 9,
  RVF_UNUSED1 = 1 << 10,

  // call ReflectableObject::onReflectionVarChanged() method when var changed (rpc vars marked with this flag automatically)
  RVF_CALL_HANDLER = 1 << 11,

  RVF_UNUSED2 = 1 << 12,

  // dump serialization/deserialization of this
  RVF_DEBUG = 1 << 13,
  // this flas are user flag, not used directly in reflection but can be used for convenience
  // by user (in declaration & serialization)
  RVF_USER_FLAG = 1 << 14,
  RVF_UNRELIABLE = 1 << 15,
};

namespace danet {
  class ReflectionVarMeta;

  class ReflectableObject;

  class ReplicatedObject;

#define DANET_ENCODER_SIGNATURE int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs

  typedef int (*reflection_var_encoder)(DANET_ENCODER_SIGNATURE);

  template<typename T, bool mt_safe>
  struct List {
    T *head, *tail;

    List() : head(NULL), tail(NULL) {}

    void lock() {}

    void unlock() {}
  };


  template<typename T>
  struct List<T, true> {
    T *head, *tail;

    List() : head(NULL), tail(NULL) {}
  };

#define DANET_DLIST_PUSH(l, who, prevName, nextName) \
  {                                                  \
    if (!who->prevName && (l).head != who)           \
    {                                                \
      if ((l).tail)                                  \
      {                                              \
        (l).tail->nextName = who;                    \
        who->prevName = (l).tail;                    \
        (l).tail = who;                              \
      }                                              \
      else                                           \
      {                                              \
        G_ASSERT(!(l).head);                         \
        (l).head = (l).tail = who;                   \
      }                                              \
    }                                                \
  }

#define DANET_DLIST_REMOVE(l, who, prevName, nextName) \
  {                                                    \
    if (who->prevName)                                 \
      who->prevName->nextName = who->nextName;         \
    if (who->nextName)                                 \
      who->nextName->prevName = who->prevName;         \
    if ((l).head == who)                               \
      (l).head = (l).head->nextName;                   \
    if ((l).tail == who)                               \
      (l).tail = (l).tail->prevName;                   \
    who->prevName = who->nextName = NULL;              \
  }


// vars of this type are linked in one list inside of ReflectableObject
  class ReflectionVarMeta {
  public:
    uint8_t persistentId;
    uint16_t flags;
    uint16_t numBits;
    const char *name;
    reflection_var_encoder coder;
    ReflectionVarMeta *next;
    friend ReflectableObject;

    const char *getVarName() const { return name; }

    template<class T>
    T *getValue() const {
      // Correct pointer arithmetic using char* as base
      return reinterpret_cast<T *>(reinterpret_cast<char *>(const_cast<ReflectionVarMeta *>(this)) +
                                   sizeof(ReflectionVarMeta));
    }

    void setChanged(bool f) {
      flags = f ? (flags | RVF_CHANGED) : (flags & ~RVF_CHANGED);
    }
  };


  template<typename T>
  struct DefaultEncoderChooser;


  template<>
  struct DefaultEncoderChooser<void> {
    static constexpr reflection_var_encoder coder = nullptr; //
  };

  // Primary template for detecting if a type has a valid encoder
  template<typename T, typename = void>
  struct HasValidEncoder : std::false_type {
  };

  // Specialization that checks if DefaultEncoderChooser<T>::coder exists and is not nullptr
  template<typename T>
  struct HasValidEncoder<T, std::void_t<decltype(DefaultEncoderChooser<T>::coder)>> {
    static constexpr bool value = DefaultEncoderChooser<T>::coder != nullptr;
  };


  template<typename T>
  class ReflectionVar : public ReflectionVarMeta {
    T data;

    static constexpr reflection_var_encoder getCoder() {
      G_STATIC_ASSERT(HasValidEncoder<T>::value);
      return DefaultEncoderChooser<T>::coder;
    }

  public:
    ReflectionVar() = delete; // some values NEED to be set
    void init(const char *name, ReflectionVarMeta *next, uint8_t pid, reflection_var_encoder coder = getCoder(),
              uint16_t bits = sizeof(T) << 3) {
      this->name = name;
      this->next = next;
      this->persistentId = pid;
      this->numBits = bits;
      this->coder = coder;
    }

    ReflectionVar(const char *name, ReflectionVarMeta *next, uint8_t pid) : ReflectionVarMeta(), data() {
      init(name, next, pid);
    }

    ReflectionVar(const char *name, ReflectionVarMeta *next, uint8_t pid, reflection_var_encoder coder_)
        : ReflectionVarMeta(), data() {
      init(name, next, pid, coder_);
    }

    ReflectionVar(const char *name, ReflectionVarMeta *next, uint8_t pid, uint16_t bit_count)
        : ReflectionVarMeta(), data() {
      init(name, next, pid, getCoder(), bit_count);
    }

    ReflectionVar(const char *name, ReflectionVarMeta *next, uint8_t pid, uint16_t bit_count,
                  reflection_var_encoder coder_) : ReflectionVarMeta(), data() {
      init(name, next, pid, coder_, bit_count);
    }

    T *Get() const {
      return this->getValue<T>();
    }
  };

  struct Invalid {
  }; // if you say a ReflectionVar is 'invalid', any warnings during parsing are ignored
  // in practice, the InvalidCoder returns 2, which says error silently

  struct ReplicatedObjectCreator {
    int *class_id_ptr;
    const char *name;

    ReplicatedObject *(*create)(BitStream &);
  };

  extern List<ReflectableObject, true> changed_reflectables;
  extern List<ReflectableObject, true> all_reflectables;
  extern ReplicatedObjectCreator registered_repl_obj_creators[DANET_REPLICATION_MAX_CLASSES];
  extern int num_registered_obj_creators;

#define DANET_WATERMARK      _MAKE4C('DNET')
#define DANET_DEAD_WATERMARK _MAKE4C('DEAD')

// class, you need inherit all your objects from
// all this objects are linked in all_reflectables list, as well may be in changed_reflectables list
  class ReflectableObject : public mpi::IObject {
  public:
    enum Flags {
      /* free */
      EXCLUDED = 2, // this object will not synchronized (but will deserializes)
      /* free */
      REPLICATED = 8,        // this is instance of ReplicatedObject
      FULL_SYNC = 16,        // for this objects forced full sync of all wars
      DEBUG_REFLECTION = 32, // like define for all vars of this object RVF_DEBUG
    };
    List<ReflectionVarMeta, false> varList;
    uint32_t debugWatermark;

    ReflectableObject *prevChanged, *nextChanged; // for changed_reflectables list
    ReflectableObject *prev, *next;               // for all_reflectables list

    void setRelfectionFlag(Flags flag) { reflectionFlags |= flag; }

    bool isRelfectionFlagSet(Flags flag) const { return (bool) (reflectionFlags & flag); }

    uint32_t getRelfectionFlags() const { return reflectionFlags; }

  private:
    uint32_t reflectionFlags;

// Note: explicit padding to defeat tail padding optimization (same layout across majority of platforms for compat with das aot
// compiler)
#if !_TARGET_PC_LINUX && _TARGET_64BIT          // Linux have it's own das aot compiler
    char _pad[sizeof(void *) - sizeof(uint32_t)]; //-V730_NOINIT
#endif

    // dummy mpi implementation
    virtual mpi::Message *dispatchMpiMessage(mpi::MessageID /*mid*/) override { return NULL; }

    virtual void applyMpiMessage(const mpi::Message * /*m*/) override {}

  protected: // only childs can create and delete this class
    static const int EndReflVarQuotaNumber = 0;
    static const int SizeReflVarQuotaNumber = 64;

    ReflectableObject(mpi::ObjectID uid = mpi::INVALID_OBJECT_ID) :
        mpi::IObject(uid),
        debugWatermark(DANET_WATERMARK),
        prevChanged(nullptr),
        nextChanged(nullptr),
        prev(nullptr),
        next(nullptr),
        reflectionFlags(EXCLUDED) {}

    ReflectableObject(const ReflectableObject &) = default;

    ~ReflectableObject() {
      checkWatermark();
      disableReflection(true);
      debugWatermark = DANET_DEAD_WATERMARK;
    }

    ReflectableObject &operator=(const ReflectableObject &) = default;

  public:
    void checkWatermark() const {
      if (debugWatermark != DANET_WATERMARK)
      EXCEPTION("Reflection: invalid object {}", fmt::ptr(this));
    }

    void enableReflection() {
      checkWatermark();

      if (reflectionFlags & EXCLUDED) {
        G_ASSERT(!prevChanged);
        G_ASSERT(!nextChanged);

        DANET_DLIST_PUSH(all_reflectables, this, prev, next);

        reflectionFlags &= ~EXCLUDED;
      }
    }

    void disableReflection(bool full) {
      checkWatermark();

      resetChangeFlag();
      if (full) DANET_DLIST_REMOVE(all_reflectables, this, prev, next);
      reflectionFlags |= EXCLUDED;
    }

    int calcNumVars() const {
      checkWatermark();

      int r = 0;
      for (const ReflectionVarMeta *m = varList.head; m; m = m->next)
        ++r;
      return r;
    }

    const ReflectionVarMeta *searchVarByName(const char *name) const {
      checkWatermark();

      for (const ReflectionVarMeta *m = varList.head; m; m = m->next)
        if (strcmp(m->getVarName(), name) == 0)
          return m;
      return NULL;
    }

    int lookUpVarIndex(const ReflectionVarMeta *var) const {
      checkWatermark();

      int idx = 0;
      for (const ReflectionVarMeta *m = varList.head; m; m = m->next, ++idx)
        if (m == var)
          return idx;
      return -1;
    }

    ReflectionVarMeta *getVarByIndex(int idx) const {
      int i = 0;
      for (ReflectionVarMeta *m = varList.head; m; m = m->next, ++i)
        if (idx == i)
          return m;
      return NULL;
    }

    ReflectionVarMeta *getVarByPersistentId(int id) const {
      for (ReflectionVarMeta *m = varList.head; m; m = m->next)
        if (id == m->persistentId)
          return m;
      return NULL;
    }

    void resetChangeFlag() {
      checkWatermark();

      reflectionFlags &= ~FULL_SYNC;
      for (ReflectionVarMeta *m = varList.head; m; m = m->next)
        m->flags &= ~RVF_CHANGED;

      DANET_DLIST_REMOVE(changed_reflectables, this, prevChanged, nextChanged);
    }

    bool isReflObjChanged() const {
      return prevChanged || this == changed_reflectables.head; // TODO: circular linked list
    }

    void forceFullSync() {
      checkWatermark();

      if (reflectionFlags & EXCLUDED)
        return;

      for (ReflectionVarMeta *m = varList.head; m; m = m->next)
        if ((m->flags & RVF_EXCLUDED) == 0)
          m->flags |= RVF_CHANGED;

      reflectionFlags |= FULL_SYNC;

      markAsChanged();
    }

    void markAsChanged() {DANET_DLIST_PUSH(changed_reflectables, this, prevChanged, nextChanged); }

    void relocate(const ReflectableObject *old_ptr) {
      G_ASSERT(old_ptr);
      G_ASSERT(old_ptr != this);
      G_ASSERT((reflectionFlags & EXCLUDED) &&
               (!prev && !next && all_reflectables.head != this)); // only allowed in disabled reflection

#define RELOC(x) (ReflectionVarMeta *)(uintptr_t(x) - uintptr_t(old_ptr) + uintptr_t(this))
      for (ReflectionVarMeta **m = &varList.head; *m; m = &(*m)->next)
        *m = RELOC(*m);
#undef RELOC
    }

    bool checkAndRemoveFromChangedList() {
      checkWatermark();

      for (ReflectionVarMeta *v = varList.head; v; v = v->next)
        if (v->flags & RVF_CHANGED)
          return false;

      reflectionFlags &= ~FULL_SYNC;
      DANET_DLIST_REMOVE(changed_reflectables, this, prevChanged, nextChanged);

      return true;
    }

    // set or unset flag for var (parameters for rpc function marked as well)
    void markVarWithFlag(ReflectionVarMeta *var, uint16_t f, bool set);

    // return how many vars was serialized
    // reset RVF_CHANGED flag for variables and remove object from changed_reflectables list
    // (if none changed wars was left and do_reset_changed_flag==true)
    int
    serialize(BitStream &bs, uint16_t flags_to_have = RVF_CHANGED, bool fth_all = true, uint16_t flags_to_ignore = 0,
              bool fti_all = false, bool do_reset_changed_flag = true);

    virtual bool deserialize(BitStream &bs, int data_size);

    // called automatically before vars deserialization
    virtual void onBeforeVarsDeserialization() {}

    // called automatically after vars deserialization
    virtual void onAfterVarsDeserialization() {}

    // called after every var change (if RVF_CALL_HANDLER specified for it)
    virtual void onReflectionVarChanged(ReflectionVarMeta *, void * /*newVal*/) {}

    // called before serialization (if returned false - no serialization performed, change flags discarded)
    [[nodiscard]] virtual bool isNeedSerialize() const { return true; }
    // implemented automatically in DECL_REFLECTION macros

    virtual const char *getClassName() const = 0;
  };

  class ReplicatedObject : public ReflectableObject {
  public:
    ReplicatedObject(mpi::ObjectID uid = mpi::INVALID_OBJECT_ID) : ReflectableObject(uid) {
      setRelfectionFlag(REPLICATED);
    }

    bool isNetworkOwner() const { return !(isRelfectionFlagSet(EXCLUDED)) && (!prev || all_reflectables.head == this); }

    virtual int getClassId() const = 0; // implemented automatically in DECL_REFLECTION
    // serialize data needed for replica creation (this data is passed to createReplicatedObject())
    virtual void serializeReplicaCreationData(BitStream &bs) const = 0;

    // two proxy functions who create & receieve replication packet
    void genReplicationEvent(BitStream &out_bs) const;

    static ReplicatedObject *onRecvReplicationEvent(BitStream &bs);

    static void genReplicationEventForAll(BitStream &bs);

    static bool onRecvReplicationEventForAll(BitStream &bs);
  };

// serialize changed vars, flags parameter means additional flags must exist (all of them)
// vars with RVF_EXCLUDED do not serializes
// accept flags that must exists in changed vars (all or any of that flags must exist ruled by bool)
// return how many objects was serialized (could be zero)
  int serializeChangedReflectables(BitStream &bs, uint16_t flags_to_have = RVF_CHANGED, bool fth_all = true,
                                   uint16_t flags_to_ignore = 0, bool fti_all = false,
                                   bool do_reset_changed_flag = true);

  int
  serializeAllReflectables(BitStream &bs, uint16_t flags_to_have = 0, bool fth_all = true, uint16_t flags_to_ignore = 0,
                           bool fti_all = false, bool do_reset_changed_flag = true);

// return -1 on error, or how many reflectables was deserialized
  int deserializeReflectables(BitStream &bs, mpi::object_dispatcher resolver, ParserState *state);

  int forceFullSyncForAllReflectables(); // return num reflectables

#define DANET_COMMA ,

#define DANET_DEF_GET_DEBUG_NAME(class_name)                                               \
  const char *getClassName() const override                                                \
  {                                                                                        \
    G_STATIC_ASSERT(danet::IsBase<danet::ReflectableObject DANET_COMMA ThisClass>::Value); \
    return #class_name;                                                                    \
  }


// this method is needed to guarantee independance of executable build process from replication mechanism
// (otherwise you need to keep build exactly the same for local and remote machines)
  void normalizeReplication();

  template<class _Ty>
  class IsPtr;

  template<class _Ty>
  class IsPtr<_Ty *> {
  public:
    static constexpr int Value = 1;
    typedef _Ty Type;
  };

  template<class _Ty>
  class IsPtr {
  public:
    static constexpr int Value = 0;
    typedef _Ty Type;
  };

  template<typename T>
  struct DeRef {
    typedef T Type;
  };
  template<typename T>
  struct DeRef<const T &> {
    typedef T Type;
  };
  template<typename T>
  struct DeRef<T &> {
    typedef T Type;
  };

  template<class Base, class Derived>
  struct IsBase {
    typedef char yes[2];
    typedef char no[1];

    template<class B>
    static yes &f(B *);

    template<class>
    static no &f(...);

    static constexpr int Value = (sizeof(f<Base>((Derived *) 0)) == sizeof(yes));
  };


#define DECL_REFLECTION(class_name, base_class)                                              \
  typedef base_class BaseClass;                                                              \
  typedef class_name ThisClass;                                                              \
  static const int StartReflVarQuotaNumber = BaseClass::EndReflVarQuotaNumber + 1;           \
  static const int EndReflVarQuotaNumber = StartReflVarQuotaNumber + SizeReflVarQuotaNumber; \
  DANET_DEF_GET_DEBUG_NAME(class_name)

#define DECL_REPLICATION_FOOTER(class_name)                                               \
  static int you_forget_to_put_IMPLEMENT_REPLICATION_4_##class_name;                      \
  int getClassId() const override                                                         \
  {                                                                                       \
    G_STATIC_ASSERT(danet::IsBase<danet::ReplicatedObject DANET_COMMA ThisClass>::Value); \
    return you_forget_to_put_IMPLEMENT_REPLICATION_4_##class_name;                        \
  }                                                                                       \
  void serializeReplicaCreationData(BitStream &bs) const override;                 \
  static danet::ReplicatedObject *createReplicatedObject(BitStream &bs);

#define DECL_REPLICATION(class_name, base_class) \
  DECL_REFLECTION(class_name, base_class)        \
  DECL_REPLICATION_FOOTER(class_name)

#define DECL_REPLICATION_NO_REFLECTION(class_name) \
  typedef class_name ThisClass;                    \
  DANET_DEF_GET_DEBUG_NAME(class_name)             \
  DECL_REPLICATION_FOOTER(class_name)

#define DECL_REFLECTION_STUB \
  const char *getClassName() const { return ""; }

#define DECL_REPLICATION_STUB                    \
  DECL_REFLECTION_STUB                           \
  int getClassId() const override { return -1; } \
  void serializeReplicaCreationData(BitStream &) const override {}

#define IMPLEMENT_REPLICATION_BODY(class_name, str_name, templ_name, pref)                                           \
  pref int class_name::you_forget_to_put_IMPLEMENT_REPLICATION_4_##templ_name = -1;                                  \
  static struct ReplicationTypeRegistator4_##class_name                                                              \
  {                                                                                                                  \
    ReplicationTypeRegistator4_##class_name()                                                                        \
    {                                                                                                                \
      G_ASSERT(danet::num_registered_obj_creators < DANET_REPLICATION_MAX_CLASSES);                                  \
      class_name::you_forget_to_put_IMPLEMENT_REPLICATION_4_##templ_name = danet::num_registered_obj_creators;       \
      danet::ReplicatedObjectCreator &c = danet::registered_repl_obj_creators[danet::num_registered_obj_creators++]; \
      c.class_id_ptr = &class_name::you_forget_to_put_IMPLEMENT_REPLICATION_4_##templ_name;                          \
      c.create = &class_name::createReplicatedObject;                                                                \
      c.name = #str_name;                                                                                            \
      std::cout << "REPLICATION: " << c.name << "\n";                                                                                   \
    }                                                                                                                \
  } replication_registrator_4_##class_name;

#define IMPLEMENT_REPLICATION(class_name)                   IMPLEMENT_REPLICATION_BODY(class_name, class_name, class_name, )
#define IMPLEMENT_REPLICATION_TEMPL(class_name, templ_name) IMPLEMENT_REPLICATION_BODY(class_name, class_name, templ_name, template <>)
}