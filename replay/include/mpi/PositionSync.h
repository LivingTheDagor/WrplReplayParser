#pragma once

struct ParserState;

bool FMSync(ParserState &state, BitStream &bs);
bool GMSync(ParserState &state, BitStream &bs);
bool WeaponSync(ParserState &state, BitStream &bs);