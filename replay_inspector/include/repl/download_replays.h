

#pragma once
#include <curl/curl.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

static const std::string REPLAY_FOLDER_PATH = "replays";
static const std::string NET_RPL_PATH = "https://wt-game-replays.warthunder.com";

struct downloadFileCtx {
  std::atomic_uint16_t curr_file_index;
  std::atomic_bool done;

  downloadFileCtx() {
    curr_file_index = 0;
    done = false;
  }
};

inline size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
  std::ofstream *out = static_cast<std::ofstream *>(userdata);
  out->write(static_cast<const char *>(ptr), size * nmemb);
  return size * nmemb;
}

inline std::string to_lower_hex_with_leading_zero_prefix(uint64_t battle_hex) {
  std::ostringstream oss;
  oss << "0" << std::hex << std::nouppercase << battle_hex;
  return oss.str();
}

inline void prepare_replay_directory(const std::string &path) {
  if (!fs::exists(path)) {
    fs::create_directories(path);
  } else {
    for (const auto &entry: fs::directory_iterator(path)) {
      fs::remove_all(entry.path());
    }
  }
}

inline bool download_file(CURL *curl, const std::string &url, const std::string &output_path, long &http_code) {
  std::ofstream out(output_path, std::ios::binary);
  if (!out) {
    LOGE("Failed to open file for writing: {}", output_path);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    LOGE("CURL error for {}: {}", url, curl_easy_strerror(res));
    out.close();
    fs::remove(output_path);
    return false;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  out.close();

  if (http_code != 200) {
    fs::remove(output_path);
  }

  return true;
}

inline void download_replay(downloadFileCtx *ctx, uint64_t battle_hex) {
  std::string path = fmt::format("{}/{:#x}", REPLAY_FOLDER_PATH, battle_hex);
  prepare_replay_directory(path);
  std::string battle_part = to_lower_hex_with_leading_zero_prefix(battle_hex);

  CURL *curl = curl_easy_init();
  if (!curl) {
    EXCEPTION("Failed to initialize libcurl");
    return;
  }

  for (uint16_t index = 0; index < 500;) {
    ctx->curr_file_index.store(index);
    std::ostringstream file_name_stream;
    file_name_stream << std::setw(4) << std::setfill('0') << index << ".wrpl";
    std::string current_battle = file_name_stream.str();

    std::string url = fmt::format("{}/{}/{}", NET_RPL_PATH, battle_part, current_battle);
    std::string output_path = fmt::format("{}/{}", path, current_battle);

    long http_code = 0;
    bool ok = download_file(curl, url, output_path, http_code);

    if (!ok) {
      LOGE("Download failed for {}: HTTP code {}, URL: {}", current_battle, http_code, url);
      ctx->done.store(true);
      return;
    } else if (http_code == 200) {
      // success
    } else if (http_code == 404) {
      break;
    } else {
      LOGE("WARNING STATUS CODE: {}", http_code);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (index == 0) {
      index++;
    } else {
      index += 2;
    }
  }
  ctx->done.store(true);
  curl_easy_cleanup(curl);
}
