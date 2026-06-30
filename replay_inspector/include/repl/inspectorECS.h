#pragma once
#include "callback.h"
#include "imgui.h"

void iterate_all_tanks(ParserState &state, on_unit_cb_t &on_unit);

void iterate_all_aircraft(ParserState &state, on_unit_cb_t &on_unit);

void iterate_all_rockets(ParserState &state, on_rocket_cb_t &on_weapon);

void iterate_all_bombs(ParserState &state, on_bomb_cb_t &on_weapon);

const char *get_unit_name_1(const unit::Unit &unit);

ImVec2 get_longest_unit_name(ParserState &state);
