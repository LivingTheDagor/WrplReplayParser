#include "Replay/Replay.h"

void readFilesFromDirectory(const fs::path& dirPath, std::vector<fs::path>& fileSet) {
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
    return;
  }

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    if (fs::is_regular_file(entry.status())) {
      fileSet.emplace_back(fs::absolute(entry.path()));
    }
  }
}

std::string file_exists(std::string path, const std::vector<fs::path>& paths)
{
  for(auto &path_ : paths)
  {
    if(path_.filename().string() == path)
      return path_.string();
  }
  return {};
}