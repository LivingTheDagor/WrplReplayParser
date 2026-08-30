//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once
#include "Logger.h"
#include <cstdint>
#include "string"
#include <span>
#include "dag_assert.h"
#include "generic/dag_span.h"

class VROMFs;

/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
/// Serialization callbacks.


/// Interface for writing to abstract output stream.
class IGenSave {
public:
  IGenSave() = default;

  IGenSave(const IGenSave &) = default;

  IGenSave(IGenSave &&) = default;

  IGenSave &operator=(const IGenSave &) = default;

  IGenSave &operator=(IGenSave &&) = default;

  /// virtual destructor
  virtual ~IGenSave() {}


  /// @name Abstract stream output methods.
  /// @{

  /// Write data to output stream.
  virtual void write(const void *ptr, int size) = 0;

  /// Try write data to output stream. Might write less than passed in arguments. Default implementation same as write()
  virtual int tryWrite(const void *ptr, int size);

  /// Get current write position in stream.
  virtual int tell() = 0;

  /// Seek to specified position.
  virtual void seekto(int) = 0;

  /// Seek to end of stream (or relative to the end).
  virtual void seektoend(int ofs = 0) = 0;

  /// @}

  /// Returns some name as stream description (e.g., filename for file stream)
  virtual const char *getTargetName() = 0;

  /// Begin block, must be matched with endBlock().
  /// Call beginBlock() again for nested blocks (match with endBlock() of course).
  virtual void beginBlock() = 0;

  /// Begin block, must be matched with endBlock(). Stores also block id (tag) as 32-bit int
  /// Call beginBlock() again for nested blocks (match with endBlock() of course).
  inline void beginTaggedBlock(int tag) {
    beginBlock();
    writeInt(tag);
  }

  /// End block. Call after writing all block data.
  virtual void endBlock(unsigned block_flags_2bits = 0) = 0;

  /// Returns block nesting level (number of calls to beginBlock() without endBlock()).
  virtual int getBlockLevel() = 0;

  /// Flush stream
  virtual void flush() = 0;

  /// Write text string. NOTE: padded with zeros to align on 4 bytes.
  template<typename T>
  inline void writeString(const T &s) {
    int len = s.length();
    if (!len) {
      writeInt(0);
      return;
    }

    writeInt(len);
    write(&s[0], len);
    alignOnDword(len);
  }

  /// Write text string. NOTE: padded with zeros to align on 4 bytes.
  inline void writeString(const char *s) {
    if (!s) {
      writeInt(0);
      return;
    }

    int len = (int) strlen(s);
    writeInt(len);
    write(s, len);
    alignOnDword(len);
  }

  inline void writeString(char *s) { writeString((const char *) s); }

  template<typename T>
  inline void writeShortString(const T &s) {
    int len = s.length();
    if (len > 0xFFFF)
      len = 0xFFFF;

    writeIntP<2>(len);
    write(&s[0], len);
  }

  inline void writeShortString(const char *s) {
    if (!s) {
      writeIntP<2>(0);
      return;
    }

    int len = (int) strlen(s);
    if (len > 0xFFFF)
      len = 0xFFFF;
    writeIntP<2>(len);
    write(s, len);
  }

  inline void writeShortString(char *s) { writeShortString((const char *) s); }

  inline void writeInt(int v) { write(&v, sizeof(int)); }

  inline void writeInt64(int64_t v) { write(&v, sizeof(int64_t)); }

  template<typename T>
  inline void writeObj(const T &v) {
    write(&v, sizeof(T));
  }

  template<int BYTENUM>
  inline void writeIntP(int v) {
    G_STATIC_ASSERT(BYTENUM > 0 && BYTENUM <= 4);
#if _TARGET_CPU_BE
    write(((char *) &v) + (4 - BYTENUM), BYTENUM);
#else
    write(&v, BYTENUM);
#endif
  }

  inline void writeReal(float v) { write(&v, sizeof(float)); }

  // write data pointed by generic slice (or derived container) to stream
  template<typename V, typename T = typename V::value_type>
  inline void writeTabData(const V &tab) {
    write(tab.data(), data_size(tab));
  }

  // write array (count + data) pointed by generic slice (or derived container) to stream
  template<typename V, typename T = typename V::value_type>
  inline void writeTab(const V &tab) {
    writeInt(tab.size()); //== prevent replace
    writeTabData(tab);
  }

  // the same as writeTab, but also stores elemsize to check element size in IGenLoad::readTabSafe
  template<typename V, typename T = typename V::value_type>
  inline void writeTabSafe(const V &tab) {
    writeInt(elem_size(tab));
    writeTab(tab);
  }

  // Write zeros if needed to align on 4 bytes boundary. Used by writeString().
  inline void alignOnDword(int len) {
    // alignment! all strings are aligned on 4 bytes boundary
    if (len & 3) {
      int zero = 0;
      write(&zero, 4 - (len & 3));
    }
  }
};

inline int IGenSave::tryWrite(const void *ptr, int size) {
  write(ptr, size);
  return size;
}

/// Interface for reading from abstract input stream.
class IGenLoad {
public:
  IGenLoad() = default;

  IGenLoad(const IGenLoad &) = default;

  IGenLoad(IGenLoad &&) = default;

  IGenLoad &operator=(const IGenLoad &) = default;

  IGenLoad &operator=(IGenLoad &&) = default;

  /// virtual destructor
  virtual ~IGenLoad() {}

  /// explicit end of reading; used for decompressor streams
  virtual bool ceaseReading() { return true; }

  /// @name Abstract stream input methods.
  /// @{

  /// Read data from input stream.
  virtual void read(void *ptr, int size) = 0;

  /// Read data from input stream; returns read size; doesn't generate exception
  virtual int tryRead(void *ptr, int size) = 0;

  /// Get current position in input stream.
  virtual int tell() = 0;

  /// Seek to specified position.
  virtual void seekto(int) = 0;

  /// Seek relative to current position.
  virtual bool seekrel(int) = 0;

  /// @}

  /// Returns some name as stream description (e.g., filename for file stream)
  virtual const char *getTargetName() = 0;

  /// Returns size of target data stream if available, or -1 when size is not known
  virtual int64_t getTargetDataSize() { return -1; }

  /// Begin block. Returns block length (getBlockLength() could also be used).
  /// If out_block_flags!=NULL then 2 bits (block flags) are stored to *out_block_flags.
  virtual int beginBlock(unsigned *out_block_flags = nullptr) = 0;

  /// Begin block. Reads tag as 32-bit int. Returns block tag.
  inline int beginTaggedBlock(unsigned *out_block_flags = nullptr) {
    beginBlock(out_block_flags);
    return readInt();
  }

  /// Seeks to the end of current block.
  virtual void endBlock() = 0;

  /// Returns total length of current block.
  virtual int getBlockLength() = 0;

  /// Returns remaining block length, uses tell().
  virtual int getBlockRest() = 0;

  /// Returns block nesting level (number of calls to beginBlock() without endBlock()).
  virtual int getBlockLevel() = 0;

  virtual const VROMFs *getTargetVromFs() const { return nullptr; };

  virtual std::span<char> getTargetRomData() const { return {}; }

  // advanced interface
  /// Read data from input stream; returns read size; doesn't generate exception
  inline bool readExact(void *ptr, int size) {
    int rd_part = tryRead(ptr, size);
    if (rd_part == -1)
      return false;
    int rd = 0;
    for (rd = rd_part; rd_part && rd < size; rd += rd_part)
      rd_part = tryRead(rd + (char *) ptr, size - rd);

    // G_ASSERTF(rd == size, "{}: readExact({},{})={}", getTargetName(), ptr, size, rd);
    return rd == size;
  }

  template<typename T>
  inline void readString(T &s) {
    int len = readInt();

    s.resize(len + 1);
    read(&s[0], len);
    alignOnDword(len);
  }

  template<typename T>
  inline void readShortString(T &s) {
    unsigned short len = 0;
    read(&len, sizeof(len));

    s.allocBuffer(len + 1);
    read(&s[0], len);
  }

  inline void readInt64(int64_t &v) { read(&v, sizeof(int64_t)); }

  inline void readInt(int &v) { read(&v, sizeof(int)); }

  inline void readReal(float &v) { read(&v, sizeof(float)); }

  inline int readInt() {
    int v;
    read(&v, sizeof(int));
    return v;
  }

  template<typename T>
  bool readInto(T &v) {
    return readExact(&v, sizeof(T));
  }

  inline int64_t readInt64() {
    int64_t v;
    read(&v, sizeof(int64_t));
    return v;
  }

  template<int BYTENUM>
  inline int readIntP() {
    G_STATIC_ASSERT(BYTENUM > 0 && BYTENUM <= 4);
    int v = 0;
#if _TARGET_CPU_BE
    read(((char *) &v) + (4 - BYTENUM), BYTENUM);
#else
    read(&v, BYTENUM);
#endif
    return v;
  }

  inline float readReal() {
    float v;
    read(&v, sizeof(float));
    return v;
  }

  inline void skipString() {
    int len = readInt();
    EXCEPTION_IF_FALSE(seekrel(len), "");
    alignOnDword(len);
  }

  inline void skipShortString() {
    unsigned short len = 0;
    read(&len, sizeof(len));
    EXCEPTION_IF_FALSE(seekrel(len), "");
  }

  inline void gets(char *s, int n) {
    int i;
    for (i = 0; i < n - 1; i++) {
      if (tryRead(&s[i], 1) != 1)
        break;
      if (s[i] == '\n') {
        if (i > 0 && s[i - 1] == '\r')
          --i;
        break;
      }
    }
    s[i] = 0;
  }

  // fill container from stream
  template<class T>
  inline void readTabData(T &tab) {
    if (data_size(tab))
      read(tab.data(), (size_t) data_size(tab));
  }

  // read counter, resize container and fill container from stream
  template<class T>
  inline void readTab(T &tab) {
    tab.resize(readInt());
    readTabData(tab);
  }

  // the same as readTab, but also checks elemsize written by IGenLoad::readTabSafe
  template<class T>
  inline bool readTabSafe(T &tab) {
    int elemsz = readInt();
    if (elemsz != elem_size(tab)) {
      tab.resize(0);
      EXCEPTION_IF_FALSE(seekrel(elemsz * readInt()), "");
      return false;
    }
    readTab(tab);
    return true;
  }

  inline void alignOnDword(int len) {
    // alignment! all strings are aligned on 4 bytes boundary
    if (len & 3)
      EXCEPTION_IF_FALSE(seekrel(4 - (len & 3)), "");
  }
};

/// @}

/// @}
