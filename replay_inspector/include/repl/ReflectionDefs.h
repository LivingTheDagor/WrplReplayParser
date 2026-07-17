#pragma once
#include "mpi/serializers.h"
#include "mpi/codegen/ReflIncludes.h"

class DrawHistory : public IRenderHandler {
public:
  void onEnd() override {
      ImGui::EndTable();
  }
  uint32_t curr_index, total;
  bool setCount(uint32_t count, uint32_t curr_offs) override {
    curr_index = curr_offs;
    total = count;
    ImGui::Text("Total: %i; current: %i", total, curr_index);
    if (!ImGui::BeginTable("History Table", 3, ImGuiTableFlags_Borders))
      return false;
    ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();
    return true;
  };
  void DrawIndex(uint32_t idx, uint32_t time_ms, std::string &value) override {
    ImGui::TableNextColumn();
    ImGui::Text("%i", idx);
    ImGui::TableNextColumn();
    ImGui::Text("%.3f", time_ms/1000.f);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value.c_str());
  }
};

void danet::ReflectableObject::drawObject() const {
  for (auto var = varList.head; var; var = var->next) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    char buff[40];
    fmt::format_to_n(buff, 40, "History##{}{}", fmt::ptr(this), var->name);
    if (ImGui::Button(buff)) {
      var->isOpen = !var->isOpen; // toggles the state based on the button pressed, QOL
      if (var->isOpen) {
        float base_h = ImGui::GetMainViewport()->WorkSize.y;
        float default_h = base_h - 20.0f;
        ImGui::SetNextWindowSize(ImVec2(0, default_h));
      }
    }
    if (var->isOpen) {
      char popup_name[256] = {};
      fmt::format_to_n(popup_name, 256, "History for {}: {}", getClassName(), var->name);
      ImGui::Begin(popup_name, &var->isOpen);
      DrawHistory history{};
      var->DrawHistory(&history);
      ImGui::End();
    }
     /* char popup_name[256] = {};
      fmt::format_to_n(popup_name, 256, "History for {}: {}", getClassName(), var->name);
      ImGui::Begin(popup_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::BeginTable("History Table", 3, ImGuiTableFlags_Borders);
      ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Value");
    }*/
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(var->name);
    ImGui::TableNextColumn();
    auto str = var->toString();
    if (str.contains('\n')) {
      char buff[40];
      fmt::format_to_n(buff, 40, "##{}{}", fmt::ptr(this), var->name);
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);
      if (ImGui::TreeNode(buff)) {
        ImGui::TextUnformatted(str.c_str());
        ImGui::TreePop();
      }
    } else {
    ImGui::TextUnformatted(str.c_str());
    }
  }
}
void MPlayer::drawObject() const {ReflectableObject::drawObject();}
void TeamData::drawObject() const {ReflectableObject::drawObject();}
void GlobalElo::drawObject() const {ReflectableObject::drawObject();}
void GeneralState::drawObject() const {ReflectableObject::drawObject();}
void MissionArea::drawObject() const {ReflectableObject::drawObject();}
void MissionZone::drawObject() const {ReflectableObject::drawObject();}
void BombingZone::drawObject() const {ReflectableObject::drawObject();}
void CaptureZone::drawObject() const {ReflectableObject::drawObject();}
void RearmZone::drawObject() const {ReflectableObject::drawObject();}
void ExitZone::drawObject() const {ReflectableObject::drawObject();}
void PickupZone::drawObject() const {ReflectableObject::drawObject();}
void BaseExtReflectable::drawObject() const {ReflectableObject::drawObject();}
void FMWReflectable::drawObject() const {ReflectableObject::drawObject();}
void GMReflectable::drawObject() const {ReflectableObject::drawObject();}
void DVMReflectable::drawObject() const {ReflectableObject::drawObject();}
void FM_DVMReflectable::drawObject() const {ReflectableObject::drawObject();}
void GM_DVMReflectable::drawObject() const {ReflectableObject::drawObject();}
void UnitWeaponsMask::drawObject() const {ReflectableObject::drawObject();}