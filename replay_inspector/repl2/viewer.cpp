
#define STBI_ASSERT(x) G_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "repl/InspectorScreen.h"
#include <SDL.h>
#include "string"
#include "Logger.h"
#include "ecs/ecsHash.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"


#include "ecs/EntityManager.h"
#include "FileSystem.h"
#include "init/initialize.h"

#include "Replay/Replay.h"
#include "state/ParserState.h"


#include "imgui-utils/load_fonts.h"
#include "tinyfiledialogs.h"
#include "tracy/Tracy.hpp"
#include "../include/repl/callback.h"
#ifdef PARSER_ENABLE_INSPECTOR_CURL
#include "repl/download_replays.h"
#endif

#include <new>
#include <cstdlib>
#include <tracy/Tracy.hpp>
/*
void* operator new(std::size_t size) {
  if (void* ptr = std::malloc(size)) {
    TracyAllocS(ptr, size, 6);
    return ptr;
  }
  throw std::bad_alloc{};
}

void operator delete(void* ptr) noexcept {
  if (ptr) {
    TracyFreeS(ptr, 6);
    std::free(ptr);
  }
}

void* operator new[](std::size_t size) {
  if (void* ptr = std::malloc(size)) {
    TracyAllocS(ptr, size, 6);
    return ptr;
  }
  throw std::bad_alloc{};
}

void operator delete[](void* ptr) noexcept {
  if (ptr) {
    TracyFreeS(ptr, 6);
    std::free(ptr);
  }
}

void operator delete(void* ptr, std::size_t) noexcept {
  if (ptr) {
    TracyFreeS(ptr, 6);
    std::free(ptr);
  }
}

void operator delete[](void* ptr, std::size_t) noexcept {
  if (ptr) {
    TracyFreeS(ptr, 6);
    std::free(ptr);
  }
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  if (void* ptr = std::malloc(size)) {
    TracyAllocS(ptr, size, 6);
    return ptr;
  }
  return nullptr;
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
  if (void* ptr = std::malloc(size)) {
    TracyAllocS(ptr, size, 6);
    return ptr;
  }
  return nullptr;
}*/


ECS_DECL_PULL_VAR(query_draw_units);

volatile size_t inspector_pulls = ecs_pull_query_draw_units;


// static float current_look_direction;


#ifdef PARSER_ENABLE_INSPECTOR_CURL
void download_replay_internal(downloadFileCtx *ctx, uint64_t session_id) {
  ctx->done.store(false);
  download_replay(ctx, session_id);
}
#endif

struct ReplayPath {
  fs::path dir_path;
  Replay *rpl;
  std::string pretty_name{};
};

struct WindowMgr {
  std::vector<ReplayHandler *> states{};
#ifdef PARSER_ENABLE_INSPECTOR_CURL
  std::unique_ptr<downloadFileCtx> download_file_ctx{};
#endif

  std::thread thread{};

  std::vector<ReplayPath> replay_dirs{}; // all the 0000.wrpl
  std::unordered_set<uint64_t> replay_session_ids{};
  // all the session ids of the replays in replay_dirs, used to prevent downloading duplicates

  std::string error_message{};
  bool show_error_popup = false;
  char path[1024];
  char download[100];

  void check_internal(std::unique_ptr<IReplay> rpl) {
    if (!rpl->isValid()) {
      error_message = "Invalid Replay File";
      show_error_popup = true;
      return;
    }
    this->states.push_back(new ReplayHandler(std::move(rpl)));
  }


  void add_folder(const std::string &folder_path) { check_internal(std::make_unique<ServerReplay>(folder_path)); }

  void add_file(const std::string &file_path) { check_internal(std::make_unique<Replay>(file_path)); }

  WindowMgr() {
    memset(path, 0, sizeof(path));
    memset(download, 0, sizeof(download));
#ifdef PARSER_ENABLE_INSPECTOR_CURL
    scan_for_replays();
#endif
  }

#ifdef PARSER_ENABLE_INSPECTOR_CURL
  void scan_for_replays() {
    ZoneScoped fs::path dir = REPLAY_FOLDER_PATH;
    if (!fs::is_directory(dir)) {
      return;
    }
    for (auto &entry: fs::directory_iterator(dir)) {
      if (entry.is_directory()) {
        auto rpl_path = fmt::format("{}\\0000.wrpl", entry.path().string());
        auto rpl = new Replay(rpl_path);
        if (!rpl->isValid()) {
          delete rpl;
          continue;
        }
        auto session_id = rpl->getHeader()->session_id;
        if (rpl->is_valid && replay_session_ids.find(session_id) == replay_session_ids.end()) {
          replay_dirs.push_back({entry.path(), rpl, fmt::format("{}; {}", session_id, rpl->header.mission_name)});
          replay_session_ids.insert(session_id);
        } else {
          delete rpl;
        }
      }
    }
  }
#endif

  void draw() {
    ZoneScoped if (show_error_popup) { ImGui::OpenPopup("Error"); }
    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Replay Error: %s", error_message.c_str());
      if (ImGui::Button("OK")) {
        ImGui::CloseCurrentPopup();
        show_error_popup = false;
      }
      ImGui::EndPopup();
    }
    if (ImGui::BeginTabBar("open files")) {
      if (ImGui::BeginTabItem("+")) {
        if (ImGui::Button("Select Replay File")) {
          constexpr const char *filters[] = {"*.wrpl"};
          const char *file =
            tinyfd_openFileDialog("select replay (.wrpl) file", "", sizeof(filters) / sizeof(filters[0]), filters,
                                  "replay (.wrpl) files", false);
          if (file) {
            std::string file_path{file};
            add_file(file_path);
          }
        }
        if (ImGui::Button("Select Server Replay Folder")) {
          const char *file = tinyfd_selectFolderDialog("Select Server Replay Folder", "");
          if (file) {
            std::string file_path{file};
            add_folder(file_path);
          }
        }
        ImGui::TextUnformatted("");
        ImGui::TextUnformatted("Enter Replay Path");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(600.0f);
        ImGui::InputText("##path_input", path, sizeof(path));
        ImGui::SameLine();
        if (ImGui::Button("Open Replay")) {
          std::string_view path_view{path};
          if (*path_view.begin() == '\"' && *(path_view.end()-1) == '\"')
            path_view = std::string_view{path+1, path_view.size()-2};
          fs::path f_path(path_view);
          std::string s_path = f_path.string();
          if (fs::exists(f_path)) {
            if (fs::is_directory(f_path)) {
              add_folder(s_path);
            } else if (fs::is_regular_file(f_path)) {
              add_file(s_path);
            }
          } else {
            show_error_popup = true;
            error_message = "Path does not exist";
          }
        }
        ImGui::TextUnformatted("");
#ifdef PARSER_ENABLE_INSPECTOR_CURL
        if (!download_file_ctx) {
          ImGui::SetNextItemWidth(200.0f);
          ImGui::TextUnformatted("Enter Server Replay Session Id");
          ImGui::SameLine();
          ImGui::InputText("##session_id_download", download, sizeof(download));
          if (ImGui::Button("Download Replay")) {
            uint64_t session_id = 0;
            try {
              session_id = std::stoull(download);
              if (session_id < 5000000) {
                session_id = std::stoull(download, nullptr, 16);
              }
            } catch (std::exception &e) {
              error_message = e.what();
              show_error_popup = true;
            }
            if (!show_error_popup) {
              if (replay_session_ids.find(session_id) != replay_session_ids.end()) {
                error_message =
                  fmt::format("replay with session id {}/{:#x} already downloaded", session_id, session_id);
                show_error_popup = true;
              }
              // replay is not already downloaded
              if (!show_error_popup) {
                download_file_ctx = std::make_unique<downloadFileCtx>();
                thread = std::thread(download_replay_internal, download_file_ctx.get(), session_id);
              }
            }
          }
        }
        if (download_file_ctx) {
          auto text = fmt::format("Downloading Replay Part {:04}.wrpl", download_file_ctx->curr_file_index.load());
          ImGui::TextUnformatted(text.c_str());
        }
        if (download_file_ctx && download_file_ctx->done.load()) {
          thread.join();
          download_file_ctx.reset(nullptr);
          scan_for_replays();
        }

        ImGui::TextUnformatted("");

        ImGui::TextUnformatted("Saved Replays:");
        for (auto &rpl_st: replay_dirs) {
          ImGui::PushID((int) rpl_st.rpl->header.session_id);
          if (ImGui::Button("Open Replay")) {
            add_folder(rpl_st.dir_path.string());
          }
          ImGui::PopID();
          ImGui::SameLine();
          ImGui::TextUnformatted(rpl_st.pretty_name.c_str());
        }
#endif
        ImGui::EndTabItem();
      }

      std::vector<ReplayHandler *> scheduled_for_deletion{};
      ZoneScopedN("States") for (auto st: states) {
        bool is_open = true;
        if (ImGui::BeginTabItem(st->session_id_hex.c_str(), &is_open)) {
          st->runAll();
          ImGui::EndTabItem();
          if (!is_open) {
            scheduled_for_deletion.push_back(st);
          }
        }
      }
      for (auto st: scheduled_for_deletion) {
        auto it = std::find(states.begin(), states.end(), st);
        if (it != states.end()) {
          states.erase(it);
          delete st;
        }
      }
      ImGui::EndTabBar();
    }
  }

  ~WindowMgr() {
    if (thread.joinable()) {
      thread.join();
    }
    for (auto st: states) {
      delete st;
    }
    for (auto &d: replay_dirs) {
      delete d.rpl;
    }
  }
};

std::string convert_os_path_to_wsl2(std::string &str) {
  // this function assumes a windows os with a wsl2 linux
  G_ASSERTF(str[1] == ':', "must be an absolute path");
  std::string payload = "/mnt/";
  payload += static_cast<char>(std::tolower(str[0]));
  payload += "/";
  payload += str.substr(3);
  std::replace(payload.begin(), payload.end(), '\\', '/');
  return payload;
}

std::string convert_os_path_to_wsl2(const char *str) {
  std::string t(str);
  return convert_os_path_to_wsl2(t);
}

int main(int argc, char *argv[]) {
#ifdef PARSER_ENABLE_INSPECTOR_CURL
  curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
  // download_replay(0x6a62d480035ed8c);
  std::string map_path = "mapOut/png/2048";
  std::string config_path = "viewer_config.blk";
  if (argc == 3) {
#ifdef _TARGET_PC_LINUX
    map_path = convert_os_path_to_wsl2(argv[1]);
    config_path = convert_os_path_to_wsl2(argv[2]);
#else
    map_path = argv[1];
    config_path = argv[2];
#endif
  }
  file_mgr.add_mount(map_path);
  DataBlock conf_blk;
  G_ASSERT(dblk::load(conf_blk, config_path.c_str()));
  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
  default_minimap_size = conf_blk.getPoint2("default_minimap_size", default_minimap_size);
  std::string bin_path_str = bin_path;
#ifdef _TARGET_PC_LINUX
  // if(!source_is_linux_path) {
  //   rpl_path_str = convert_os_path_to_wsl2(replay_path);
  //  }
  if (!bin_is_linux_path) {
    bin_path_str = convert_os_path_to_wsl2(bin_path);
  }
#else
  G_UNUSED(bin_is_linux_path);
#endif

  // std::string log_file = (conf_dir / "logfile.txt").string();
  std::string log_file = "logfile.txt";
  initialize(bin_path_str, log_file, true);
  g_log_handler->start_thread();
  LOG("SDL video drivers:");
  for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
    LOG("  {}: {}", i, SDL_GetVideoDriver(i));
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    LOG("Error: {}", SDL_GetError());
    return -1;
  }


  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_::;      // Enable Docking
  //  Setup Dear ImGui style
  ImGui::StyleColorsDark();
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  // ImGui::StyleColorsLight();  // Uncomment for light theme
  // Create window with SDL_Renderer graphics context
  // Create OpenGL-enabled window
  // Create OpenGL-enabled window
  SDL_WindowFlags window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_Window *window =
    SDL_CreateWindow("Replay Inspector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  if (window == nullptr) {
    LOG("Error: SDL_CreateWindow(): {}\n", SDL_GetError());
    return -1;
  }

  // Set GL attributes before creating context (request 3.3 core)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  // Create context
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == NULL) {
    LOG("Error creating GL context: {}\n", SDL_GetError());
    return -1;
  }
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // vsync

  LOG("SDL video driver: {}", SDL_GetCurrentVideoDriver());

  // Init ImGui backends for SDL + OpenGL3
  const char *glsl_version = "#version 330";
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);
  load_imgui_fonts();
  bool done = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  WindowMgr windows{};
  // std::string f_path = R"(D:\SteamLibrary\steamapps\common\War Thunder\Replays\#2026.05.19 21.16.58.wrpl)";
  // std::string f_path_2 = R"(C:\Users\samue\Downloads\260516 Blitz QUAL Desert 2.wrpl)";
#ifdef _TARGET_PC_LINUX
  // f_path = convert_os_path_to_wsl2(f_path);
  // f_path_2 = convert_os_path_to_wsl2(f_path_2);
#endif
  // windows.add_file(f_path);
  // windows.add_file(f_path_2);

  while (!done) {
    // Poll and handle events
    {
      ZoneScopedN("SDL Process Event") SDL_Event event;
      while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
          done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window))
          done = true;
      }
    }
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }


    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // auto viewport = ImGui::GetMainViewport();
    // ImGui::SetNextWindowPos(viewport->Pos);
    // ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags fullFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    bool thang = true;

    // ... inside the frame, before ImGui::Begin(...)
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    // Place the window to cover the whole viewport work area (avoids OS taskbar)
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    {
      ZoneScopedN("RenderBody") {
        ZoneScopedN("MyRenderBody") ImGui::Begin("Replay Inspect", &thang, fullFlags);
        // Rendering
        windows.draw();
        ImGui::End();
      }
      {
        ZoneScopedN("Render Class")


          // Main framebuffer render
          int display_w,
          display_h;
        SDL_GL_GetDrawableSize(window, &display_w, &display_h);
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int) display_w, (int) display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        // Handle multiple viewports
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
          // Backup current context/window
          SDL_Window *backup_window = SDL_GL_GetCurrentWindow();
          SDL_GLContext backup_context = SDL_GL_GetCurrentContext();

          ImGui::UpdatePlatformWindows();
          ImGui::RenderPlatformWindowsDefault();

          // Restore context
          SDL_GL_MakeCurrent(backup_window, backup_context);
        }
      }
    }
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_DestroyWindow(window);
  SDL_Quit();
#ifdef PARSER_ENABLE_INSPECTOR_CURL
  curl_global_cleanup();
#endif
  return 0;
}
