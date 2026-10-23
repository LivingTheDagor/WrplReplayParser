
#pragma once
#include "Module.h"
#include <vector>
#include "mpi/SensorStates.h"

/// A ground weapon with its rounds copied out of ParserState::GroundRounds, for Python.
struct PyGroundWeapon {
  mpi::GroundWeaponSync w;
  std::vector<uint16_t> rounds;
};

class PyBattleMessages : protected Module {
public:
  PyBattleMessages() : Module() {}
  void include(py::module_ &m);
};

extern PyBattleMessages py_battle_messages;
