

#ifndef MYEXTENSION_DATABLOCKSHARED_H
#define MYEXTENSION_DATABLOCKSHARED_H

#include "vartypes.h"
#include "StringTableAllocator.h"
#include "unordered_map"
#include "hash.h"
#include "dag_hashedKeyMap.h"

namespace dblk {

  // most of this is almost directly copied
  class DataBlock;

  class DefaultOAHasher {
  public:
    static hash_t hash_data(std::string_view str) {
      // we only do case sensitive hashing here
      hash_t ret;
      ret = (hash_t) wyhash(str.data(), str.length(), 1);
      return ret ? ret : (hash_t(1) << (sizeof(hash_t) * 8 - 1));
    }
  };

  // if your wondering why I switch alot of cosnt char * to std::string_view
  // its cause stringview pretty hehe

  class DBNameMap {
    HashedKeyMap<hash_t, name_id> hashToStringId; // used for string lookup
    std::vector<uint32_t> strings; // indexed by name_id, represents where each name lives in allocator
    StringTableAllocator allocator; // holds all the strings
  public:
    uint8_t noCollisions() const { return allocator.padding; }
    uint8_t &noCollisions() { return allocator.padding; }
    uint32_t addString(std::string_view str)
    {
      const uint32_t len = (uint32_t)str.length() + 1; // term zero, or one symbol after len. todo: fixme, so we don't memcpy from it
      const uint32_t stringId = (uint32_t)strings.size();
      const uint32_t at = allocator.addDataRaw(str.data(), len);
      const_cast<char *>(allocator.head.data.get() + allocator.get_page_ofs(at))[str.length()] = 0; // ensure term zero
      strings.push_back(at);
      return stringId;
    }
    uint32_t addString(const char *name) { return addString(std::string_view(name)); }
    static inline hash_t string_hash(std::string_view str) { return DefaultOAHasher::hash_data(str); }
    static inline hash_t string_hash(const char *s) { return string_hash(std::string_view(s)); }
    std::string_view getStringDataUnsafe(uint32_t id, uint32_t &max_len) const { return allocator.getDataRawUnsafe(strings[id], max_len); }
    std::string_view getStringDataUnsafe(uint32_t id) const { return allocator.getDataRawUnsafe(strings[id]); }
    inline bool string_equal(std::string_view str_v, const uint32_t id) const
    {
      G_FAST_ASSERT(!str_v.empty());
      uint32_t max_len; //-V779
      auto str_other = getStringDataUnsafe(id, max_len);
      // memcmp is faster but we can only call it, when there is enough data in str
      return (max_len >= str_v.length() ? memcmp(str_other.data(), str_v.data(), str_v.length()) : strncmp(str_other.data(), str_v.data(), str_v.length())) == 0; // we actually probably don't ever
      // collide
    }
    name_id getNameId(std::string_view name, const hash_t hash) const
    {
      if (DAGOR_LIKELY(noCollisions()))
      {
        const int it = hashToStringId.findOr(hash, -1);
        if (it == -1 || !string_equal(name, it))
          return -1;
        return it;
      }
      // check hash collision
      return hashToStringId.findOr(hash, -1, [this, name](uint32_t id) { return string_equal(name, id); });
    }
    name_id getNameId(std::string_view str) const { return getNameId(str, string_hash(str)); }
    name_id getNameId(const char *name) const { return name ? getNameId(std::string_view(name)) : -1; }
    name_id addNameId(std::string_view str, hash_t hash)
    {
      int it = -1;
      if (DAGOR_LIKELY(noCollisions()))
      {
        it = hashToStringId.findOr(hash, -1);
        if (DAGOR_UNLIKELY(it != -1 && !string_equal(str, it)))
        {
          noCollisions() = 0;
          it = -1;
        }
      }
      else
        it = hashToStringId.findOr(hash, -1, [this, str](uint32_t id) { return string_equal(str, id); });
      if (it == -1)
      {
        uint32_t id = addString(str);
        hashToStringId.emplace(hash, eastl::move(id));
        return id;
      }
      return it;
    }
    name_id addNameId(std::string_view str) { return addNameId(str, string_hash(str)); }
    name_id addNameId(const char *name) { return name ? addNameId(std::string_view(name)) : -1; } //-V1071
    void erase(uint32_t id)
    {
      if (id >= strings.size())
        return;
      std::string_view name = getStringDataUnsafe(id);
      hash_t hash = string_hash(name);
      if (DAGOR_LIKELY(noCollisions()))
        hashToStringId.erase(hash);
      else
        hashToStringId.erase(hash, [this, name](uint32_t id) { return string_equal(name, id); });
      for (auto &i : hashToStringId)
        if (i.val() > id)
          i.val()--;
      strings.erase(strings.begin() + id);
    }

    int nameCount() const { return (int)strings.size(); }
    std::string_view getName(name_id name_id) const
    {
      if (uint32_t(name_id) >= strings.size())
        return "";
      return getStringDataUnsafe(name_id);
    }
    size_t totalAllocated() const { return allocator.totalAllocated(); }
    size_t totalUsed() const { return allocator.totalUsed(); }

    void combine_pages()
    {
      // todo: implement me, find optimal start_page size
      return;
    }

    void shrink_to_fit()
    {
      hashToStringId.shrink_to_fit();
      strings.shrink_to_fit();
      // combine_pages();//while this can be potentially benefitial for mem usage, due to reminders in pages, in Enlisted it saves just
      // 7kb of memory, and requires one big chunk of memory (1.3mb)
      allocator.shrink_to_fit();
    }
    void reset(bool erase_only = true)
    {
      hashToStringId.clear();
      allocator.clear();
      strings.clear();
      if (!erase_only)
        shrink_to_fit();
    }
    void clear() { reset(false); }
    void reserve(int n)
    {
      hashToStringId.reserve(n);
      strings.reserve(n);
    }
  };

  class DataBlockShared {
    DBNameMap rw; // rw is all the names added to a specific block
    DBNameMap *ro; // ro represents the nm file in a vromfs, it is owned by the vromfs, so ensure not to destroy vromfs while having active files from it
    // string view as StringTableAllocator doesnt move data;
    HashedKeyMap<hash_t, name_id> lookup;
    uint32_t block_ofs = 0;
    uint32_t param_ofs = 0;
    uint16_t block_count = 0;
    uint16_t param_count = 0;
    mutable volatile int refs = 0;

    std::string_view getName(name_id id) const {
      int roc = !ro ? 0 : ro->nameCount();
      return id < roc ? ro->getName(id) : rw.getName((int) id - roc); //-V1004
    }

    uint32_t nameCount() const { return rw.nameCount() + (!ro ? 0 : ro->nameCount()); }

    bool nameExists(uint32_t id) const { return id < nameCount(); }

    int getNameId(std::string_view name) const {
      auto hash = DBNameMap::string_hash(name);
      const int roc = !ro ? 0 : ro->nameCount();
      if (rw.nameCount()) {
        int id = rw.getNameId(name, hash);
        if (id >= 0)
          return id + roc;
      }
      return roc ? ro->getNameId(name, hash) : -1; //-V1004
    }

    int getNameId(const char *name) const { return getNameId(std::string_view(name)); }

    int addNameId(std::string_view name) {
      auto hash = DBNameMap::string_hash(name);
      const int roc = !ro ? 0 : ro->nameCount();
      int id = roc ? ro->getNameId(name, hash) : -1; //-V1004
      if (id >= 0)
        return id;
      return rw.addNameId(name, hash) + roc;
    }

    int addNameId(const char *name) { return addNameId(std::string_view(name)); }


    char *getDataUnsafe() { return ((char *) this + sizeof(DataBlockShared)); }

    const char *getDataUnsafe() const { return ((const char *) this + sizeof(DataBlockShared)); }

    char *getData() {
      G_ASSERT(block_count);
      return getDataUnsafe();
    }

    const char *getData() const {
      G_ASSERT(block_count);
      return getDataUnsafe();
    }

    DataBlock *getBlock(uint32_t i) {
      G_ASSERT(i < block_count);
      return getROBlockUnsafe(i);
    }

    const DataBlock *getBlock(uint32_t i) const {
      G_ASSERT(i < block_count);
      return getROBlockUnsafe(i);
    }

    Param *getParam(uint32_t i) {
      G_ASSERT(i < param_count);
      return getParamUnsafe(i);
    }

    const Param *getParam(uint32_t i) const {
      G_ASSERT(i < param_count);
      return getParamUnsafe(i);
    }

    uint32_t roDataSize() const { return block_ofs + block_count * sizeof(DataBlock) + sizeof(*this); } // blocks are always at the end

    const DataBlock *getROBlockUnsafe(uint32_t i) const { return ((const DataBlock *) getUnsafe(block_ofs)) + i; }

    DataBlock *getROBlockUnsafe(uint32_t i) { return ((DataBlock *) getUnsafe(block_ofs)) + i; }

    const Param *getParamUnsafe(uint32_t i) const { return ((const Param *) getUnsafe(param_ofs)) + i; }

    Param *getParamUnsafe(uint32_t i) { return ((Param *) getUnsafe(param_ofs)) + i; }


    char *getUnsafe(uint32_t at) { return getDataUnsafe() + at; }

    const char *getUnsafe(uint32_t at) const { return getDataUnsafe() + at; }

    char *get(uint32_t at) {
      G_ASSERT(at < roDataSize());
      return getData() + at;
    }

    const char *get(uint32_t at) const {
      G_ASSERT(at < roDataSize());
      return getData() + at;
    }
  };
}


#endif //MYEXTENSION_DATABLOCKSHARED_H
