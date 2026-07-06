// Copyright (C) Gaijin Games KFT.  All rights reserved.

//#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_zlibIo.h>
//#include <osApiWrappers/dag_fileIoErr.h>
#include "zlibInline.h"


static z_stream *get_zstream(unsigned char (&buf)[SIZE_OF_Z_STREAM]) {
    G_STATIC_ASSERT(SIZE_OF_Z_STREAM >= sizeof(z_stream));
    return std::launder(reinterpret_cast<z_stream *>(buf));
}

//-----------------------------------------------------------------------
// ZlibLoadCB
//-----------------------------------------------------------------------
void ZlibLoadCB::open(IGenLoad &in_crd, int in_size, bool raw_inflate) {
    G_ASSERT(!loadCb && "already opened?");

    G_ASSERT(in_size > 0);
    loadCb = &in_crd;
    inBufLeft = in_size;
    isStarted = false;
    isFinished = false;
    rawInflate = raw_inflate;
    z_stream *zs = get_zstream(strm);
    zs->zalloc = Z_NULL;
    zs->zfree = Z_NULL;
    zs->next_in = Z_NULL;
    zs->avail_in = 0;
}

void ZlibLoadCB::close() {
    G_ASSERT(isFinished || !isStarted);
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
    z_stream *zs = reinterpret_cast<z_stream *>(strm);
    zs->next_in = zcrd.buffer;
    zs->avail_in = sz;
    zcrd.inBufLeft -= sz;
    return sz;
}

inline int ZlibLoadCB::tryReadImpl(void *ptr, int size) {
    if (!size || isFinished)
        return 0;

    z_stream *zs = get_zstream(strm);
    if (!isStarted) {
        int err = rawInflate ? inflateInit2(zs, -MAX_WBITS) : inflateInit(zs);
        if (err != Z_OK) {
            if (fatalErrors) {
                EXCEPTION("zlib error {} in {}\nsource: '{}'\n", err, "inflateInit", getTargetName());
            }
            return -1;
        }
        isStarted = true;
    }

    zs->avail_out = size;
    zs->next_out = (Bytef *) ptr;

    int res = inflateEx(zs, Z_SYNC_FLUSH, (in_fetch_func) fetchInput, this);

    if (res != Z_OK && res != Z_STREAM_END && res != Z_BUF_ERROR) {
        if (fatalErrors) {
            EXCEPTION("zlib error {} ({}) in {}\nsource: '{}'\n", res, zs->msg, "inflate", getTargetName());
        }
        return -1;
    }
    size -= zs->avail_out;

    if (res == Z_STREAM_END && !ceaseReading())
        return -1;

    return size;
}

int ZlibLoadCB::tryRead(void *ptr, int size) {
  ZoneScopedN("ZlibLoadCB::tryRead");
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

void ZlibLoadCB::read(void *ptr, int size) {
    int rd_sz = tryRead(ptr, size);
    if (rd_sz != size) {
        isFinished = true;
        EXCEPTION("Zlib read error");
    }
}

bool ZlibLoadCB::seekrel(int ofs) {
    if (ofs < 0)
        return false;
    else
        while (ofs > 0) {
            char buf[4096];
            int sz = ofs > sizeof(buf) ? sizeof(buf) : ofs;
            if (tryRead(buf, sz) != sz) {
                isFinished = true;
                return false;
            }
            ofs -= sz;
        }
    return true;
}

bool ZlibLoadCB::ceaseReading() {
    if (isFinished || !isStarted)
        return true;

    z_stream *zs = get_zstream(strm);
    loadCb->seekrel(inBufLeft > 0x70000000 ? -int(zs->avail_in) : inBufLeft);

    int err = inflateEnd(zs);

    bool ret = err == Z_OK;
    if (!ret && fatalErrors) {
        EXCEPTION("zlib error {} in {}\nsource: '{}'", err, "inflateEnd", getTargetName());
    }

    isFinished = true;

    return ret;
}

void BufferedZlibLoadCB::read(void *ptr, int size) {
    int rd = tryRead(ptr, size);
    if (rd != size) {
        EXCEPTION("error in BufferedZlibLoadCB::read({}, {})={} totalOut={} curPos={}", ptr, size, rd, totalOut,
                  curPos);
    }
}

int BufferedZlibLoadCB::tryRead(void *_ptr, int size) {
    char *ptr = (char *) _ptr;
    if (curPos + size <= totalOut) {
        memcpy(ptr, outBuf + curPos, size);
        curPos += size;
        return size;
    }

    if (totalOut - curPos && totalOut - curPos < size) {
        memcpy(ptr, outBuf + curPos, totalOut - curPos);
        ptr += totalOut - curPos;
        size -= totalOut - curPos;
    }

    if (size > OUT_BUF_SZ / 2) {
        while (size > 0) {
            totalOut = ZlibLoadCB::tryRead(outBuf, size >= OUT_BUF_SZ ? OUT_BUF_SZ : size);
            memcpy(ptr, outBuf, totalOut);
            ptr += totalOut;
            size -= totalOut;
            if (!totalOut)
                break;
        }

        totalOut = 0;
        curPos = 0;
    } else {
        totalOut = ZlibLoadCB::tryReadImpl(outBuf, OUT_BUF_SZ);
        G_ASSERT(totalOut >= size);
        memcpy(ptr, outBuf, size);
        ptr += size;
        curPos = size;
    }
    return ptr - (char *) _ptr;
}


//-----------------------------------------------------------------------
// ZlibSaveCB
//-----------------------------------------------------------------------

ZlibSaveCB::ZlibSaveCB(IGenSave &in_save_cb, int compression_level, bool raw_inflate) : saveCb(&in_save_cb) {
    zlibWriter = new ZLibGeneralWriter(in_save_cb, 64 << 10, compression_level, raw_inflate);
    isFinished = false;
}


ZlibSaveCB::~ZlibSaveCB() {
    G_ASSERT(isFinished);
    delete zlibWriter;
}


void ZlibSaveCB::write(const void *ptr, int size) {
    G_ASSERT(!isFinished);
    zlibWriter->pack((void *) ptr, size, false);
}


void ZlibSaveCB::syncFlush() {
    G_ASSERT(!isFinished);
    zlibWriter->packZflush(NULL, 0, Z_SYNC_FLUSH);
}

void ZlibSaveCB::finish() {
    zlibWriter->pack(NULL, 0, true);
    isFinished = true;
}

float ZlibSaveCB::getCompressionRatio() { return zlibWriter->getCompressionRatio(); }


//-----------------------------------------------------------------------
// ZlibDecompressSaveCB
//-----------------------------------------------------------------------
ZlibDecompressSaveCB::ZlibDecompressSaveCB(IGenSave &in_save_cb, bool raw_inflate,
                                           bool fatal_errors) : saveCb(in_save_cb), fatalErrors(fatal_errors) {
    z_stream *zs = get_zstream(strm);
    zs->zalloc = Z_NULL;
    zs->zfree = Z_NULL;
    zs->next_in = Z_NULL;
    zs->avail_in = 0;
    int err = raw_inflate ? inflateInit2(zs, -MAX_WBITS) : inflateInit(zs);
    if (err != Z_OK) {
        if (fatalErrors) {
            EXCEPTION("zlib error {} in {}\nsource: '{}'\n", err, "inflateInit", getTargetName());
        }
        isBroken = true;
    }
}


ZlibDecompressSaveCB::~ZlibDecompressSaveCB() {
    z_stream *zs = get_zstream(strm);
    int err = inflateEnd(zs);
    if (err != Z_OK && fatalErrors) {
        EXCEPTION("zlib error {} in {}\nsource: '{}'\n", err, "inflateEnd", getTargetName());
    }
}


bool ZlibDecompressSaveCB::doProcessStep() {
    if (isBroken || isFinished)
        return false;
    z_stream *zs = get_zstream(strm);
    zs->next_out = buffer;
    zs->avail_out = ZLIB_LOAD_BUFFER_SIZE;
    int wasAvailIn = zs->avail_in;
    int res = inflate(zs, Z_SYNC_FLUSH);
    if (res != Z_OK && res != Z_STREAM_END) {
        isBroken = true;
        if (fatalErrors) {
            EXCEPTION("zlib error {} in {}\nsource: '{}'\n", res, "inflate", getTargetName());
        }
        return false;
    }

    int processedBytes = wasAvailIn - zs->avail_in;
    int outputBytes = ZLIB_LOAD_BUFFER_SIZE - zs->avail_out;
    if (outputBytes > 0)
        saveCb.write(buffer, outputBytes);

    if (res == Z_STREAM_END) {
        isFinished = true;
        return false;
    }

    return processedBytes > 0 || outputBytes > 0;
}


void ZlibDecompressSaveCB::write(const void *ptr, int size) {
    z_stream *zs = get_zstream(strm);
    zs->next_in = (unsigned char *) ptr;
    zs->avail_in = size;
    while (zs->avail_in > 0) {
        if (!doProcessStep())
            break;
    }
}


void ZlibDecompressSaveCB::syncFlush() {
    bool haveMore = true;
    while (haveMore)
        haveMore = doProcessStep();
}


void ZlibDecompressSaveCB::finish() {
    flush();

    if (!isFinished && !isBroken && fatalErrors) {
        int err = Z_DATA_ERROR;
        EXCEPTION("zlib error {} in {}\nsource: '{}'\n", err, "finish", getTargetName());
    }
}


int zlib_compress_data(IGenSave &dest, int compression_level, IGenLoad &src, int sz) {
    static constexpr int BUF_SZ = 64 << 10;
    ZLibGeneralWriter zlibWriter(dest, BUF_SZ, compression_level, false);
    char *buf = new char[BUF_SZ];

    while (sz > 0) {
        int b_sz = (sz > BUF_SZ) ? BUF_SZ : sz;
        b_sz = src.tryRead(buf, b_sz);
        sz -= b_sz;
        zlibWriter.pack(buf, b_sz, sz <= 0);
    }
    delete[] buf;

    return zlibWriter.compressedTotal;
}

