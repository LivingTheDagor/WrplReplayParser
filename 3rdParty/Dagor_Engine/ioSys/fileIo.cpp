// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_fileIo.h>
#include "generic/dag_span.h"
#include <osApiWrappers/dag_direct_simple.h>

#include "util/dag_globDef.h"


LFileGeneralLoadCB::LFileGeneralLoadCB(File *handle) : fileHandle(handle) {
}

void LFileGeneralLoadCB::read(void *ptr, int size) {
    if (!fileHandle)
        EXCEPTION("file not open");
    if (fileHandle->df_read(ptr, size) != size)
        EXCEPTION("read error: {}", tell());
}

int LFileGeneralLoadCB::tryRead(void *ptr, int size) {
    if (!fileHandle)
        return 0;
    return (int) fileHandle->df_read(ptr, size);
}

int LFileGeneralLoadCB::tell() {
    if (!fileHandle)
        EXCEPTION("file not open");
    int64_t o = fileHandle->tell();
    if (o == -1)
        EXCEPTION("tell returns error");
    return (int) o;
}

void LFileGeneralLoadCB::seekto(int o) {
    if (!fileHandle)
        EXCEPTION("file not open");
    if (fileHandle->df_seek_to(o) == -1)
        EXCEPTION("seek error: {}", tell());
}

bool LFileGeneralLoadCB::seekrel(int o) {
    if (!fileHandle)
        EXCEPTION("file not open");
    if (fileHandle->df_seek_rel(o) == -1)
        return false;
    return true;
}

const VROMFs *LFileGeneralLoadCB::getTargetVromFs() const {
    return fileHandle ? fileHandle->getUnderlyingVromfs() : nullptr;
}

bool FullFileLoadCB::open(const std::string_view &fname, bool lower_fname) {
  close();
    targetDataSz = -1;
    if (fname.empty())
        return false;
    auto handle = file_mgr.getFile(fname, lower_fname);
    fileHandle = handle.get();
    handle.release();
    // we can't store a unique_ptr in LFileGeneralLoadCB as that shouldn't own, but FullFileLoadCB does
    // we could just make this store the unique ptr and then store the raw ptr in General, but we don't need to and can just take control of the ptr instead
    if (!fileHandle)
        return false;
    targetDataSz = fileHandle->length();
    return true;
}

void FullFileLoadCB::close() {
    del_it(fileHandle);
}

void FullFileLoadCB::beginFullFileBlock() {
    G_VERIFY(blocks.size() == 0);

    int i = append_items(blocks, 1);
    blocks[i].ofs = 0;
    blocks[i].len = (int) fileHandle->length();
}

//#define EXPORT_PULL dll_pull_iosys_fileIo
//#include <supp/exportPull.h>
