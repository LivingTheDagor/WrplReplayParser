#pragma once

struct ParserState;

bool FMSync(ParserState &state, danet::BitStream &bs);
bool GMSync(ParserState &state, danet::BitStream &bs);
bool WeaponSync(ParserState &state, danet::BitStream &bs);
