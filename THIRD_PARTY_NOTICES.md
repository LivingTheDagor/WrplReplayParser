# Third-party code

The parser is BSD 3-Clause (see [LICENSE](LICENSE)). The code below keeps its own license. Each directory in
`3rdParty` holds the full license text, except where the table says otherwise.

## In this repository

| Directory | Project | License |
|---|---|---|
| `3rdParty/Dagor_Engine` | Dagor Engine (Gaijin Entertainment) | BSD 3-Clause |
| `3rdParty/eastl` | EASTL (Electronic Arts) | BSD 3-Clause |
| `3rdParty/fmt-12.1.0` | {fmt} | MIT |
| `3rdParty/cpptrace-1.0.4` | cpptrace | MIT |
| `3rdParty/mimalloc-3.3.2` | mimalloc (Microsoft) | MIT |
| `3rdParty/enet` | ENet | MIT |
| `3rdParty/hash` | MurmurHash3 (MIT, in `LICENSE`), xxHash, wyhash, mum-hash, MD5, CRC32, CRC32C, Rijndael | each other file has its license in its header |
| `3rdParty/libdeflate` | libdeflate | MIT |
| `3rdParty/libunwind-1.8.3` | libunwind | MIT |
| `3rdParty/compile-time-regular-expressions-3.10.0` | CTRE | Apache 2.0 |
| `3rdParty/boost` | Boost.Describe, Boost.Mp11 | Boost Software License 1.0 (in the file headers) |
| `3rdParty/tinyfiledialogs` | tinyfiledialogs | zlib (in the file header) |
| `3rdParty/arc/zlib-ng-2.1.2` | zlib-ng | zlib |
| `3rdParty/arc/zstd-1.4.5` | Zstandard | BSD |
| `3rdParty/arc/lz4` | LZ4 | BSD 2-Clause |
| `3rdParty/arc/fastlz` | FastLZ | MIT |
| `3rdParty/arc/lzma-9.20` | LZMA SDK | public domain |
| `3rdParty/arc/unzip` | minizip | zlib (minizip is part of zlib) |
| `3rdParty/regExp` | Digital Mars regular expressions | no license ("All Rights Reserved"). The build does not use it. |

## Downloaded at configure time

| Project | When | License |
|---|---|---|
| pybind11 | `PARSER_ENABLE_PYTHON_BINDINGS` | BSD 3-Clause |
| Tracy | always (the profiler is off unless `TRACY_ENABLE`) | BSD 3-Clause |
| libdwarf | cpptrace on Linux | LGPL 2.1. It is linked statically into the binaries. |
| SDL2, Dear ImGui | `PARSER_ENABLE_INSPECTOR` | zlib, MIT |
| curl | `PARSER_ENABLE_INSPECTOR_CURL` | curl (MIT style) |

On Linux the binaries also link the system `libunwind` and `liblzma`.
