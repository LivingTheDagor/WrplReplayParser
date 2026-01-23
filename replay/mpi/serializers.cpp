#include "mpi/serializers.h"

namespace danet {

  int TranslatedCoder(DANET_ENCODER_SIGNATURE) {
    return stringCoder(op, meta, ro, bs);
  }
}