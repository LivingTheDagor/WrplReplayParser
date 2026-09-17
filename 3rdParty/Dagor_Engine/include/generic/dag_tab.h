//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifdef __cplusplus

#include <generic/dag_span.h>
#include <dag/dag_vector.h>

template<typename T>
using Tab = dag::Vector<T, EASTLAllocatorType, false, uint32_t>;


#endif // __cplusplus
