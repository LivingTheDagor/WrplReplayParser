
#ifndef MYEXTENSION_FILESYSTEM_H
#define MYEXTENSION_FILESYSTEM_H

#include <unordered_map>
#include <cassert>
#include <variant>
#include "span"
#include <iostream>
#include <fstream>
#include <vector>
#include "ThreadPool.h"
#include "utils.h"
#include <filesystem>
#include "dag_assert.h"
#include "FileSystem.h"

namespace fs = std::filesystem;

class Directory;

class FileIndex;

class FSObject;

class VROMFs;


class SmartFSHandle {
public:
  SmartFSHandle() = default;

  explicit SmartFSHandle(std::shared_ptr<FSObject> ptr)
    : ptr(std::move(ptr)) {
  }

  SmartFSHandle(std::nullptr_t)
    : ptr(nullptr) {
  }

  FSObject *operator->() const {
    return ptr.get();
  }

  FSObject &operator*() const {
    return *ptr;
  }

  explicit operator bool() const {
    return static_cast<bool>(ptr);
  }

  SmartFSHandle operator[](const std::string &name) const;

  [[nodiscard]] std::shared_ptr<FSObject> getShared() const {
    return ptr;
  }

  std::shared_ptr<FileIndex> asFile();

  std::shared_ptr<Directory> asDirectory();

private:
  std::shared_ptr<FSObject> ptr;
};

enum FsObjectTypes {
  isNone,
  isFile,
  isDirectory,
};


class FSObject {
public:

  /// returns the name of this FSObject, will include extension if it exists
  /// \return
  std::string getName() const {
    return name.filename().string();
  }

  const fs::path &getPath() const {
    return name;
  }


  FsObjectTypes getFSObjectType() const {
    return obj_type;
  }


  void unBind();

  virtual ~FSObject() = default;

  virtual void print(int indent) {
    for (int i = 0; i < indent; i++) {
      std::cout << " ";
    }
    std::cout << getName() << "\n";
  }

  virtual SmartFSHandle operator[](const std::string &lookup_name) const {
    throw std::runtime_error("Attempted to index an FSObject that is not a directory");
  }

  static std::shared_ptr<FileIndex> FSObjToFile(const std::shared_ptr<FSObject> &obj) {
    if (obj->getFSObjectType() == isFile) {
      return std::dynamic_pointer_cast<FileIndex>(obj);
    }
    return nullptr;
  }

  FileIndex *asFile();

  Directory *asDirectory();

  static std::shared_ptr<Directory> FSObjToDirectory(const std::shared_ptr<FSObject> &obj) {
    if (obj->getFSObjectType() == isDirectory) {
      return std::dynamic_pointer_cast<Directory>(obj);
    }
    return nullptr;
  }

protected:
  fs::path name;
  Directory *owner = nullptr;
  FsObjectTypes obj_type = isNone;
  friend FileIndex;
  friend Directory;
};

class DataBlock;
class File;
class HostFile;


class FileIndex : public FSObject {
public:
  ~FileIndex() override = default;

  FileIndex(const fs::path &path) {
    name = path;
    obj_type = isFile;
  }

  FileIndex(const std::string &name_) {
    name = fs::path(name_);
    obj_type = isFile;
  }

  virtual std::unique_ptr<File> getFile(std::shared_ptr<FileIndex> ths) = 0;

  friend File;
  friend HostFile;
};

class File {
protected:
  std::shared_ptr<FileIndex> index;
  int64_t read_offs{}; // used with tell(), read(), etc
  int64_t f_length{};

  // read implementation
  int64_t virtual read_impl(void *ptr, size_t length) = 0;

public:
  const FileIndex *getIndex() const { return index.get(); }

  virtual ~File() = default;

  explicit File(std::shared_ptr<FileIndex> index) : index(std::move(index)) {
    read_offs = -1;
    f_length = -1;
  }

  std::string getExtension() {
    return index->name.extension().string();
  }


  fs::path getFullFilePath() {
    return index->name;
  }

  int64_t tell() { return this->read_offs; }

  // all seek methods based on dagor engine definition
  int64_t df_seek_to(int64_t offs) {
    if (offs < 0 || offs >= f_length) {
      return -1;
    }
    this->read_offs = offs;
    return this->read_offs;
  }

  int64_t df_seek_rel(int64_t offs) {
    if (offs < 0 && read_offs + offs >= f_length) {
      return -1;
    }
    this->read_offs += offs;
    return this->read_offs;
  }

  int64_t df_seek_end(int64_t offs) {
    if (offs > 0 || f_length - offs < 0) {
      return -1;
    }
    this->read_offs = f_length - offs;
    return this->read_offs;
  }

  int64_t df_read(void *ptr, size_t len) {
    return read_impl(ptr, len);
  }


  /// reads data as stored in the file, no processing
  virtual std::span<char> readRaw() = 0;

  virtual void Save(std::ofstream *cb) = 0;

  virtual const VROMFs *getUnderlyingVromfs() = 0;

  virtual bool loadBlk(DataBlock &blk) = 0;

  int64_t length() const { return f_length; };
};

class HostFileIndex : public FileIndex {
  int64_t file_length;

  void init();

public:
  ~HostFileIndex() override = default;

  HostFileIndex(const std::string &name_) : FileIndex(name_) {
    init();
  }

  HostFileIndex(const fs::path &path) : FileIndex(path) {
    init();
  }

  std::unique_ptr<File> getFile(std::shared_ptr<FileIndex> ths) override;

  friend HostFile;
};

class HostFile : public File {
protected:
  int64_t read_impl(void *ptr, size_t length) override;

  void init_hf();

public:
  explicit HostFile(std::shared_ptr<FileIndex> index) : File(std::move(index)) {
    init_hf();
  }

  /// A implementation for reading a file from the OS, will cache data on first read
  /// will only reread if the file data has changed
  /// after calling this, assume old span
  /// \return a std::span<char> pointing to the container for the file data
  std::span<char> readRaw() override;

  void Save(std::ofstream *cb) override;

  bool loadBlk(DataBlock &blk) override;

  const VROMFs *getUnderlyingVromfs() override {
    return nullptr;
  }

private:
  std::ifstream file_stream;
  std::vector<char> buffer;
};

class DummyOStream : std::basic_ostream<char> {
public:
  DummyOStream() = default;
};

class Directory : public FSObject {
private:
  struct ObjAdder {
  public:
    explicit ObjAdder(const fs::path &input_path) {
      setupPath(input_path);
    }

    void setupPath(const fs::path &input_path) {
      // if the path has an extension, then the last part of the path is def a file, if it isnt, then we assume its a directory
      if (input_path.has_extension()) {
        path = input_path.relative_path().parent_path();
      } else {
        path = input_path.relative_path();
      }

      iter_pointer = path.begin();
      iter_end = path.end();
    }

    [[nodiscard]] std::string getCurrent() const {
      return iter_pointer->string();
    }

    [[nodiscard]] std::string GetCurrentAndAdvance() {
      auto x = iter_pointer->string();
      Advance();
      return x;
    }

    [[nodiscard]] bool isEnd() const {
      // this handles both normal case and if the path has no elements
      return iter_pointer == iter_end;
    }

    void Advance() {
      if (isEnd()) {
        EXCEPTION("tried to advance past the end of a path");
      }
      iter_pointer = std::next(iter_pointer);
    }

  private:
    fs::path path;
    fs::path::iterator iter_pointer;
    fs::path::iterator iter_end;
  };

public:
  explicit Directory(const std::string &name) {
    this->name = name;
    this->obj_type = isDirectory;
  }

  explicit Directory(fs::path &path) {
    this->name = path.filename();
    this->obj_type = isDirectory;
  }

  /// checks if a object with name exists in this directory
  /// \param name the name to look up
  /// \return return true if it found a matching object, false otherwise
  bool nameExists(const std::string &name) {
    auto it = objects.find(name);
    return it != objects.end();
  }

  /// Gets a generic FileSystem Object, indexing calls this
  /// \param name the name to look up in this directory
  /// \return a std::shared_ptr<FsObject> or a nullptr
  std::shared_ptr<FSObject> getFSObject(const std::string &name) const {
    std::string to_use;
    if (name.c_str()[0] == '%') {
      to_use = std::string(name.c_str() + 1);
    } else {
      to_use = name;
    }
    auto it = objects.find(to_use);
    if (it == objects.end() || !it->second)
      return nullptr;

    const std::shared_ptr<FSObject> &obj = it->second;
    if (obj->obj_type != isNone) {
      return obj;
    }
    return nullptr;
  }

  /// Gets a File
  /// \param name the name to look up in this directory
  /// \return a std::shared_ptr<File> or a nullptr
  std::shared_ptr<FileIndex> getFile(const std::string &name) {
    auto it = objects.find(name);
    if (it == objects.end() || !it->second)
      return nullptr;

    const std::shared_ptr<FSObject> &obj = it->second;
    if (obj->obj_type == isFile) {
      return std::dynamic_pointer_cast<FileIndex>(obj);
    }
    return nullptr;
  }

  /// Gets a Directory
  /// \param name the name to look up in this directory
  /// \return a std::shared_ptr<Directory> or a nullptr
  std::shared_ptr<Directory> getDirectory(const std::string &name) {
    auto it = objects.find(name);
    if (it == objects.end() || !it->second)
      return nullptr;

    const std::shared_ptr<FSObject> &obj = it->second;
    if (obj->obj_type == isDirectory) {
      return std::dynamic_pointer_cast<Directory>(obj);
    }
    return nullptr;
  }

  bool RemoveFsObject(const std::string &name) {
    return objects.erase(name) == 1;
  }

  bool addFSObject(const std::shared_ptr<FSObject> &obj) {
    std::string name = obj->getName();
    if (obj->owner) {
      return false;
    }
    if (!nameExists(name)) {
      this->objects[name] = obj;
      obj->owner = this;
      return true;
    }
    return false;
  }

  bool addFile(const std::shared_ptr<FileIndex> &file) {
    return addFSObject(file);
  }

  bool addFile(const std::shared_ptr<FileIndex> &file, const fs::path &path) {
    auto objAdder = ObjAdder(path);
    return addFSObject(file, objAdder);
  }

  bool addDirectory(const std::shared_ptr<Directory> &directory) {
    return addFSObject(directory);
  }

  bool addDirectory(const std::shared_ptr<Directory> &directory, const fs::path &path) {
    auto objAdder = ObjAdder(path);
    return addFSObject(directory, objAdder);
  }


  std::shared_ptr<Directory> getCreateDirectory(const std::string &name) {
    auto obj = getFSObject(name);
    if (obj != nullptr) {
      if (obj->obj_type == isDirectory) {
        return std::dynamic_pointer_cast<Directory>(obj);
      }
      return nullptr;
    }
    auto x = std::make_shared<Directory>(name);
    x->owner = this;
    this->objects[name] = x;
    return x;
  }

  void print(int indent) override {
    for (int i = 0; i < indent; i++) {
      std::cout << " ";
    }
    std::cout << getName() << "\n";
    for (const auto &obj: objects) {
      if (obj.second->obj_type == isDirectory) {
        obj.second->print(indent + 4);
      }
    }
    for (const auto &obj: objects) {
      if (obj.second->obj_type == isFile) {
        obj.second->print(indent + 4);
      }
    }
  }

  void getFilesInDirectory(std::vector<FileIndex *> &files) {
    for (const auto &entry: this->objects) {
      auto obj = entry.second;
      if (!obj) {
        continue;
      }
      if (obj->obj_type == isFile) {
        auto file = obj->asFile();
        files.push_back(file);
      }
    }
  }

  SmartFSHandle operator[](const std::string &lookup_name) const override {
    auto x = getFSObject(lookup_name);
    if (x == nullptr) {
      //LOGD2("indexing of directory '{}' for FSObject '{}' returned null", this->name.string().c_str(), lookup_name.c_str());
      return nullptr;
    }
    return SmartFSHandle(x);
  }

protected:
  bool addFSObject(const std::shared_ptr<FSObject> &f, ObjAdder &path) {
    if (path.isEnd()) {
      return addFSObject(f);
    }
    auto nextDir = getCreateDirectory(path.getCurrent());
    if (nextDir == nullptr) {
      return false;
    }
    path.Advance();
    return nextDir->addFSObject(f, path);
  }

  std::unordered_map<std::string, std::shared_ptr<FSObject> > objects;
};

class VROMFs;

struct FileManager {
  bool mountVromfs(std::string &vromfsPath);

  bool mountVromfs(fs::path &vromfsPath) {
    auto str = vromfsPath.string();
    return mountVromfs(str);
  }

  bool unmountVromfs(const std::string &vromfs_name);

  std::unique_ptr<File> loadRealFsFile(const fs::path &path);

  std::unique_ptr<File> loadVromfsFile(const fs::path &path);

  std::unique_ptr<File> getFile(const fs::path &path, bool lower = false, bool prioritizeVromfs = true);

  void find_vromfs_files_in_folder(std::vector<fs::path> &ut_list, const std::string &dir_path);

  int find_files_in_folder(std::vector<std::string> &out_list, std::string &dir_path,
                           const char *file_suffix_to_match = "",
                           bool vromfs = true, bool realfs = true, bool subdirs = false);

  inline void add_mount(const fs::path &path) {
    this->real_fs_mounts.push_back(path);
  }


  ~FileManager();

private:
  SmartFSHandle getObject(const fs::path &path);

  std::vector<fs::path> real_fs_mounts;
  std::unordered_map<fs::path, Directory> mounted_vromfs{};
  std::vector<VROMFs *> loaded_vromfs;
};

extern FileManager file_mgr;


#endif //MYEXTENSION_FILESYSTEM_H
