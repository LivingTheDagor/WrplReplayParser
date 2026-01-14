
#include "reader.h"
#include "zlib.h"
#include "dag_assert.h"

void ZlibLoadCB::open(GenReader &in_crd, int in_size, bool raw_inflate) {
  assert(!loadCb && "already opened?");
  static_assert(SIZE_OF_Z_STREAM >= sizeof(z_stream));

  assert(in_size > 0);
  loadCb = &in_crd;
  inBufLeft = in_size;
  isStarted = false;
  isFinished = false;
  rawInflate = raw_inflate;
  ((z_stream *) &strm)->zalloc = Z_NULL;
  ((z_stream *) &strm)->zfree = Z_NULL;
  //((z_stream *)&strm)->opaque = tmpmem; //Have no FUCKING idea if we need this, fuck youg gaijin
  ((z_stream *) &strm)->next_in = Z_NULL;
  ((z_stream *) &strm)->avail_in = 0;
}

void ZlibLoadCB::close() {
  assert(isFinished || !isStarted);
  ceaseReading();
  loadCb = NULL;
  inBufLeft = 0;
  isStarted = false;
  isFinished = false;
}


unsigned ZlibLoadCB::fetchInput(void *handle, void *strm) {
  ZlibLoadCB &zcrd = *(ZlibLoadCB *) handle;
  int sz = zcrd.inBufLeft;
  G_ASSERT(sz >= 0);

  if (sz > ZLIB_LOAD_BUFFER_SIZE)
    sz = ZLIB_LOAD_BUFFER_SIZE;

  if (sz <= 0)
    return 0;
  sz = zcrd.loadCb->tryRead(zcrd.buffer, sz);
  G_ASSERT(!zcrd.fatalErrors || sz > 0);
  ((z_stream *) strm)->next_in = zcrd.buffer;
  ((z_stream *) strm)->avail_in = sz;
  zcrd.inBufLeft -= sz;
  return sz;
}

inline int ZlibLoadCB::tryReadImpl(void *ptr, int size) {
  if (!size || isFinished)
    return 0;

  if (!isStarted) {
    int err = rawInflate ? inflateInit2((z_stream *) &strm, -MAX_WBITS) : inflateInit((z_stream *) &strm);
    if (err != Z_OK) {
      if (fatalErrors) {
        EXCEPTION("zlib error {} in {}; source: '{}'", err, "inflateInit", getTargetName());
      }
      return -1;
    }
    isStarted = true;
  }

  ((z_stream *) &strm)->avail_out = size;
  ((z_stream *) &strm)->next_out = (Bytef *) ptr;

  int res = inflateEx((z_stream *) &strm, Z_SYNC_FLUSH, (in_fetch_func) fetchInput, this);

  if (res != Z_OK && res != Z_STREAM_END && res != Z_BUF_ERROR) {
    if (fatalErrors) {

      EXCEPTION("zlib error {} ({}) in {}; source: '{}'", res, ((z_stream *) &strm)->msg, "inflate", getTargetName());
    }
    return -1;
  }
  size -= ((z_stream *) &strm)->avail_out;

  if (res == Z_STREAM_END && !ceaseReading())
    return -1;

  return size;
}

int ZlibLoadCB::tryRead(void *ptr, int size) {
  int rd_sz = ZlibLoadCB::tryReadImpl(ptr, size);
  int total_read_sz = rd_sz;
  while (rd_sz > 0 && rd_sz < size) {
    ptr = (char *) ptr + rd_sz;
    size -= rd_sz;
    rd_sz = ZlibLoadCB::tryReadImpl(ptr, size);
    if (rd_sz > 0)
      total_read_sz += rd_sz;
  }
  return total_read_sz;
}

bool ZlibLoadCB::read(void *ptr, unsigned size) {
  int rd_sz = tryRead(ptr, size);
  if (rd_sz != size) {
    isFinished = true;
    EXCEPTION("ZLIB read error");
  }
  return true;
}

bool ZlibLoadCB::seekrel(int ofs) {
  if (ofs < 0) {
    issueFatal();
    return false;
  } else
    while (ofs > 0) {
      char buf[4096];
      int sz = ofs > sizeof(buf) ? sizeof(buf) : ofs;
      read(buf, sz);
      ofs -= sz;
    }
  return true;
}

bool ZlibLoadCB::ceaseReading() {
  if (isFinished || !isStarted)
    return true;

  loadCb->seekrel(inBufLeft > 0x70000000 ? -int(((z_stream *) &strm)->avail_in) : inBufLeft);

  int err = inflateEnd((z_stream *) &strm);

  bool ret = err == Z_OK;
  if (!ret && fatalErrors) {
    EXCEPTION("zlib error {} in {}; source: '{}'", err, "inflateEnd", getTargetName());
  }

  isFinished = true;

  return ret;
}

void ZlibLoadCB::issueFatal() { assert(0 && "restricted by design"); }