
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

namespace fs = std::filesystem;

class Directory;

class File;

class FSObject;


class SmartFSHandle {
public:
  SmartFSHandle() = default;

  explicit SmartFSHandle(std::shared_ptr<FSObject> ptr)
      : ptr(std::move(ptr)) {}

  SmartFSHandle(std::nullptr_t)
      : ptr(nullptr) {}

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

  std::shared_ptr<File> asFile();

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
  std::string getName() {
    return name.filename().string();
  }

  FsObjectTypes getFSObjectType() {
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

  virtual SmartFSHandle operator[](const std::string &lookup_name) {
    throw std::runtime_error("Attempted to index an FSObject that is not a directory");
  }

  static std::shared_ptr<File> FSObjToFile(const std::shared_ptr<FSObject> &obj) {
    if (obj->getFSObjectType() == isFile) {
      return std::dynamic_pointer_cast<File>(obj);
    }
    return nullptr;
  }

  File *asFile();

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
  friend File;
  friend Directory;
};

class DataBlock;

class File : public FSObject {
public:

  void init(const fs::path &path) {
    name = path;
    obj_type = isFile;
  }

  void init(const std::string &name) {
    init(fs::path(name));
  }

  std::string getExtension() {
    return name.extension().string();
  }


  fs::path getFullFilePath() {
    return name;
  }



  /// reads data as stored in the file, no processing
  virtual std::span<char> readRaw() = 0;

  virtual void Save(std::ofstream *cb) = 0;

  virtual bool loadBlk(DataBlock &blk) = 0;

  // TODO: turn readRaw into returning a std::shared_ptr<std::span<char>>
};


class HostFile : public File {
public:
  explicit HostFile(const std::string &name) {
    init(name);
  }

  explicit HostFile(const fs::path &name) {
    init(name);
  }

  /// A implementation for reading a file from the OS, will cache data on first read
  /// will only reread if the file data has changed
  /// after calling this, assume old span
  /// \return a std::span<char> pointing to the container for the file data
  std::span<char> readRaw() override;

  void Save(std::ofstream *cb) override;

  bool loadBlk(DataBlock &blk) override;

private:
  std::vector<char> buffer;
  fs::file_time_type last_write_time;

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
        throw std::runtime_error("tried to advance past the end of a path");
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
  std::shared_ptr<FSObject> getFSObject(const std::string &name) {
    std::string to_use;
    if (name.c_str()[0] == '%')
    {
      to_use = std::string(name.c_str()+1);
    }
    else
    {
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
  std::shared_ptr<File> getFile(const std::string &name) {
    auto it = objects.find(name);
    if (it == objects.end() || !it->second)
      return nullptr;

    const std::shared_ptr<FSObject> &obj = it->second;
    if (obj->obj_type == isFile) {
      return std::dynamic_pointer_cast<File>(obj);
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

  bool addFile(const std::shared_ptr<File> &file) {
    return addFSObject(file);
  }

  bool addFile(const std::shared_ptr<File> &file, const fs::path &path) {
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


  void dumpToPath(const fs::path &path) {
    // Ensure the root directory exists (create it if needed)
    if (!fs::exists(path)) {
      fs::create_directories(path);
    }

    for (const auto &objPair: objects) {
      const auto &name = objPair.first;
      const auto &obj = objPair.second;

      // Build the full path for this object
      fs::path targetPath = path / name;

      if (obj->obj_type == isDirectory) {
        // Recursively dump subdirectories
        auto dir = std::dynamic_pointer_cast<Directory>(obj);
        if (dir) {
          dir->dumpToPath(targetPath);
        }
      } else if (obj->obj_type == isFile) {
        auto file = std::dynamic_pointer_cast<File>(obj);
        if (file) {
          // Create the parent directories (if any)
          if (!fs::exists(targetPath.parent_path())) {
            fs::create_directories(targetPath.parent_path());
          }

          // Dump the file data (assume File has read/write methods)

          std::ofstream out(targetPath, std::ios::binary);
          try {
            if (!out) {
              throw std::runtime_error("Failed to write file: " + targetPath.string());
            }

            // Assuming File has a `readRaw()` method or equivalent
            std::cout << "Parsing file: " << file->getName() << "\n";
            file->Save(&out);
            //std::cout << "Saved file: " << file->getName() << "\n";
          }
          catch (const std::exception &e) {
            std::cerr << "Exception saving file " << targetPath << ": " << e.what() << "\n";
          }

        }
      }
    }
  }

  void callSaveForAllFiles() {
    for (const auto& entry: this->objects) {
      auto obj = entry.second;
      if (!obj) {
        continue;
      }
      if (obj->obj_type == isFile) {
        auto file = obj->asFile();
        std::ofstream *os; //
        std::cout << "parsing file: " << file->getName() << "\n";
        file->Save(os);
      }
      if (obj->obj_type == isDirectory) {
        auto dir = obj->asDirectory();
        dir->callSaveForAllFiles();
      }
    }
  }

  void getFilesInDirectory(std::vector<File *> &files)
  {
    for (const auto& entry: this->objects) {
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

  void dumpToPathThreaded(const fs::path &path, ThreadPool &pool) {
    // Ensure the root directory exists (create it if needed)
    if (!fs::exists(path)) {
      fs::create_directories(path);
    }

    for (const auto &objPair: objects) {
      const auto &name = objPair.first;
      const auto &obj = objPair.second;

      // Build the full path for this object
      fs::path targetPath = path / name;

      if (obj->obj_type == isDirectory) {
        // Recursively dump subdirectories
        auto dir = std::dynamic_pointer_cast<Directory>(obj);
        if (dir) {
          dir->dumpToPathThreaded(targetPath, pool);
        }
      } else if (obj->obj_type == isFile) {
        auto file = std::dynamic_pointer_cast<File>(obj);
        if (file) {
          // Create the parent directories (if any)
          if (!fs::exists(targetPath.parent_path())) {
            fs::create_directories(targetPath.parent_path());
          }

          // Dump the file data (assume File has read/write methods)
          pool.enqueue([file, targetPath]() {
            std::ofstream out(targetPath, std::ios::binary);
            try {
              if (!out) {
                throw std::runtime_error("Failed to write file: " + targetPath.string());
              }

              // Assuming File has a `readRaw()` method or equivalent
              std::cout << "Parsing file: " << file->getName() << "\n";
              file->Save(&out);
              //std::cout << "Saved file: " << file->getName() << "\n";
            }
            catch (const std::exception &e) {
              std::cerr << "Exception saving file " << targetPath << ": " << e.what() << "\n";
            }
          });
        }
      }
    }
  }

  SmartFSHandle operator[](const std::string &lookup_name) override {
    auto x = getFSObject(lookup_name);
    if (x == nullptr) {
      EXCEPTION("indexing of directory '{}' for FSObject '{}' returned null", this->name.string().c_str(), lookup_name.c_str());
    }
    return SmartFSHandle(x);
  }

  void loadFromOsPath(const fs::path &os_path) {
    assert(fs::exists(os_path) && fs::is_directory(os_path));

    for (const auto &entry: fs::recursive_directory_iterator(os_path)) {
      fs::path relative = fs::relative(entry.path(), os_path);

      if (entry.is_directory()) {
        auto dir = std::make_shared<Directory>(entry.path().filename().string());
        this->addDirectory(dir, relative);
      } else if (entry.is_regular_file()) {
        auto file = std::make_shared<HostFile>(entry.path().string());
        this->addFile(file, relative);
        //std::cout << "Adding object: " << file->getName()
        //          << ", type: " << file->getFSObjectType()
        //          << ", real type: " << typeid(file.get()).name() << "\n";
      }
      // Optionally: skip symlinks or other non-regular files
    }
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

  std::unordered_map<std::string, std::shared_ptr<FSObject>> objects;
};

class VROMFs;

struct FileManager
{
  bool loadVromfs(std::string &vromfsPath);

  bool loadVromfs(fs::path &vromfsPath) { auto str = vromfsPath.string();return loadVromfs(str);}

  std::shared_ptr<File> loadRealFsFile(const fs::path &path);

  std::shared_ptr<File> loadVromfsFile(const fs::path &path);

  std::shared_ptr<File> getFile(const fs::path& path, bool lower=false, bool prioritizeVromfs = true);

  void find_vromfs_files_in_folder(std::vector<fs::path> &ut_list, const std::string &dir_path);

  int find_files_in_folder(std::vector<std::string> &out_list, std::string &dir_path, const char *file_suffix_to_match = "",
                                   bool vromfs = true, bool realfs = true, bool subdirs = false);
  inline void add_mount(fs::path path)
  {
    this->real_fs_mounts.push_back(path);
  }
  inline std::shared_ptr<Directory> getDir()
  {
    return holder_dir;
  }

private:
  SmartFSHandle getObject(const fs::path& path);

  std::vector<fs::path> real_fs_mounts;
  std::shared_ptr<Directory> holder_dir;

  std::vector<std::shared_ptr<VROMFs>> loaded_vromfs;
};

extern FileManager file_mgr;


#endif //MYEXTENSION_FILESYSTEM_H