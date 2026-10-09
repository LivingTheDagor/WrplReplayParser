#pragma once
#include <cstddef>
#include <cstdint>
#define PULL_VAR(x)      file_pull_##x
#define DECL_PULL_VAR(x) extern const size_t file_pull_##x
#define DEF_PULL_VAR(x)  extern const size_t file_pull_##x = (size_t) (&file_pull_##x)
