

#ifndef WTFILEUTILS_MAIN_WRAPPER_H
#define WTFILEUTILS_MAIN_WRAPPER_H


#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h> // SetProcessDPIAware()
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include "Logger.h"
#include "math/integer/dag_IPoint2.h"

SDL_GLContext g_gl_ctx;
SDL_Window *g_sdl_window;


int setup_sdl2(const char *window_name, IPoint2 screen_size = {1900, 1000}) {
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
  g_sdl_window =
    SDL_CreateWindow("Replay Inspector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  if (g_sdl_window == nullptr) {
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
  g_gl_ctx = SDL_GL_CreateContext(g_sdl_window);
  if (g_gl_ctx == NULL) {
    LOG("Error creating GL context: {}\n", SDL_GetError());
    return -1;
  }
  SDL_GL_MakeCurrent(g_sdl_window, g_gl_ctx);
  SDL_GL_SetSwapInterval(1); // vsync

  LOG("SDL video driver: {}", SDL_GetCurrentVideoDriver());

  // Init ImGui backends for SDL + OpenGL3
  const char *glsl_version = "#version 330";
  ImGui_ImplSDL2_InitForOpenGL(g_sdl_window, g_gl_ctx);
  ImGui_ImplOpenGL3_Init(glsl_version);
  bool done = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  return 0;
}

typedef void (*render_cb_t)();

// 1900, 1000 instead of 1920, 1080 to allow for borders to show instead of fake fullscreen
int mainloop_sdl2(render_cb_t cb) {
  bool done = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  auto window = g_sdl_window;
  ImGuiIO &io = ImGui::GetIO();

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
    bool thang = true;

    // ... inside the frame, before ImGui::Begin(...)
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    // Place the window to cover the whole viewport work area (avoids OS taskbar)
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    {
      ZoneScopedN("RenderBody") {
        ZoneScopedN("MyRenderBody") ImGui::Begin("Replay Inspect", &thang);
        cb();
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

  SDL_GL_DeleteContext(g_gl_ctx);
  SDL_DestroyWindow(window);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

#endif // WTFILEUTILS_MAIN_WRAPPER_H
