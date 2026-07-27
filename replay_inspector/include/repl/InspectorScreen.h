#pragma once
#include "repl/InspectorECS.h"

#include "imgui.h"
#include "stb_image.h"
#include "math/dag_mathAng.h"

#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#ifdef _WIN32
#include <windows.h> // SetProcessDPIAware()
#endif


#include "imgui_internal.h"
#include "state/ParserState.h"
#include "ReflectionDefs.h"


class MapImage {
  bool loadTexture(std::span<char> img_data) {
    int channels;
    unsigned char *data = stbi_load_from_memory((const unsigned char *) (void *) img_data.data(), (int) img_data.size(),
                                                &sourceWidth, &sourceHeight, &channels, 4);
    if (!data)
      return false;

    glGenTextures(1, &glTexture);
    glBindTexture(GL_TEXTURE_2D, glTexture);

    // Texture params - adjust as needed
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Optional wrap:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload (RGBA)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sourceWidth, sourceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
  }

public:
  MapImage(std::span<char> data, Point2 &topLeft, Point2 &bottomRight, Point2 cropped_image_size) {
    croppedWidth = (int) cropped_image_size.x;
    croppedHeight = (int) cropped_image_size.y;
    // no SDL renderer dependency anymore
    G_ASSERT(this->loadTexture(data));
  }

  ~MapImage() {
    if (glTexture != 0) {
      glDeleteTextures(1, &glTexture);
      glTexture = 0;
    }
  }

  ImTextureID GetTexture() {
    // ImGui expects textures as ImTextureID; cast the GLuint to intptr_t -> void*
    return (ImTextureID) (intptr_t) glTexture;
  }

private:
  int sourceWidth = 0, sourceHeight = 0;
  int croppedWidth = 0, croppedHeight = 0;
  GLuint glTexture = 0;
};

std::array<ImU32, 32> player_colors = {
  IM_COL32(230, 25, 75, 255), // Red
  IM_COL32(60, 180, 75, 255), // Green
  IM_COL32(255, 225, 25, 255), // Yellow
  IM_COL32(0, 130, 200, 255), // Blue
  IM_COL32(245, 130, 48, 255), // Orange
  IM_COL32(145, 30, 180, 255), // Purple
  IM_COL32(70, 240, 240, 255), // Cyan
  IM_COL32(240, 50, 230, 255), // Magenta
  IM_COL32(210, 245, 60, 255), // Lime
  IM_COL32(250, 190, 190, 255), // Pink
  IM_COL32(0, 128, 128, 255), // Teal
  IM_COL32(230, 190, 255, 255), // Lavender
  IM_COL32(170, 110, 40, 255), // Brown
  IM_COL32(255, 250, 200, 255), // Beige
  IM_COL32(128, 0, 0, 255), // Maroon
  IM_COL32(170, 255, 195, 255), // Mint
  IM_COL32(128, 128, 0, 255), // Olive
  IM_COL32(255, 215, 180, 255), // Apricot
  IM_COL32(0, 0, 128, 255), // Navy
  IM_COL32(128, 128, 128, 255), // Grey
  IM_COL32(255, 255, 255, 255), // White
  IM_COL32(0, 0, 0, 255), // Black
  IM_COL32(255, 204, 203, 255), // Light Pink
  IM_COL32(64, 224, 208, 255), // Turquoise
  IM_COL32(250, 128, 114, 255), // Salmon
  IM_COL32(135, 206, 235, 255), // Sky Blue
  IM_COL32(123, 104, 238, 255), // Medium Slate Blue
  IM_COL32(255, 239, 213, 255), // Papaya Whip
  IM_COL32(72, 61, 139, 255), // Dark Slate Blue
  IM_COL32(189, 183, 107, 255), // Dark Khaki
  IM_COL32(240, 230, 140, 255), // Khaki
  IM_COL32(244, 164, 96, 255) // Sandy Brown
};

std::array<ImU32, 3> teamColors = {
  IM_COL32(100, 100, 100, 255), // grey
  IM_COL32(255, 50, 50, 255), // Red
  IM_COL32(50, 50, 255, 255), // Green

};

Point2 default_minimap_size{600, 600};

// MapInfo represents all the needed data
struct MapInfo {
  MapImage *curr_image = nullptr;
  Point2 mapTopLeft; // absolute
  Point2 mapBottomRight;
  Point2 difference;
  Point2 topLeftUV; // uv for cropped map
  Point2 bottomRightUV;
  UnitType drawn_units{};

  Point2 mapTopLeftResized; // top left absolute cord of the minimap
  Point2 mapBottomRightResized; // bottom right absolute coord of the minimap
  Point2 differenceResized; // difference of TopLeft and BottomRight

  // these are same as above, but have zoom logic applied to them
  Point2 mapTopLeftZoomed;
  Point2 mapBottomRightZoomed;
  Point2 differenceZoomed;

  ImVec2 mapSize{default_minimap_size[0], default_minimap_size[1]};
  float slider_map_size = default_minimap_size[0];
  bool show_as_popup = false;

  ImVec2 centerLocalUV{0.5f, 0.5f}; // center of the zoom window in local cropped UV space [0..1]
  // temporary pan state:
  bool isPanning = false;
  ImVec2 panMouseStart{0, 0}; // screen coords where pan started
  ImVec2 panCenterStart{0.5f, 0.5f}; // centerLocalUV when pan started

  float current_zoom = 1.0f;

  MapInfo(UnitType type) : drawn_units(type) {}

  ~MapInfo() { clear(); }

  void clear() {
    delete curr_image;
    curr_image = nullptr;
    mapTopLeft = {0, 0};
    mapBottomRight = {0, 0};
    difference = {0, 0};
    topLeftUV = {1, 1};
    bottomRightUV = {0, 0};
  }
};


void DrawCapturePoint(ImDrawList *draw_list, ImVec2 center, float radius, int captureValue) {
  captureValue = ImClamp(captureValue, -100, 100);

  ImU32 color_filled = IM_COL32(0, 0, 0, 50);
  ImU32 color;
  if (captureValue == 0)
    color = IM_COL32(0, 0, 0, 200);
  else if (captureValue < 0) {
    color = IM_COL32((uint8_t) std::min(255, (int) ((-captureValue) * 2.55)), 0, 0, 200);
  } else {
    color = IM_COL32(0, 0, (uint8_t) std::min(255, (int) ((-captureValue) * 2.55)), 200);
  }
  draw_list->AddCircle(center, radius, color, 0, 2.0f);
  draw_list->AddCircleFilled(center, radius, color_filled);

  const float progress = ImAbs((float) captureValue) / 100.0f;
  if (progress <= 0.0f)
    return;

  // 20% alpha = about 51/255
  const ImU32 fillColor = (captureValue < 0) ? IM_COL32(255, 0, 0, 100) // red team
                                             : IM_COL32(0, 128, 255, 100); // blue team

  const float start_angle = -IM_PI * 0.5f; // start from top
  const float end_angle = start_angle + (IM_PI * 2.0f * progress);

  const int segments = 64;

  draw_list->PathClear();
  draw_list->PathLineTo(center);

  for (int i = 0; i <= segments; ++i) {
    const float t = (float) i / (float) segments;
    const float a = start_angle + (end_angle - start_angle) * t;

    draw_list->PathLineTo(ImVec2(center.x + cosf(a) * radius, center.y + sinf(a) * radius));
  }

  draw_list->PathFillConvex(fillColor);
}

void DrawX(ImDrawList *draw_list, ImVec2 center, float size, float thickness) {
  ImVec2 topLeft, topRight, bottomLeft, bottomRight;
  topLeft = ImVec2(center.x + size / 2, center.y + size / 2);
  topRight = ImVec2(center.x + size / 2, center.y - size / 2);
  bottomLeft = ImVec2(center.x - size / 2, center.y + size / 2);
  bottomRight = ImVec2(center.x - size / 2, center.y - size / 2);
  draw_list->AddLine(topLeft, bottomRight, IM_COL32(255, 0, 0, 255), thickness);
  draw_list->AddLine(topRight, bottomLeft, IM_COL32(255, 0, 0, 255), thickness);
}


ImVec2 UVToScreen(MapInfo &map, const Point3 &location, const ImVec2 &canvas_origin, const ImVec2 &canvas_size,
                  bool &do_draw, bool force_draw = false) {
  // Convert world position to UV
  float ImageCenterZ = map.mapTopLeftResized.y + map.differenceResized.y * 0.5f;
  float new_z = 2.0f * ImageCenterZ - location.z;

  Point2 uv{(location.x - map.mapTopLeftZoomed.x) / map.differenceZoomed.x,
            ((new_z) -map.mapTopLeftZoomed.y) / map.differenceZoomed.y};

  if (!force_draw)
    if (uv.x < -0.1f || uv.x > 1.1f || uv.y < -0.1f || uv.y > 1.1f) {
      do_draw = false;
      return {};
    }

  return ImVec2(canvas_origin.x + (uv.x * canvas_size.x), canvas_origin.y + (uv.y * canvas_size.y));
}

void DrawYawLine(float yaw_rad, ImVec2 center, float len, ImU32 col, float thickness = 2.0f) {
  // Screen coords: +x right, +y down
  // If yaw=0 should point "up", subtract PI/2 or swap sin/cos as needed.
  ImVec2 dir = ImVec2(cosf(yaw_rad), sinf(yaw_rad));

  ImVec2 tip = ImVec2(center.x + dir.x * len, center.y + dir.y * len);

  ImDrawList *dl = ImGui::GetWindowDrawList();
  dl->AddLine(center, tip, col, thickness);

  // optional arrow head
  float ah = 8.0f;
  float a = yaw_rad;
  ImVec2 l = ImVec2(tip.x - cosf(a - 0.35f) * ah, tip.y - sinf(a - 0.35f) * ah);
  ImVec2 r = ImVec2(tip.x - cosf(a + 0.35f) * ah, tip.y - sinf(a + 0.35f) * ah);
  dl->AddTriangleFilled(tip, l, r, col);
}

class ReplayHandler {
protected:
  static Point3 TMToCenter(const TMatrix &tm) {
    Point3 ret = tm.col[3];
    auto len = tm.col[0].length();
    // ret.x += cos(DegToRad(current_look_direction)) * len;
    // ret.z += sin(DegToRad(current_look_direction)) * len;
    ret.z += len; // cos(pi/2) == 0, sin(pi/2) == 1, so this is just that but simplified
    return ret;
  }

  inline ImU32 get_player_color(int player_id, uint8_t team) {
    if (this->show_player_colors) {
      return player_colors[player_id % player_colors.size()];
    }
    return teamColors[team];
  }

  void calculateZoomedSizes(MapInfo &info) {
    float center_u = info.centerLocalUV.x;
    float center_v = info.centerLocalUV.y;
    float span = 1.0f / info.current_zoom;
    ImVec2 uv0(center_u - span * 0.5f, center_v - span * 0.5f);
    ImVec2 uv1(center_u + span * 0.5f, center_v + span * 0.5f);

    info.mapTopLeftZoomed = Point2(info.mapTopLeftResized.x + uv0.x * info.differenceResized.x,
                                   info.mapTopLeftResized.y + uv0.y * info.differenceResized.y);
    info.mapBottomRightZoomed = Point2(info.mapTopLeftResized.x + uv1.x * info.differenceResized.x,
                                       info.mapTopLeftResized.y + uv1.y * info.differenceResized.y);
    info.differenceZoomed = Point2(info.mapBottomRightZoomed.x - info.mapTopLeftZoomed.x,
                                   info.mapBottomRightZoomed.y - info.mapTopLeftZoomed.y);
  }

  Point2 vectorToDegrees(const Point3 &vec) {
    Point2 ret;
    ret.x = atan2f(-vec.y, sqrtf(vec.x * vec.x + vec.z * vec.z));
    ret.y = atan2f(vec.x, vec.z);
    return ret;
  }

  void DrawMap(MapInfo &info, const char *table_name) {
    ZoneScoped if (!info.curr_image) return;
    ImGui::PushID(&info);
    ImGui::BeginGroup();
    // ImGui::TextUnformatted(table_name);
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + info.mapSize.x, canvas_p0.y + info.mapSize.y);

    // --- mouse interactivity: pan (left-drag) and zoom (wheel) ---
    ImGuiIO &io = ImGui::GetIO();
    bool hovered = ImGui::IsMouseHoveringRect(canvas_p0, canvas_p1, true);
    ImVec2 mousePos = io.MousePos;

    if (hovered) {
      ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoMove;
      ImGui::GetCurrentWindow()->BeginOrderWithinParent = 0; // Optional, ensures no move this frame
      ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoScrollWithMouse;
      ImGui::SetKeyOwner(ImGuiKey_MouseWheelY, 69, ImGuiInputFlags_LockThisFrame);
    } else {
      ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_NoMove;
      ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_NoScrollWithMouse;
    }

    // start pan when left mouse clicked on the canvas
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      info.isPanning = true;
      info.panMouseStart = mousePos;
      info.panCenterStart = info.centerLocalUV;
    }

    // continue pan while mouse is down
    if (info.isPanning) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta{mousePos.x - info.panMouseStart.x, mousePos.y - info.panMouseStart.y};
        float span = 1.0f / info.current_zoom;
        // dragging the mouse moves the map opposite of the mouse movement:
        info.centerLocalUV.x = info.panCenterStart.x - (delta.x / info.mapSize.x) * span;
        // screen Y increases downwards, but local UV Y increases downwards in this codepath,
        // so we add delta.y (not subtract) for centerLocalUV.y
        info.centerLocalUV.y = info.panCenterStart.y - (delta.y / info.mapSize.y) * span;

        // clamp to [0,1]
        info.centerLocalUV.x = ImClamp(info.centerLocalUV.x, 0.0f, 1.0f);
        info.centerLocalUV.y = ImClamp(info.centerLocalUV.y, 0.0f, 1.0f);
      } else {
        // finished dragging
        info.isPanning = false;
      }
    }

    // zoom with mouse wheel, centered under cursor
    if (hovered && io.MouseWheel != 0.0f) {
      const float zoomFactorPerTick = 1.2f; // mouse wheel step factor
      float oldZoom = info.current_zoom;
      float newZoom;
      if (io.MouseWheel > 0.0f)
        newZoom = oldZoom * zoomFactorPerTick;
      else
        newZoom = oldZoom / zoomFactorPerTick;

      // clamp zoom range
      const float minZoom = 1.0f; // no zoom out below 1
      const float maxZoom = 64.0f; // change as desired
      newZoom = ImClamp(newZoom, minZoom, maxZoom);

      float oldSpan = 1.0f / oldZoom;
      float newSpan = 1.0f / newZoom;

      // mouse local position inside canvas (0..1)
      ImVec2 localMouse{(mousePos.x - canvas_p0.x) / info.mapSize.x, (mousePos.y - canvas_p0.y) / info.mapSize.y};
      localMouse.x = ImClamp(localMouse.x, 0.0f, 1.0f);
      localMouse.y = ImClamp(localMouse.y, 0.0f, 1.0f);

      // keep the world point under the mouse stable by shifting the center
      info.centerLocalUV.x = info.centerLocalUV.x + (localMouse.x - 0.5f) * (oldSpan - newSpan);
      info.centerLocalUV.y = info.centerLocalUV.y + (localMouse.y - 0.5f) * (oldSpan - newSpan);

      // clamp center
      info.centerLocalUV.x = ImClamp(info.centerLocalUV.x, 0.0f, 1.0f);
      info.centerLocalUV.y = ImClamp(info.centerLocalUV.y, 0.0f, 1.0f);

      info.current_zoom = newZoom;
    }
    float center_u = info.centerLocalUV.x;
    float center_v = info.centerLocalUV.y;
    float span = 1.0f / info.current_zoom;
    ImVec2 uv0(center_u - span * 0.5f, center_v - span * 0.5f);
    ImVec2 uv1(center_u + span * 0.5f, center_v + span * 0.5f);
    if (uv0.x < 0) {
      info.centerLocalUV.x -= uv0.x;
    }
    if (uv0.y < 0) {
      info.centerLocalUV.y -= uv0.y;
    }
    if (uv1.x > 1) {
      info.centerLocalUV.x -= (uv1.x - 1);
    }
    if (uv1.y > 1) {
      info.centerLocalUV.y -= (uv1.y - 1);
    }
    calculateZoomedSizes(info);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Draw canvas background
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(40, 40, 40, 255));

    // Draw border
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(200, 200, 200, 255), 0.0f, 0, 2.0f);

    // Clip drawing to canvas area
    draw_list->PushClipRect(canvas_p0, canvas_p1, true);
    ImGui::SetCursorScreenPos(canvas_p0);
    drawImage(info);
    float time_start = current_time;
    float time_end = current_path_length != 0.0f ? current_time - current_path_length : 0.0f;
    if (time_end < 0)
      time_end = 0;
    auto time_start_ms = (uint32_t) (time_start * 1000);
    auto time_end_ms = (uint32_t) (time_end * 1000);
    struct Data {
      uint32_t time_start_ms; // is current point in time
      uint32_t time_end_ms; // the oldest point in time we want to draw (so current_time - path_length)
      ImDrawList *draw_list;
      ImVec2 canvas_p0;
      MapInfo *info;
    };
    Data data{time_start_ms, time_end_ms, draw_list, canvas_p0, &info};
    on_unit_cb_t on_unit_cb = [&data, this](const unit::Unit &unit) {
      auto pid = unit.owner_pid;
      if (pid == -1 || pid >= this->state.players.size())
        return;
      auto team = *this->state.players[pid].team.curr();
      static std::vector<ImVec2> screen_points;
      auto &state = this->state;
      // front is earliest point
      // back is latest point
      auto & positions = unit.positions.history();
      if (positions.front().time_ms > data.time_start_ms || unit.killed_at_ms < data.time_end_ms)
        return; // no points in range, skip

      screen_points.resize(0);
      screen_points.reserve(positions.size());
      if (positions.size() <= 5)
        return;

      const Point3 *prev = nullptr;
      constexpr int max_distance = 250;
      // we iterate from front to back, so new points first
      for (int idx_ = (int) positions.size() - 6; idx_ >= 0; idx_--) {
        // lets skip over points until we get to our start time
        if (positions[idx_].time_ms > data.time_start_ms)
          continue;
        // we reached end? break
        if (positions[idx_].time_ms < data.time_end_ms)
          break;
        auto &pos = positions[idx_].data.location;
        bool do_draw = true;
        auto curr_point = UVToScreen(*data.info, pos, data.canvas_p0, data.info->mapSize, do_draw);
        if (!do_draw)
          continue;

        bool add = true;
        if (prev) {
          auto sqrt_val = sqrt((pos.x - prev->x) * (pos.x - prev->x) + (pos.z - prev->z) * (pos.z - prev->z));
          if (abs(sqrt_val) > max_distance) {
            data.draw_list->AddPolyline(screen_points.data(), (int) screen_points.size(), get_player_color(pid, team),
                                        0, // ImDrawFlags (0 = open line)
                                        3.0f // Thickness
            );
            screen_points.resize(0); // lets remove the old line and then draw a new one
            add = false;
          }
          // we go from back to front (most recent to oldest) so we want to stop adding once we hit a significant
          // distance
        }
        if (add) {
          screen_points.push_back(curr_point);
        }
        prev = &pos;
      }
      data.draw_list->AddPolyline(screen_points.data(), (int) screen_points.size(), get_player_color(pid, team),
                                  0, // ImDrawFlags (0 = open line)
                                  3.f // Thickness
      );
      auto front = unit.positions.currState();
      bool draw_front = true;
      ImVec2 front_uv = UVToScreen(*data.info, front->data.location, data.canvas_p0, data.info->mapSize, draw_front);
      draw_front = draw_front && front->time_ms <= data.time_start_ms && front->time_ms >= data.time_end_ms;
      if (draw_front) {

        auto * camera_data = unit.base_data->camera_data.currState();
        // we only want to draw data if camera_data is younger than 250 ms
        if (camera_data && state.curr_time_ms - camera_data->time_ms < 1000) {

          DrawYawLine(camera_data->data.gun_pointer.x, front_uv, 40, IM_COL32(255, 0, 0, 255), 2.0f);
          DrawYawLine(camera_data->data.camera_euler.y, front_uv, 50, IM_COL32(255, 200, 0, 255), 2.0f);
        }

        bool is_dead = unit.killed_at_ms <= data.time_start_ms && unit.killed_at_ms >= data.time_end_ms;
        if (is_dead) {
          DrawX(data.draw_list, front_uv, 10.0f, 4.0f);
        } else {
          data.draw_list->AddCircle(front_uv, 5.0f, get_player_color(pid, team), 0, 2.0f);
        }
        DrawYawLine(front->data.euler.y, front_uv, 10, IM_COL32(255, 200, 0, 255), 4.0f);
      }
    };

    on_rocket_cb_t on_rocket_cb = [&data, this](const Rocket &rocket) {
      auto owner_eid = rocket.ownerEid;
      int *player__id = this->state.g_entity_mgr.getNullable<int>(owner_eid, ECS_HASH("unit__playerId"));

      if (!player__id || *player__id == -1 || *player__id >= this->state.players.size())
        return;
      auto team = *this->state.players[*player__id].team.curr();
      static std::vector<ImVec2> screen_points;
      auto &state = this->state;
      // front is earliest point
      // back is latest point
      auto &positions = rocket.positions.history();
      if (positions.front().time_ms > data.time_start_ms || rocket.destroyed_at_ms < data.time_end_ms)
        return; // no points in range, skip

      screen_points.resize(0);
      screen_points.reserve(positions.size());
      if (positions.size() <= 5)
        return;

      const Point3 *prev = nullptr;
      constexpr int max_distance = 250;
      bool found_front = false;
      const SpaceTimeEuler * front_val = nullptr;
      ImVec2 front = ImVec2(0.0f, 0.0f);
      // we iterate from front to back, so new points first
      for (int idx_ = (int) positions.size() - 6; idx_ >= 0; idx_--) {
        // lets skip over points until we get to our start time
        if (positions[idx_].time_ms > data.time_start_ms)
          continue;
        // we reached end? break
        if (positions[idx_].time_ms < data.time_end_ms)
          break;
        auto &pos = positions[idx_].data.location;
        bool do_draw = true;
        auto curr_point = UVToScreen(*data.info, pos, data.canvas_p0, data.info->mapSize, do_draw);
        if (!do_draw)
          continue;
        if (!found_front) {
          front_val = &positions[idx_].data;
          front = curr_point;
          found_front = true;
        }
        bool add = true;
        if (prev) {
          auto sqrt_val = sqrt((pos.x - prev->x) * (pos.x - prev->x) + (pos.z - prev->z) * (pos.z - prev->z));
          if (abs(sqrt_val) > max_distance) {
            data.draw_list->AddPolyline(screen_points.data(), (int) screen_points.size(),
                                        get_player_color(*player__id, team),
                                        0, // ImDrawFlags (0 = open line)
                                        1.0f // Thickness
            );
            screen_points.resize(0); // lets remove the old line and then draw a new one
            add = false;
          }
          // we go from back to front (most recent to oldest) so we want to stop adding once we hit a significant
          // distance
        }
        if (add) {
          screen_points.push_back(curr_point);
        }
        prev = &pos;
      }
      data.draw_list->AddPolyline(screen_points.data(), (int) screen_points.size(), get_player_color(*player__id, team),
                                  0, // ImDrawFlags (0 = open line)
                                  1.f // Thickness
      );

      if (found_front) {
        DrawYawLine(front_val->euler.y, front, 10, IM_COL32(255, 200, 0, 255), 4.0f);
      }
    };

    if (info.drawn_units == UnitType::TankType) {
      iterate_all_tanks(state, on_unit_cb);
      iterate_all_aircraft(state, on_unit_cb);
    } else if (info.drawn_units == UnitType::AircraftType) {
      iterate_all_aircraft(state, on_unit_cb);
    }
    if (show_missiles)
      iterate_all_rockets(state, on_rocket_cb);


#define SQUARE(x) ((x) * (x))

    for (auto objs: this->state.Zones) {

      auto curr_obj = *objs->curr();
      if (!curr_obj)
        continue;
      auto area_flags = *curr_obj->flags.curr();
      auto valid_air = info.drawn_units == UnitType::AircraftType && (area_flags & 1 << 9) != 0;
      auto valid_tank = info.drawn_units == UnitType::TankType && (area_flags & 1 << 10) != 0;
      if (valid_air || valid_tank) {
        auto true_center = curr_obj->area->tm.col[3];
        auto center = TMToCenter(curr_obj->area->tm);
        bool do_draw = true;
        auto center_vec = UVToScreen(info, center, canvas_p0, info.mapSize, do_draw, true);
        auto true_center_vec = UVToScreen(info, true_center, canvas_p0, info.mapSize, do_draw, true);
        if (!do_draw)
          continue;
        auto radius = std::sqrt((SQUARE(center_vec.x - true_center_vec.x) + SQUARE(center_vec.y - true_center_vec.y)));
        if (auto capture = dynamic_cast<CaptureZone *>(curr_obj)) {
          DrawCapturePoint(draw_list, true_center_vec, radius, *capture->mpTimeX100.curr());
        } else {
          ImU32 color_filled = IM_COL32(255, 0, 0, 50);
          ImU32 color = IM_COL32(255, 0, 0, 200);
          draw_list->AddCircle(true_center_vec, radius, color, 0, 2.0f);
          draw_list->AddCircleFilled(true_center_vec, radius, color_filled);
        }
      }
    }
    draw_list->PopClipRect();
    ImGui::EndGroup();
    ImGui::PopID();
  }

  void DrawMapWrapper(MapInfo &info, const char *table_name) {
    if (!info.curr_image)
      return;
    auto button_name = fmt::format("Popout {}", table_name);
    auto undo_button_name = fmt::format("Reset Map ##{}", table_name);
    auto popup_name = fmt::format("{} Popup", table_name);
    auto slider_name = fmt::format("{} Map Size Slider", table_name);
    if (ImGui::BeginChild(table_name, ImVec2(0, 0), RESIZE_CHILD_FLAGS)) {
      if (ImGui::Button(button_name.c_str())) {
        info.show_as_popup = true;
      }
      ImGui::SameLine();
      if (ImGui::Button(undo_button_name.c_str())) {
        info.mapSize = {default_minimap_size[0], default_minimap_size[1]};
        info.slider_map_size = default_minimap_size[0];
        info.current_zoom = 1.0f;
        info.panCenterStart = {0.5f, 0.5f};
      }
      if (info.show_as_popup) {
        ImGui::Begin(popup_name.c_str(), &info.show_as_popup, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SliderFloat(slider_name.c_str(), &info.slider_map_size, 50.f, 1200.f);
        info.mapSize.x = info.slider_map_size;
        info.mapSize.y = info.slider_map_size;
        DrawMap(info, table_name);
        ImGui::End();
      } else {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat(slider_name.c_str(), &info.slider_map_size, 50.f, 1200.f);
        info.mapSize.x = info.slider_map_size;
        info.mapSize.y = info.slider_map_size;
        DrawMap(info, table_name);
      }
    }
    ImGui::EndChild();
  }

  float current_time = 5.0f;
  float current_path_length = 0.0f;
  float play_speed = 1.0f;
  bool play_video_thing = false;
  ImVec2 longest_unit_name_size;
  std::string unit_header = "Vehicle";
  ImVec2 longest_player_name_size{};
  std::string player_header = "Name";

  uint32_t max_players_for_team = 0;

  bool show_player_colors = true;
  bool show_missiles = true;

public:
  ParserState state;
  std::unique_ptr<IReplay> replay;

  uint32_t replay_length_ms = 0;
  float replay_length_f = 0.f;
  std::string session_id_hex;
  uint64_t session_id;
  MapInfo air_map{UnitType::AircraftType};
  MapInfo tank_map{UnitType::TankType};

  void calculate_data() {
    longest_unit_name_size = get_longest_unit_name(state);
    int teams[3] = {0, 0, 0};
    ImVec2 ret = longest_player_name_size;
    for (const auto &player: state.players) {
      auto team = *player.team.curr();
      G_ASSERT(team <= 3);
      teams[team]++;

      ImVec2 temp = ImGui::CalcTextSize(player.uid.curr()->name);
      ret = temp.x > ret.x ? temp : ret;
    }
    longest_player_name_size = ret;
    // we won't ever plan to draw team0 unless if ffa comes around or something
    max_players_for_team = std::max(teams[1], teams[2]);
  }

  virtual void loadReplay() {
    ZoneScoped auto rdr = replay->getReplayReader();
    ReplayPacket p{};
    while (rdr->getNextPacket(p) && this->state.ParsePacket(p))
      ;
    this->replay_length_ms = p.timestamp_ms;
    this->replay_length_f = (float(p.timestamp_ms) / 1000.f);
    delete rdr;
  }


  ReplayHandler(std::unique_ptr<IReplay> rpl) : state(rpl.get()) {
    ZoneScoped auto header = rpl->getHeader();
    session_id_hex = fmt::format("{:#X}", header->session_id);
    this->session_id = header->session_id;
    std::string blk_path = header->mission_file;
    if (!blk_path.empty())
      load_map_info_from_blk(blk_path);
    else {
      std::string level_bin = header->level_path;
      load_map_info_from_bin(level_bin);
    }
    this->replay = std::move(rpl);
  }

  virtual ~ReplayHandler() = default;

  void load_map_info_from_blk(std::string_view path) {
    ZoneScoped DataBlock mission_blk{};
    G_ASSERTF(dblk::load(mission_blk, path), "failed to load {}", fmt::format("{}/{}", path, ".blk"));
    const char *level_path = "levels/water.bin";
    auto mission_settings = mission_blk.getBlockByNameEx("mission_settings");
    std::string level_base_path = mission_settings->getBlockByNameEx("mission")->getStr("level", level_path);
    level_base_path.resize(level_base_path.size() - 4); // remove .bin
    auto map_file_blk_path = fmt::format("{}.blk", level_base_path);
    DataBlock level_blk{};
    G_ASSERTF(dblk::load(level_blk, map_file_blk_path), "failed to load {}", map_file_blk_path);
    air_map.mapTopLeft = air_map.mapTopLeftResized = level_blk.getPoint2("mapCoord0", Point2(0, 0));
    air_map.mapBottomRight = air_map.mapBottomRightResized = level_blk.getPoint2("mapCoord1", Point2(0, 0));
    air_map.difference = air_map.differenceResized = {air_map.mapBottomRight.x - air_map.mapTopLeft.x,
                                                      air_map.mapBottomRight.y - air_map.mapTopLeft.y};

    tank_map.mapTopLeft = tank_map.mapTopLeftResized = level_blk.getPoint2("tankMapCoord0", Point2(0, 0));
    tank_map.mapBottomRight = tank_map.mapBottomRightResized = level_blk.getPoint2("tankMapCoord1", Point2(0, 0));
    tank_map.difference = tank_map.differenceResized = {tank_map.mapBottomRight.x - tank_map.mapTopLeft.x,
                                                        tank_map.mapBottomRight.y - tank_map.mapTopLeft.y};

    auto str = mission_settings->getBlockByNameEx("briefing")
                 ->getBlockByNameEx("part")
                 ->getBlockByNameEx("slide")
                 ->getBlockByNameEx("battleArea")
                 ->getStr("target", nullptr);
    air_map.topLeftUV = {0, 0};
    air_map.bottomRightUV = {1, 1}; // default values, covers the whole map

    tank_map.topLeftUV = {0, 0};
    tank_map.bottomRightUV = {1, 1}; // default values, covers the whole map

    // this logic isn't tested
    if (str) {
      TMatrix tm = mission_blk.getBlockByNameEx("areas")->getBlockByNameEx(str)->getTm("tm", {});
      Point2 center{tm[3][0], tm[3][2]};
      float x = std::sqrt(tm[0][0] * tm[0][0] + tm[0][2] * tm[0][2]) / 2;
      float z = std::sqrt(tm[2][0] * tm[2][0] + tm[2][2] * tm[2][2]) / 2;
      if (x * 2 > tank_map.difference.x) {
        x = tank_map.difference.x / 2;
      }
      if (z * 2 > tank_map.difference.y) {
        z = tank_map.difference.y / 2;
      }
      tank_map.mapTopLeftResized = {center.x - x, center.y - z};
      tank_map.mapBottomRightResized = {center.x + x, center.y + z};

      if (tank_map.mapTopLeftResized.x < tank_map.mapTopLeft.x) {
        float diff = tank_map.mapTopLeft.x - tank_map.mapTopLeftResized.x;
        tank_map.mapTopLeftResized.x += diff;
        tank_map.mapBottomRightResized.x += diff;
      }

      if (tank_map.mapTopLeftResized.y < tank_map.mapTopLeft.y) {
        float diff = tank_map.mapTopLeft.y - tank_map.mapTopLeftResized.y;
        tank_map.mapTopLeftResized.y += diff;
        tank_map.mapBottomRightResized.y += diff;
      }

      if (tank_map.mapBottomRightResized.x > tank_map.mapBottomRight.x) {
        float diff = tank_map.mapBottomRightResized.x - tank_map.mapBottomRight.x;
        tank_map.mapTopLeftResized.x -= diff;
        tank_map.mapBottomRightResized.x -= diff;
      }

      if (tank_map.mapBottomRightResized.y > tank_map.mapBottomRight.y) {
        float diff = tank_map.mapBottomRightResized.y - tank_map.mapBottomRight.y;
        tank_map.mapTopLeftResized.y -= diff;
        tank_map.mapBottomRightResized.y -= diff;
      }


      tank_map.differenceResized = {x * 2, z * 2};
      Point2 mapSize = tank_map.mapBottomRight - tank_map.mapTopLeft;
      Point2 uvMin = (tank_map.mapTopLeftResized - tank_map.mapTopLeft) / mapSize;
      Point2 uvMax = (tank_map.mapBottomRightResized - tank_map.mapTopLeft) / mapSize;
      tank_map.topLeftUV = {uvMin.x, 1.0f - uvMax.y};
      tank_map.bottomRightUV = {uvMax.x, 1.0f - uvMin.y};
    }
    std::string air_dds_path;
    std::string tank_dds_path;
    if (const char *customLevel = level_blk.getStr("customLevelMap", nullptr)) {
      air_dds_path = customLevel;
      air_dds_path.resize(air_dds_path.size() - 1);
      air_dds_path = fmt::format("levels/{}.png", air_dds_path);
    } else {
      air_dds_path = fmt::format("{}_map.png", level_base_path);
    }
    if (const char *customLevel = level_blk.getStr("customLevelTankMap", nullptr)) {
      tank_dds_path = customLevel;
      tank_dds_path.resize(tank_dds_path.size() - 1);
      tank_dds_path = fmt::format("levels/{}.png", tank_dds_path);
    } else {
      tank_dds_path = fmt::format("{}_tankmap.png", level_base_path);
    }

    auto air_dds_file = file_mgr.getFile(air_dds_path, false, false);
    if (air_dds_file) {
      G_ASSERT(air_dds_file);
      auto raw = air_dds_file->readRaw();
      air_map.curr_image = new MapImage(raw, air_map.topLeftUV, air_map.bottomRightUV, default_minimap_size);
    }
    auto tank_dds_file = file_mgr.getFile(tank_dds_path, false, false);
    if (tank_dds_file) {
      G_ASSERT(tank_dds_file);
      auto raw = tank_dds_file->readRaw();
      tank_map.curr_image = new MapImage(raw, tank_map.topLeftUV, tank_map.bottomRightUV, default_minimap_size);
    }
  }

  void load_map_info_from_bin(std::string_view bin_path) {
    ZoneScoped std::string level_base_path{bin_path};
    level_base_path.resize(level_base_path.size() - 4); // remove .bin
    auto map_file_blk_path = fmt::format("{}.blk", level_base_path);
    DataBlock level_blk{};
    G_ASSERTF(dblk::load(level_blk, map_file_blk_path), "failed to load {}", map_file_blk_path);
    air_map.mapTopLeft = air_map.mapTopLeftResized = level_blk.getPoint2("mapCoord0", Point2(0, 0));
    air_map.mapBottomRight = air_map.mapBottomRightResized = level_blk.getPoint2("mapCoord1", Point2(0, 0));
    air_map.difference = air_map.differenceResized = {air_map.mapBottomRight.x - air_map.mapTopLeft.x,
                                                      air_map.mapBottomRight.y - air_map.mapTopLeft.y};

    tank_map.mapTopLeft = tank_map.mapTopLeftResized = level_blk.getPoint2("tankMapCoord0", Point2(0, 0));
    tank_map.mapBottomRight = tank_map.mapBottomRightResized = level_blk.getPoint2("tankMapCoord1", Point2(0, 0));
    tank_map.difference = tank_map.differenceResized = {tank_map.mapBottomRight.x - tank_map.mapTopLeft.x,
                                                        tank_map.mapBottomRight.y - tank_map.mapTopLeft.y};

    air_map.topLeftUV = {0, 0};
    air_map.bottomRightUV = {1, 1}; // default values, covers the whole map

    tank_map.topLeftUV = {0, 0};
    tank_map.bottomRightUV = {1, 1}; // default values, covers the whole map

    // this logic isn't tested
    std::string air_dds_path;
    std::string tank_dds_path;
    if (const char *customLevel = level_blk.getStr("customLevelMap", nullptr)) {
      air_dds_path = customLevel;
      air_dds_path.resize(air_dds_path.size() - 1);
      air_dds_path = fmt::format("levels/{}.png", air_dds_path);
    } else {
      air_dds_path = fmt::format("{}_map.png", level_base_path);
    }
    if (const char *customLevel = level_blk.getStr("customLevelTankMap", nullptr)) {
      tank_dds_path = customLevel;
      tank_dds_path.resize(tank_dds_path.size() - 1);
      tank_dds_path = fmt::format("levels/{}.png", tank_dds_path);
    } else {
      tank_dds_path = fmt::format("{}_tankmap.png", level_base_path);
    }

    auto air_dds_file = file_mgr.getFile(air_dds_path, false, false);
    if (air_dds_file) {
      G_ASSERT(air_dds_file);
      auto raw = air_dds_file->readRaw();
      air_map.curr_image = new MapImage(raw, air_map.topLeftUV, air_map.bottomRightUV, default_minimap_size);
    }
    auto tank_dds_file = file_mgr.getFile(tank_dds_path, false, false);
    if (tank_dds_file) {
      G_ASSERT(tank_dds_file);
      auto raw = tank_dds_file->readRaw();
      tank_map.curr_image = new MapImage(raw, tank_map.topLeftUV, tank_map.bottomRightUV, default_minimap_size);
    }
  }

  void draw_circle(float radius, ImU32 color) {
    ImVec2 center = ImGui::GetCursorScreenPos();
    center.x += radius;
    center.y += ImGui::GetTextLineHeight() * 0.5f;
    ImGui::GetWindowDrawList()->AddCircleFilled(center, radius, color);
    ImGui::Dummy(ImVec2(radius * 2, ImGui::GetTextLineHeight()));
  }

  template<int team>
  void draw_player(int index) {
    ZoneScoped if (index == -1) {
      ImGui::TableNextColumn();
      ImGui::TextUnformatted("");
      ImGui::TableNextColumn();
      ImGui::TableNextColumn();
      ImGui::TableNextColumn();
      ImGui::TableNextColumn();
      return;
    }
    G_STATIC_ASSERT(team == 1 || team == 2);
    auto curr_color = get_player_color(index, team);

    constexpr ImU32 red = IM_COL32(255, 0, 0, 255);
    constexpr ImU32 green = IM_COL32(0, 255, 0, 255);
    if constexpr (team == 1) {
      ImGui::TableNextColumn();
      draw_circle(6.0f, curr_color);
      auto &player_data = this->state.players[index];
      ImGui::TableNextColumn();

      ImGui::Text("%i", player_data.dummyForRoundScore.curr()->combined);
      ImGui::TableNextColumn();

      ImGui::Text("%i:%s", index, player_data.uid.curr()->name);
      ImGui::TableNextColumn();

      auto owned_eid = *player_data.ownedUnitRef.curr();
      auto unit__ref = this->state.g_entity_mgr.getNullable<unit::UnitRef>(owned_eid, ECS_HASH("unit__ref"));
      if (!unit__ref || !unit__ref->unit) {
        ImGui::TextUnformatted("<NONE>");
        ImGui::TableNextColumn();
        draw_circle(6.0f, red);
      } else {
        auto unit = unit__ref->unit;
        auto name = get_unit_name_1(*unit);
        ImGui::TextUnformatted(name);
        ImGui::TableNextColumn();
        if (state.curr_time_ms >= unit->killed_at_ms) {
          draw_circle(6.0f, red);
        } else {
          draw_circle(6.0f, green);
        }
      }
    } else if constexpr (team == 2) {
      ImGui::TableNextColumn();
      auto &player_data = this->state.players[index];
      auto owned_eid = *player_data.ownedUnitRef.curr();
      auto unit__ref = this->state.g_entity_mgr.getNullable<unit::UnitRef>(owned_eid, ECS_HASH("unit__ref"));
      if (!unit__ref || !unit__ref->unit) {
        draw_circle(6.0f, red);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("<NONE>");
      } else {
        auto unit = unit__ref->unit;
        if (state.curr_time_ms >= unit->killed_at_ms) {
          draw_circle(6.0f, red);
        } else {
          draw_circle(6.0f, green);
        }
        ImGui::TableNextColumn();
        auto name = get_unit_name_1(*unit);
        ImGui::TextUnformatted(name);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%i:%s", index, player_data.uid.curr()->name);

      ImGui::TableNextColumn();
      ImGui::Text("%i", player_data.dummyForRoundScore.curr()->combined);

      ImGui::TableNextColumn();
      draw_circle(6.0f, curr_color);
    }
  }

  void collect_sort_players(std::vector<int> &indexes_out, uint8_t team_index) {
    for (uint8_t i = 0; i < (uint8_t) this->state.players.size(); i++) {
      auto &curr_player = this->state.players[i];
      auto team = *curr_player.team.curr();
      if (team == team_index) {
        indexes_out.push_back(i);
      }
    }
    auto &players = this->state.players;
    std::sort(indexes_out.begin(), indexes_out.end(), [&players](auto a, auto b) {
      auto &player1_score = players[a].dummyForRoundScore.curr()->combined;
      auto &player2_score = players[b].dummyForRoundScore.curr()->combined;
      return player1_score > player2_score;
    });
  }

  static constexpr ImGuiChildFlags RESIZE_CHILD_FLAGS =
    ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX;

  void drawTeam1() {
    ImGui::BeginChild("Team 1", ImVec2(0, 0), RESIZE_CHILD_FLAGS);
    ImGui::TextUnformatted("Team 1");
    std::vector<int> indexes_out;
    collect_sort_players(indexes_out, 1);
    indexes_out.resize(this->max_players_for_team, -1);

    constexpr uint32_t flags = ImGuiTableFlags_::ImGuiTableFlags_Borders |
                               ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                               ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendX;
    if (ImGui::BeginTable("Team 1", 5, flags)) {
      ImGui::TableSetupColumn("Color");
      ImGui::TableSetupColumn("Points");
      ImGui::TableSetupColumn(this->player_header.c_str(), ImGuiTableColumnFlags_WidthFixed,
                              longest_player_name_size.x + 20);
      ImGui::TableSetupColumn(this->unit_header.c_str(), ImGuiTableColumnFlags_WidthFixed,
                              longest_unit_name_size.x + 20);
      ImGui::TableSetupColumn("");
      ImGui::TableHeadersRow();
      for (auto &index: indexes_out) {
        draw_player<1>(index);
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();
  }

  void drawTeam2() {
    ImGui::BeginChild("Team 2", ImVec2(0, 0), RESIZE_CHILD_FLAGS);
    ImGui::TextUnformatted("Team 2");
    std::vector<int> indexes_out;
    collect_sort_players(indexes_out, 2);
    indexes_out.resize(this->max_players_for_team, -1);

    constexpr uint32_t flags = ImGuiTableFlags_::ImGuiTableFlags_Borders |
                               ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                               ImGuiTableFlags_::ImGuiTableFlags_NoHostExtendX;
    if (ImGui::BeginTable("Team 2", 5, flags)) {
      ImGui::TableSetupColumn("");
      ImGui::TableSetupColumn(this->unit_header.c_str(), ImGuiTableColumnFlags_WidthFixed,
                              longest_unit_name_size.x + 20);
      ImGui::TableSetupColumn(this->player_header.c_str(), ImGuiTableColumnFlags_WidthFixed,
                              longest_player_name_size.x + 20);
      ImGui::TableSetupColumn("Points");
      ImGui::TableSetupColumn("Color");
      ImGui::TableHeadersRow();
      for (auto &index: indexes_out) {
        draw_player<2>(index);
      }
      ImGui::EndTable();
    }
    ImGui::EndChild();
  }

  void drawImage(MapInfo &info) {
    ZoneScoped if (info.curr_image) {
      float center_u = info.centerLocalUV.x;
      float center_v = info.centerLocalUV.y;
      float span = 1.0f / info.current_zoom;

      // Zoom window in local cropped-space UVs (0..1 inside the crop)
      ImVec2 local_uv0(center_u - span * 0.5f, center_v - span * 0.5f);
      ImVec2 local_uv1(center_u + span * 0.5f, center_v + span * 0.5f);

      local_uv0.x = ImClamp(local_uv0.x, 0.0f, 1.0f);
      local_uv0.y = ImClamp(local_uv0.y, 0.0f, 1.0f);
      local_uv1.x = ImClamp(local_uv1.x, 0.0f, 1.0f);
      local_uv1.y = ImClamp(local_uv1.y, 0.0f, 1.0f);

      // Base crop in source texture UVs
      ImVec2 crop_uv0(info.topLeftUV.x, info.topLeftUV.y);
      ImVec2 crop_uv1(info.bottomRightUV.x, info.bottomRightUV.y);

      // Remap local zoom UVs into the crop range on the original source texture
      ImVec2 final_uv0(crop_uv0.x + (crop_uv1.x - crop_uv0.x) * local_uv0.x,
                       crop_uv0.y + (crop_uv1.y - crop_uv0.y) * local_uv0.y);

      ImVec2 final_uv1(crop_uv0.x + (crop_uv1.x - crop_uv0.x) * local_uv1.x,
                       crop_uv0.y + (crop_uv1.y - crop_uv0.y) * local_uv1.y);

      ImGui::Image(info.curr_image->GetTexture(), ImVec2(info.mapSize.x, info.mapSize.y), final_uv0, final_uv1);
    }
  }

  void drawMapTab() {

    if (ImGui::BeginChild("Top Information", ImVec2(0, 0), RESIZE_CHILD_FLAGS)) {
      drawTeam1();
      ImGui::SameLine();
      if (ImGui::BeginChild("Controls", ImVec2(0, 0), RESIZE_CHILD_FLAGS)) {
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputFloat("##input_path", &current_path_length, 0.0f, 9999.f);

        ImGui::SameLine();
        ImGui::TextUnformatted("Path length in seconds");
        ImGui::SetNextItemWidth(400.0f);
        ImGui::SliderFloat("##slider_path", &current_path_length, 0.0f, 60.0f);

        if (ImGui::Button(show_player_colors ? "Show Team Colors" : "Show Player Colors")) {
          show_player_colors = !show_player_colors;
        }
        ImGui::SameLine();
        if (ImGui::Button(show_missiles ? "Hide Missiles" : "Show Missiles")) {
          show_missiles = !show_missiles;
        }
        ImGui::EndChild();
      }
      ImGui::SameLine();
      drawTeam2();
      ImGui::EndChild();
    }
    if (ImGui::BeginChild("center data", ImVec2(0, 0), RESIZE_CHILD_FLAGS)) {
      // ImGui::SliderFloat("##degree_val", &current_look_direction, 0.0f, 360.0f);

      if (ImGui::BeginChild("child_map_slider", ImVec2(0, 0), RESIZE_CHILD_FLAGS,
                            ImGuiWindowFlags_HorizontalScrollbar)) {
        DrawMapWrapper(air_map, "Air Map");
        ImGui::SameLine();
        DrawMapWrapper(tank_map, "Ground Map");
        ImGui::EndChild();
                            }
      ImGui::EndChild();
    }
  }

  void drawReflectableObject(danet::ReflectableObject *robj, const char * obj_name) {
    char name[256] = {};
    if (obj_name) {
      fmt::format_to_n(name, 256, "{}: {} ##{}", robj->getClassName(), obj_name, robj->getUID());
    } else {
      fmt::format_to_n(name, 256, "{}: {:#x}", robj->getClassName(), robj->getUID());
    }
    if (ImGui::TreeNode(name)) {
      ImGui::BeginTable("Reflectable Object", 3, ImGuiTableFlags_Borders);
      ImGui::TableSetupColumn("History", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Var name", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Var value");
      ImGui::TableHeadersRow();
      robj->drawObject();
      ImGui::EndTable();
      ImGui::TreePop();
    }
  }

  void drawReflectableObjects() {
    drawReflectableObject(&this->state.gen_state, nullptr);
    drawReflectableObject(&this->state.glob_elo, nullptr);
    if (ImGui::TreeNode(("Teams"))) {
      for (auto &obj: this->state.teams) {
        drawReflectableObject(&obj, nullptr);
      }
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("MPlayer Objects")) {
      for (auto &obj: this->state.players) {
        drawReflectableObject(&obj, obj.uid.curr()->name);
      }
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Zones")) {
      for (auto obj: this->state.Zones) {
        if (obj->curr()) {
          //char buff[16];
          //fmt::format_to_n(buff, 16, "{:#x}", obj->getUID());
          drawReflectableObject(*obj->curr(), nullptr);
        }
      }
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Areas")) {
      for (auto obj : this->state.missionAreas2) {
        if (obj->curr()) {
          drawReflectableObject(*obj->curr(), nullptr);
        }
      }
      ImGui::TreePop();
    }
  }

  void drawECS() {
    //state.g_entity_mgr
  }

  template <typename T>
  void drawVar(const char * varname, const T & var, bool expanded_default = true) {
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(varname);
    ImGui::TableNextColumn();
    auto str = toStringImplTyped(&var, 0);
    drawStr(str, &var, varname, expanded_default);
  }

  void drawUnit() {
    
  }

  void drawUnits() {
    for (auto unit: state.getUnitLookup()) {
      if (unit) {
        char buff[64] = {};
        auto translated = translate::localize_index(unit->name_index_1);
        if (translated == nullptr)
          translated = "<UNKNOWN>";
        fmt::format_to_n(buff, 64, "uid {}; type: {}; name: {}", unit->uid, unit->getUnitTypeName(), translated);
        if (ImGui::TreeNode(buff)) {
          ImGui::BeginTable("Unit Data", 2, ImGuiTableFlags_Borders);

          ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed);
          ImGui::TableSetupColumn("value");
          ImGui::TableHeadersRow();
          auto shop_val = translate::localize_index(unit->name_index_shop);
          auto idx_0_val = translate::localize_index(unit->name_index_0);
          auto idx_1_val = translate::localize_index(unit->name_index_1);
          auto idx_2_val = translate::localize_index(unit->name_index_2);
          drawVar("uid", unit->uid);
          drawVar("created_at_ms", unit->created_at_ms);
          drawVar("killed_at_ms", unit->killed_at_ms);
          drawVar("killed_position", unit->killed_position);
          drawVar("destroyed_at_ms", unit->destroyed_at_ms);
          drawVar("curr_eid", unit->curr_eid);
          drawVar("unitType", unit->unitType);
          drawVar("unit_name", unit->unit_name);
          drawVar("raw_unit_name", unit->raw_unit_name);
          drawVar("shop_name: ", std::string_view(shop_val ? shop_val : "<UNKNOWN>"));
          drawVar("name_index_0: ", std::string_view(idx_0_val ? idx_0_val : "<UNKNOWN>"));
          drawVar("name_index_1: ", std::string_view(idx_1_val ? idx_1_val : "<UNKNOWN>"));
          drawVar("name_index_2: ", std::string_view(idx_2_val ? idx_2_val : "<UNKNOWN>"));
          ImGui::EndTable();
          ImGui::TreePop();
        }
      }
    }
  }

public:
  void runAll() {
    if (!this->state.finishedLoading()) {
      loadReplay();
      calculate_data();
    }
    ZoneScoped;
    if (ImGui::BeginChild("Rewind Time", ImVec2(0, 0), RESIZE_CHILD_FLAGS)) {
      ImGui::Text("Replay Length: %.02f seconds", (double) replay_length_f);
      if (this->state.finishedLoading()) {

        ImGui::SameLine();
        ImGui::TextUnformatted("Replay Time");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(125.0f);
        ImGui::InputFloat("##input_time", &current_time, 0.25f, replay_length_f);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(350.0f);
        ImGui::SliderFloat("##slider_time", &current_time, 5.0f, replay_length_f);
        if (play_video_thing) {
          current_time += ImGui::GetIO().DeltaTime * play_speed;
          if (current_time > this->replay_length_f)
            play_video_thing = false;
        }
        state.rewindToMs((uint32_t) (current_time * 1000));

        ImGui::SameLine();
        if (ImGui::Button(play_video_thing ? "Playing" : "Paused")) {
          play_video_thing = !play_video_thing;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Play speed");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputFloat("##input_time_speed", &play_speed, 0.5f, 16.f);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("##slider_time_speed", &play_speed, 0.5f, 16.f);
      }
      ImGui::EndChild();
    }
    if (ImGui::BeginTabBar("Replay Tabs", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("Map") && ImGui::BeginChild("##MapArea", ImVec2(0, 0), ImGuiChildFlags_None)) {
        drawMapTab();
        ImGui::EndChild();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Reflectable Objects") && ImGui::BeginChild("##ReflectableObjectsArea", ImVec2(0, 0), ImGuiChildFlags_None)) {
        drawReflectableObjects();
        ImGui::EndChild();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("ECS") && ImGui::BeginChild("##ECSArea", ImVec2(0, 0), ImGuiChildFlags_None)) {
        //drawECS();
        ImGui::EndChild();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Units") && ImGui::BeginChild("##UnitsArea", ImVec2(0, 0), ImGuiChildFlags_None)) {
        drawUnits();
        ImGui::EndChild();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
};
