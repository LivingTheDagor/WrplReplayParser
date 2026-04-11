
#include "Unit.h"
#include "idFieldSerializer.h"
#include "ecs/ComponentTypesDefs.h"

namespace unit {
  Tank *Unit::AsTank() {
    if(unitType == TankType)
      return (Tank*)this;
    return nullptr;
  }

  Aircraft *Unit::AsAircraft() {
    if(unitType == AircraftType)
      return (Aircraft*)this;
    return nullptr;
  }

  bool LoadFromStorage(Unit *unit, FieldSerializerDict *data) {
    BitStream bs{data->data.data(), data->data.size(), false};
    IdFieldSerializer255 IdFieldSerializer{};
    uint32_t end;
    uint32_t count = IdFieldSerializer.readFieldsSizeAndCount(bs, end);
    if (!IdFieldSerializer.readFieldsIndex(bs)) {
      return false;
    }

    for (uint16_t i = 0; i < count; ++i) {
      auto fieldId = IdFieldSerializer.getFieldId(i);
      auto f_size_bits = IdFieldSerializer.getFieldSize(i);
    }
    return true;
  }
}

