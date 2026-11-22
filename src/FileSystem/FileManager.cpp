#include "FileSystem.h"
#include "VROMFs.h"
#include <algorithm>
#include <cctype>


bool FileManager::mountVromfs(const std::string &vromfsPath) {
  ZoneScoped;
  auto file = this->getFile(vromfsPath);
  if (!file)
    return false;
  std::unique_lock<std::mutex> lock(this->init_mutex, std::defer_lock);
  if (this->in_init)
    lock.lock();
  this->loaded_vromfs.emplace_back(new VROMFs(vromfsPath));
  return true;
}

std::unique_ptr<File> FileManager::getFile(const fs::path &path, bool lower, bool prioritizeVromfs) {
  ZoneScopedN("FileManager::getFile");
  fs::path to_use;
  if (lower) {
    std::string tmp = path.string();
    std::ranges::transform(tmp, tmp.begin(), [](char c) { return ::tolower((unsigned char) c); });
    to_use = fs::path(tmp);
  } else
    to_use = path;
  if (prioritizeVromfs) {
    // archives mounted now first; a deferred archive (deferVromfs) only after the disk, so that looking up a
    // file on disk does not mount it
    auto out = this->loadVromfsFile(to_use, false);
    if (out)
      return out;
  }
  auto out = this->loadRealFsFile(to_use);
  if (out)
    return out;
  return this->loadVromfsFile(to_use, true);
}

void FileManager::find_vromfs_files_in_folder(std::vector<fs::path> &out_list, const std::string &dir_path) {
  SmartFSHandle directory = getObject(fs::path(dir_path));
  if (!directory || directory->getFSObjectType() != isDirectory)
    return;
  auto d = directory.asDirectory();
  std::vector<FileIndex *> files;
  d->getFilesInDirectory(files);
  for (auto &f: files) {
    out_list.push_back(f->getPath());
  }
}

FileManager::~FileManager() { this->loaded_vromfs.clear(); }

SmartFSHandle FileManager::findIn(const std::vector<std::unique_ptr<VROMFs>> &list, const fs::path &path) {
  for (auto &vromfs: list) {
    SmartFSHandle curr_ptr;
    for (const auto &p: path) {
      if (!curr_ptr) {
        curr_ptr = (vromfs->getDirectory())[p.string()];
        if (!curr_ptr)
          break;
      } else {
        curr_ptr = curr_ptr[p.string()];
      }
    }
    if (curr_ptr)
      return curr_ptr;
  }
  return nullptr;
}

// will be used async
SmartFSHandle FileManager::getObject(const fs::path &path, bool allow_deferred) {
  std::unique_lock<std::mutex> lock(this->init_mutex, std::defer_lock);
  if (this->in_init)
    lock.lock();
  if (auto found = findIn(this->loaded_vromfs, path))
    return found;
  // the archive itself is opened through getFile, which looks here first: no second load from inside the load
  static thread_local bool loading_deferred = false;
  // an absolute path is a file on disk, never an archive entry
  if (!allow_deferred || this->in_init || this->deferred_paths.empty() || loading_deferred || path.is_absolute())
    return nullptr;
  // only paths under what the deferred archives hold: lookups of files that exist nowhere are common
  const std::string generic = path.generic_string();
  if (!std::ranges::any_of(this->deferred_prefixes, [&generic](const std::string &pre) {
        return generic.compare(0, pre.size(), pre) == 0 && (generic.size() == pre.size() || generic[pre.size()] == '/');
      }))
    return nullptr;
  // deferred_vromfs is written once, inside call_once; every caller returns from call_once after that write
  std::call_once(this->deferred_once, [this] {
    loading_deferred = true;
    for (const auto &p: this->deferred_paths)
      if (this->loadRealFsFile(p))
        this->deferred_vromfs.emplace_back(new VROMFs(p));
    loading_deferred = false;
  });
  return findIn(this->deferred_vromfs, path);
}

void FileManager::deferVromfs(const std::string &vromfsPath, std::initializer_list<const char *> dirs) {
  this->deferred_paths.push_back(vromfsPath);
  for (const char *d: dirs)
    this->deferred_prefixes.emplace_back(d);
}

void FileManager::addVromfs(std::unique_ptr<VROMFs> &&vromfs) {
  std::unique_lock<std::mutex> lock(this->init_mutex, std::defer_lock);
  if (this->in_init)
    lock.lock();
  this->loaded_vromfs.emplace_back(std::move(vromfs));
}

bool FileManager::unmountVromfs(const std::string &vromfs_name) {
  ZoneScoped;
  std::unique_lock<std::mutex> lock(this->init_mutex, std::defer_lock);
  if (this->in_init)
    lock.lock();
  for (auto it = this->loaded_vromfs.begin(); it != this->loaded_vromfs.end(); ++it) {
    if ((*it)->getName() == vromfs_name) {
      this->loaded_vromfs.erase(it);
      return true;
    }
  }
  return false;
}

std::unique_ptr<File> FileManager::loadRealFsFile(const fs::path &path) {
  ZoneScopedN("FileManager::loadRealFsFile");
  std::shared_ptr<HostFileIndex> index;
  if (path.is_absolute()) {
    if (fs::exists(path)) {
      index = std::make_shared<HostFileIndex>(path);
    }
  }
  if (!index)
    for (auto &mount: this->real_fs_mounts) {
      fs::path p = mount / path;
      if (fs::exists(p)) {
        index = std::make_shared<HostFileIndex>(p);
      }
    }
  // final check, check bin directory
  if (!index && fs::exists(path)) {
    index = std::make_shared<HostFileIndex>(path);
  }
  if (index) {
    return index->getFile(index);
  }
  return nullptr;
}

std::unique_ptr<File> FileManager::loadVromfsFile(const fs::path &path, bool allow_deferred) {
  ZoneScopedN("FileManager::loadVromfsFile");

  SmartFSHandle file = getObject(path, allow_deferred);
  if (!file || file->getFSObjectType() != isFile)
    return nullptr;
  auto f = file.asFile();
  return f->getFile(f);
}

int FileManager::find_files_in_folder(std::vector<std::string> &out_list, std::string &dir_path,
                                      const char *file_suffix_to_match, bool vromfs, bool realfs, bool subdirs) {
  std::vector<fs::path> paths{};
  this->find_vromfs_files_in_folder(paths, dir_path);
  for (auto &p: paths) {
    out_list.push_back(p.string());
  }
  return 1;
}

FileManager file_mgr{};
