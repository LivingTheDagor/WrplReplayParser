// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_memIo.h>
#include <util/dag_globDef.h>

DynamicMemGeneralSaveCB::DynamicMemGeneralSaveCB(size_t sz, size_t quant) :
  dataptr(NULL), datasize(0), data_avail(0), data_quant(quant), curptr(0) {
  if (sz && data_quant)
    resize(sz);
}

DynamicMemGeneralSaveCB::~DynamicMemGeneralSaveCB(void) {
  if (dataptr)
    free(dataptr);
  dataptr = NULL;
  datasize = 0;
  curptr = 0;
}

void DynamicMemGeneralSaveCB::write(const void *ptr, int size) {
  if (!size || !ptr)
    return;

  if (curptr + size > data_avail)
    resize(curptr + size);
  memcpy(dataptr + curptr, ptr, size);
  curptr += size;
  if (curptr > datasize)
    datasize = curptr;
}

int DynamicMemGeneralSaveCB::tell(void) {
  if (curptr < 0 || curptr > datasize)
    EXCEPTION("invalid curptr");
  return curptr;
}

void DynamicMemGeneralSaveCB::seekto(int ofs) {
  if (ofs < 0 || ofs > datasize)
    EXCEPTION("seek pos out of range {}", tell());
  curptr = ofs;
}

void DynamicMemGeneralSaveCB::seektoend(int ofs) {
  if (datasize + ofs < 0 || ofs > 0)
    EXCEPTION("seek pos out of range {}", tell());
  curptr = datasize + ofs;
}

void DynamicMemGeneralSaveCB::resize(intptr_t sz) {
  if (data_quant > 0)
    sz = (sz + data_quant - 1) / data_quant * data_quant;


  if (!sz) {
    if (dataptr) {
      free(dataptr);
      dataptr = NULL;
    }
    curptr = 0;
    datasize = 0;
    data_avail = 0;
    return;
  }
  G_ASSERT(sz >= datasize);

  if (sz == data_avail)
    return;
  if (true)
    dataptr = (unsigned char *) realloc(dataptr, sz);
  data_avail = sz;
  if (datasize > data_avail)
    datasize = data_avail;
  if (curptr > datasize)
    curptr = datasize;
}

void DynamicMemGeneralSaveCB::setsize(intptr_t sz) {
  if (sz < 0)
    EXCEPTION("size out of range {}", sz);

  if (sz > data_avail)
    resize(sz);
  G_ASSERT(sz <= data_avail);

  if (curptr > sz)
    curptr = sz;
  datasize = sz;
}

unsigned char *DynamicMemGeneralSaveCB::copy(void) {
  if (!size())
    return NULL;

  unsigned char *buf = (unsigned char *) malloc(datasize);
  memcpy(buf, dataptr, datasize);
  return buf;
}
unsigned char *DynamicMemGeneralSaveCB::acquire() {
  auto ptr = dataptr;
  dataptr = nullptr;
  return ptr;
}

MemGeneralLoadCB::MemGeneralLoadCB(const void *ptr, int sz) : dataptr(NULL), datasize(0), curptr(0) {
  if (sz) {
    resize(sz);
    memcpy((void *) dataptr, ptr, sz);
  }
}

MemGeneralLoadCB::~MemGeneralLoadCB(void) {
  if (dataptr) {
    free((void *) dataptr);
    dataptr = NULL;
  }
  datasize = 0;
  curptr = 0;
}

void MemGeneralLoadCB::read(void *ptr, int size) {
  if (!size || !ptr)
    return;
  if (curptr + size > datasize)
    EXCEPTION("read error: {}", tell());
  memcpy(ptr, dataptr + curptr, size);
  curptr += size;
}

int MemGeneralLoadCB::tryRead(void *ptr, int size) {
  if (!size || !ptr)
    return 0;
  if (curptr + size > datasize)
    size = datasize - curptr;
  memcpy(ptr, dataptr + curptr, size);
  curptr += size;
  return size;
}

int MemGeneralLoadCB::tell(void) {
  if (curptr < 0 || curptr > datasize)
    EXCEPTION("invalid curptr");
  return curptr;
}

void MemGeneralLoadCB::seekto(int ofs) {
  if (ofs < 0 || ofs > datasize)
    EXCEPTION("seek ofs out of range {}", tell());
  curptr = ofs;
}

bool MemGeneralLoadCB::seekrel(int ofs) {
  if (curptr + ofs < 0 || curptr + ofs > datasize)
    return false;
  curptr += ofs;
  return true;
}

void MemGeneralLoadCB::close(void) { G_ASSERT((dataptr && datasize) || (!dataptr && !datasize)); }

void MemGeneralLoadCB::clear(void) {
  G_ASSERT((dataptr && datasize) || (!dataptr && !datasize));
  if (dataptr && datasize) {
    free((void *) dataptr);
    dataptr = NULL;
    datasize = 0;
  }
  curptr = 0;
}

void MemGeneralLoadCB::resize(int sz) {
  G_ASSERT((dataptr && datasize) || (!dataptr && !datasize));

  if (!sz) {
    clear();
    return;
  }

  unsigned char *data = (unsigned char *) malloc(sz);

  if (dataptr) {
    if (datasize < sz) {
      memcpy(data, dataptr, datasize);
    } else {
      memcpy(data, dataptr, sz);
      if (curptr > sz)
        curptr = sz;
    }
    free((void *) dataptr);
    dataptr = NULL;
  }

  dataptr = data;
  datasize = sz;
}

int MemGeneralLoadCB::size(void) { return datasize; }

const unsigned char *MemGeneralLoadCB::data(void) { return dataptr; }

unsigned char *MemGeneralLoadCB::copy(void) {
  if (!size())
    return NULL;
  unsigned char *data = (unsigned char *) malloc(datasize);
  memcpy(data, dataptr, datasize);
  return data;
}
