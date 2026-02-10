
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <stdio.h>
#include "string"
#include "array"
#include "Logger.h"
#include "ecs/ecsHash.h"

#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
#include "network/CNetwork.h"
#include "FileSystem.h"
#include "init/initialize.h"
#include "mpi/mpi.h"

#include <ctime>
#include "Replay/Replay.h"
#include "Replay/find_rpl_files.h"
#include "mpi/ObjectDispatcher.h"
#include "Logger.h"
#include <algorithm>

#include "state/ParserState.h"

struct HashName {
  char name[100]{};
  hash_str_t hash = 0;
  bool DoesHashMatch() {
    auto new_hash = ecs_str_hash(name);
    if (new_hash != hash) {
      hash = new_hash;
      return false;
    }
    return true;
  }
};

struct ComponentRef {
  uint32_t index;
  ecs::ComponentInfo * comp;
};

class {
  std::vector<ComponentRef> refs;
public:
  std::vector<ComponentRef*> Components{};
  size_t curr_size = 0;
  HashName ComponentHash;
  void init() {
    auto comps = ecs::g_ecs_data->getComponentTypes();
    refs.resize(comps->types.size());
    Components.resize(comps->types.size());

    for(int i = 0; i < Components.size(); i++) {
      refs[i].comp = &comps->types[i];
      refs[i].index = i;
      Components[i] = &refs[i];
    }
    curr_size = Components.size();
  }

  void checkChange() {
    if (!ComponentHash.DoesHashMatch()) { //
      curr_size = 0;
      for(auto &comp: refs) {
        if(comp.comp->name.contains(ComponentHash.name)) {
          Components[curr_size++] = &comp;
        }
      }
    }
  }

  std::span<ComponentRef*> get_span() {
    return {Components.data(), curr_size};
  }

} ComponentStorage;

void ComponentsTabs() {
  constexpr uint32_t flags = ImGuiTableFlags_::ImGuiTableFlags_Borders | ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendX;
  if(ImGui::BeginTabItem("Components")) {
    ImGui::InputText("Filter by component name", ComponentStorage.ComponentHash.name, sizeof(ComponentStorage.ComponentHash.name));
    ComponentStorage.checkChange();
    auto span = ComponentStorage.get_span();
    if(ImGui::BeginChild("components content womp womp")) {
      ImGui::TextUnformatted(fmt::format("Component Count {}", span.size()).c_str());
      if(ImGui::BeginTable("defs", 6, flags)) {
        ImGui::TableSetupColumn("Index");
        ImGui::TableSetupColumn("Is Pod");
        ImGui::TableSetupColumn("Has Io");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("Hash");
        ImGui::TableSetupColumn("Name");
        ImGui::TableHeadersRow();
        for(auto comp : span) {
          bool is_pod = ecs::is_pod(comp->comp->flags);
          bool has_io = ecs::has_io(comp->comp->flags);
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(fmt::format("{}", comp->index).c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(is_pod ? "yes" : "no");
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(has_io ? "yes" : "no");
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(fmt::format("{}", comp->comp->size).c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(fmt::format("{:#x}", comp->comp->hash).c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(comp->comp->name.data());
        }
        ImGui::EndTable();
      }
      ImGui::EndChild();
    }
    ImGui::EndTabItem();
  }
}

struct DataComponentRef {
  uint32_t index;
  ecs::DataComponent * ref;
  ComponentRef * c_ref;
};

class {
public:
  std::vector<DataComponentRef> refs;
  std::vector<DataComponentRef*> DataComponents;
  size_t curr_size = 0;
  HashName DataComponentHash;
  HashName ComponentHash;
  void init() {
    auto comps = ecs::g_ecs_data->getDataComponents();
    refs.resize(comps->size());
    DataComponents.resize(comps->size());

    for(int i = 0; i < DataComponents.size(); i++) {
      refs[i].ref = &comps->components[i];
      refs[i].c_ref = ComponentStorage.Components[comps->components[i].componentIndex];
      refs[i].index = i;
      DataComponents[i] = &refs[i];
    }
    curr_size = DataComponents.size();
  }

  void checkChange() {
    bool matches = DataComponentHash.DoesHashMatch() && ComponentHash.DoesHashMatch();
    if (!matches) { //
      curr_size = 0;
      for(auto &comp: refs) {
        if(comp.ref->getName().contains(DataComponentHash.name) && comp.c_ref->comp->name.contains(ComponentHash.name)) {
          DataComponents[curr_size++] = &comp;
        }
      }
    }
  }

  std::span<DataComponentRef*> get_span() {
    return {DataComponents.data(), curr_size};
  }
} DataComponentStorage;

void DataComponentsTabs() {
  constexpr uint32_t flags = ImGuiTableFlags_::ImGuiTableFlags_Borders | ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendX;
  if(ImGui::BeginTabItem("Data Components")) {
    ImGui::InputText("Filter by Component name", DataComponentStorage.ComponentHash.name, sizeof(DataComponentStorage.ComponentHash.name));
    ImGui::InputText("Filter by DataComponent name", DataComponentStorage.DataComponentHash.name, sizeof(DataComponentStorage.DataComponentHash.name));
    DataComponentStorage.checkChange();
    auto span = DataComponentStorage.get_span();
    if(ImGui::BeginChild("components content womp womp")) {
      ImGui::TextUnformatted(fmt::format("Component Count {}", span.size()).c_str());
      if(ImGui::BeginTable("defs", 5, flags)) {
        ImGui::TableSetupColumn("Index");
        ImGui::TableSetupColumn("Hash");
        ImGui::TableSetupColumn("Has Io");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Component Name");
        ImGui::TableHeadersRow();
        for(auto comp : span) {

          bool has_io = comp->ref->serializer != nullptr;
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(fmt::format("{}", comp->index).c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(fmt::format("{:#x}", comp->ref->hash).c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(has_io ? "yes" : "no");
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(comp->ref->getName().data());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(comp->c_ref->comp->name.data());
        }
        ImGui::EndTable();
      }
      ImGui::EndChild();
    }
    ImGui::EndTabItem();
  }
}



struct TemplateRef {
  ecs::Template * ref;
  uint16_t gen = 0; // when we are doing a lookup, we increment generation. if a TemplateRef generation doesnt match current, then we rescan and set include
  bool include = true;
};

class {
  std::vector<TemplateRef> refs;
  std::vector<bool> filter_data{}; // bit vector
  uint16_t curr_gen = 1;
  bool isTemplateValid(TemplateRef*ref) {
    // Check if already processed this generation
    if(ref->gen == curr_gen)
      return ref->include;

    // Mark as current generation (prevents cycles)
    ref->gen = curr_gen;

    // Check name validity
    if(!ref->ref->getName().contains(TemplateHash.name)) {
      ref->include = false;
      return false;
    }

    // Check components
    for(auto &c : ref->ref->getComponents()) {
      if(filter_data[c.comp_type_index]) {
        ref->include = true;
        return true;
      }
    }

    // Check parents
    for(auto t : ref->ref->getParents()) {
      if(t == ecs::INVALID_TEMPLATE_INDEX) {
        LOG("Invalid template index for {}", ref->ref->getName());
        continue;
      }
      if(isTemplateValid(&refs[t])) {
        ref->include = true;
        return true;
      }
    }

    // Nothing passed
    ref->include = false;
    return false;
  }
public:
  std::vector<TemplateRef*> Templates;
  uint32_t curr_size;
  HashName TemplateHash;
  HashName DataComponentHash;
  HashName ComponentHash;
  void init() {
    auto &comps = ecs::g_ecs_data->getTemplateDB()->getTemplates();
    refs.resize(comps.size());
    Templates.resize(comps.size());
    filter_data.resize(DataComponentStorage.refs.size());
    for(int i = 0; i < Templates.size(); i++) {
      refs[i].ref = &comps[i];
      Templates[i] = &refs[i];
    }
  }
  TemplateRef* operator[](ecs::template_t tid) {
    return &refs[tid];
  }
  void checkChange() {
    bool matches = TemplateHash.DoesHashMatch();
    auto comp_matches =  DataComponentHash.DoesHashMatch() && ComponentHash.DoesHashMatch();
    if (!matches || !comp_matches) { //
      curr_size = 0;
      if(!comp_matches) { // only updated components if templates update
        std::fill(filter_data.begin(), filter_data.end(), false);
        for(auto &comp: DataComponentStorage.refs) {
          if(comp.ref->getName().contains(DataComponentHash.name) && comp.c_ref->comp->name.contains(ComponentHash.name)) {
            filter_data[comp.index] = true;
          }
        }
      }
      for(auto &comp: refs) {
        if(isTemplateValid(&comp)) {
          Templates[curr_size++] = &comp;
        }
      }
      curr_gen++;
    }
  }
  std::span<TemplateRef*> get_span() {
    return {Templates.data(), curr_size};
  }
} TemplateStorage;



void DrawTemplate(TemplateRef*ref) {
  constexpr uint32_t flags = ImGuiTableFlags_::ImGuiTableFlags_Borders | ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendX;
  if(ImGui::TreeNode(ref->ref->getName().c_str())) {
    if(ImGui::BeginTable("template components", 3, flags)) {
      ImGui::TableSetupColumn("DataComponent Name");
      ImGui::TableSetupColumn("Component Name");
      ImGui::TableSetupColumn("Value");
      ImGui::TableHeadersRow();
      auto &comps = ref->ref->getComponents();
      for(auto & comp : comps) {
        auto &datacomp = DataComponentStorage.refs[comp.comp_type_index];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(datacomp.ref->getName().data());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(datacomp.c_ref->comp->name.data());
        ImGui::TableNextColumn();
        //LOG("Attempting to print {}", datacomp.ref->getName().data());
        //g_log_handler.wait_until_empty();
        //g_log_handler.flush_all();
        std::string val = comp.default_component.getComponentRef().toString(nullptr);
        ImGui::TextUnformatted(val.c_str());
      }
    }
    ImGui::EndTable();
    for(auto t : ref->ref->getParents()) {
      DrawTemplate(TemplateStorage[t]);
    }
    ImGui::TreePop();
  }

}

void TemplateTabs() {
  if(ImGui::BeginTabItem("Templates")) {
    ImGui::InputText("Filter by Component name", TemplateStorage.ComponentHash.name, sizeof(TemplateStorage.ComponentHash.name));
    ImGui::InputText("Filter by DataComponent name", TemplateStorage.DataComponentHash.name, sizeof(TemplateStorage.DataComponentHash.name));
    ImGui::InputText("Filter by Template name", TemplateStorage.TemplateHash.name, sizeof(TemplateStorage.TemplateHash.name));
    TemplateStorage.checkChange();
    for(auto t : TemplateStorage.get_span()) {
      DrawTemplate(t);
    }
    ImGui::EndTabItem();
  }
}


typedef void (*run)();

run cbs[] = {
    ComponentsTabs,
    DataComponentsTabs,
    TemplateTabs
};

void RunAllTabs() {
  ImGui::BeginTabBar("ecs tabs");
  for(auto &cb : cbs) {
    cb();
  }
  ImGui::EndTabBar();
}

std::string convert_os_path_to_wsl2(std::string &str) { // this function assumes a windows os with a wsl2 linux
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

int main(int argc, char* argv[]) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }


  fs::path conf_dir = CONFIG_DIR;
  g_log_handler.set_default_sink_logfile((conf_dir / "logfile.txt").string());
  g_log_handler.start_thread();
  fs::path config_file = conf_dir / "dagor_replay_test.blk";
  DataBlock conf_blk;
  G_ASSERT(load(conf_blk, config_file.string().c_str()));

  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
#ifdef Linux
  std::string bin_path_str = convert_os_path_to_wsl2(bin_path);
#else
  std::string bin_path_str = bin_path;
#endif
  G_UNUSED(bin_is_linux_path);
  initialize(bin_path_str);
  ComponentStorage.init();
  DataComponentStorage.init();
  TemplateStorage.init();


  // Create window with SDL_Renderer graphics context
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window = SDL_CreateWindow(
      "Template and Component Inspector",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      1280,
      720,
      window_flags
  );

  if (window == nullptr) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return -1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
    return -1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_::;      // Enable Docking
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();  // Uncomment for light theme

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
  // Main loop
  bool done = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  while (!done) {
    // Poll and handle events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr auto windowsFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    bool thang;
    ImGui::Begin("ECS Info", &thang, windowsFlag);
    RunAllTabs();
    // Rendering
    ImGui::End();
    ImGui::Render();
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer,
                           (Uint8)(clear_color.x * 255),
                           (Uint8)(clear_color.y * 255),
                           (Uint8)(clear_color.z * 255),
                           (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  // Cleanup
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}