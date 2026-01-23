
#pragma once
#include "reflection.h"
#include "DataBlock.h"
#include "ecs/entityId.h"
#include "mpi/types.h"
#include "mpi/codegen/serializers.h"

namespace danet {

  int TranslatedCoder(DANET_ENCODER_SIGNATURE);

}

