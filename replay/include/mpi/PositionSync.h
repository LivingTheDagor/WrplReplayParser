#pragma once
#include "math/dag_Point3.h"

struct ParserState;

bool FMSync(ParserState &state, BitStream &bs);
bool GMSync(ParserState &state, BitStream &bs);
bool WeaponSync(ParserState &state, BitStream &bs);