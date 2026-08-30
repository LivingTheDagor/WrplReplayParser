//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_baseIo.h>
#include <dag/dag_vector.h>

/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
/// Serialization callbacks.


/// Callback to write into dynamically allocated memory buffer.
class DynamicMemGeneralSaveCB : public IBaseSave {
public:
  DynamicMemGeneralSaveCB(size_t sz = 0, size_t quant = 64 << 10);

  DynamicMemGeneralSaveCB(DynamicMemGeneralSaveCB &&) = default;

  DynamicMemGeneralSaveCB &operator=(DynamicMemGeneralSaveCB &&) = default;

  virtual ~DynamicMemGeneralSaveCB(void);

  void write(const void *ptr, int size) override;

  int tell(void) override;

  void seekto(int ofs) override;

  void seektoend(int ofs = 0) override;

  const char *getTargetName() override { return "(mem)"; }

  virtual void flush() { /*noop*/ }

  void resize(intptr_t sz);

  void setsize(intptr_t sz);

  /// Returns size of written buffer.
  inline intptr_t size(void) const { return datasize; }

  /// Returns pointer to buffer data.
  inline unsigned char *data(void) { return dataptr; }
  inline const unsigned char *data(void) const { return dataptr; }

  /// Returns pointer to copy of buffer data.
  unsigned char *copy(void);

  /// Returns pointer to buffer data and resets internal state (buffer ownership is transferred to caller).
  unsigned char *acquire(void);

protected:
  unsigned char *dataptr;
  intptr_t datasize, data_avail, data_quant;
  intptr_t curptr;
};


/// Constrained save to memory region (without any allocations and reallocations)
/// if memory region size is exceeded during write operations exception is thrown
class ConstrainedMemSaveCB : public DynamicMemGeneralSaveCB {
public:
  ConstrainedMemSaveCB(void *data, int sz) : DynamicMemGeneralSaveCB(0, 0) { setDestMem(data, sz); }
  ~ConstrainedMemSaveCB() { dataptr = nullptr; }

  void setDestMem(void *data, int sz) {
    dataptr = (unsigned char *) data;
    data_avail = sz;
    datasize = curptr = 0;
  }
};

/// Callback for reading from memory buffer. Allocates memory from #globmem allocator.
class MemGeneralLoadCB : public IBaseLoad {
public:
  /// Allocates buffer and copies data to it.
  MemGeneralLoadCB(const void *ptr, int sz);

  MemGeneralLoadCB(const MemGeneralLoadCB &) = default;

  MemGeneralLoadCB(MemGeneralLoadCB &&) = default;

  MemGeneralLoadCB &operator=(const MemGeneralLoadCB &) = default;

  MemGeneralLoadCB &operator=(MemGeneralLoadCB &&) = default;

  ~MemGeneralLoadCB() override;

  void read(void *ptr, int size) override;

  int tryRead(void *ptr, int size) override;

  int tell(void) override;

  void seekto(int ofs) override;

  bool seekrel(int ofs) override;

  const char *getTargetName() override { return "(mem)"; }
  int64_t getTargetDataSize() override { return datasize; }

  std::span<char> getTargetRomData() const override { return {(char *) dataptr, (size_t) datasize}; }

  void close();

  /// Free buffer.
  void clear();

  /// Resize (reallocate) buffer.
  void resize(int sz);

  /// Returns buffer size.
  int size(void);

  /// Returns pointer to buffer data.
  const unsigned char *data(void);

  /// Returns pointer to copy of buffer data.
  unsigned char *copy(void);

protected:
  const unsigned char *dataptr;
  int datasize;
  int curptr;
};


/// In-place (no copy) load from memory interface (fully inline).
class InPlaceMemLoadCB : public MemGeneralLoadCB {
public:
  InPlaceMemLoadCB() : MemGeneralLoadCB(NULL, 0) {}

  InPlaceMemLoadCB(const void *ptr, int sz) : MemGeneralLoadCB(NULL, 0) {
    dataptr = (const unsigned char *) ptr;
    datasize = sz;
  }

  InPlaceMemLoadCB(const InPlaceMemLoadCB &) = default;

  InPlaceMemLoadCB(InPlaceMemLoadCB &&) = default;

  InPlaceMemLoadCB &operator=(const InPlaceMemLoadCB &) = default;

  InPlaceMemLoadCB &operator=(InPlaceMemLoadCB &&) = default;

  ~InPlaceMemLoadCB() { dataptr = NULL; }

  const void *readAny(int sz) {
    const void *p = dataptr + curptr;
    seekrel(sz);
    return p;
  }

  void setSrcMem(const void *data, int sz) {
    dataptr = (const unsigned char *) data;
    datasize = sz;
    curptr = 0;
  }
};

enum class StreamDecompressResult {
  FAILED,
  FINISH,
  NEED_MORE_INPUT,
};

struct IStreamDecompress {
  virtual ~IStreamDecompress() {}

  virtual StreamDecompressResult decompress(std::span<uint8_t> in, std::vector<char> &out,
                                            size_t *nbytes_read = nullptr) = 0;
};


/// @}

/// @}
