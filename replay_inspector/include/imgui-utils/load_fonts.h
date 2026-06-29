

#ifndef WTFILEUTILS_LOAD_FONTS_H
#define WTFILEUTILS_LOAD_FONTS_H

const char *fallback_fonts[] = {
  "ttfs/symbols_skyquake.ttf",        "ttfs/rare_symbols.ttf", "ttfs/opensansemoji.ttf",
  "ttfs/mplus-2p-regular.subset.ttf", "ttfs/msyh.subset.ttf",  "ttfs/nanumgothicregular.subset.ttf",
};

void load_imgui_fonts() {
  // Call once during initialization, before building the atlas / creating device objects.
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->Clear();
  // 1) Load your main text font
  ImFontConfig cfg;
  cfg.OversampleH = cfg.OversampleV = 2;
  cfg.PixelSnapH = false;
  cfg.FontDataOwnedByAtlas = false;
  const float fontSizePx = 13.0f;
  auto main_font = file_mgr.getFile("ttfs/pt_sans-web-regular-reducedheight.ttf");
  G_ASSERT(main_font);

  auto spn = main_font->readRaw();
  ImFont *mainFont = io.Fonts->AddFontFromMemoryTTF((void *) spn.data(), (int) spn.size(), fontSizePx, &cfg,
                                                    io.Fonts->GetGlyphRangesDefault());

  // 2) Merge in the symbols font
  ImFontConfig symCfg;
  symCfg.MergeMode = true; // <-- this is the important part
  symCfg.PixelSnapH = false;
  symCfg.OversampleH = symCfg.OversampleV = 2;
  symCfg.FontDataOwnedByAtlas = false;

  // Choose glyph ranges for the symbols font.
  for (auto f_name: fallback_fonts) {
    auto extra_font = file_mgr.getFile(f_name);
    G_ASSERT(extra_font);
    spn = extra_font->readRaw();
    io.Fonts->AddFontFromMemoryTTF((void *) spn.data(), (int) spn.size(), fontSizePx, &symCfg, nullptr);
  }
  io.Fonts->Build();

  io.FontDefault = mainFont;
}

#endif // WTFILEUTILS_LOAD_FONTS_H
