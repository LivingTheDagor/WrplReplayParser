# WrplReplayParser
Warthunder .wrpl Replayer Parser written in cpp with python bindings

a feature filled wrpl replay parser that has python bindings (and will be publish on pypi soon)
this parser lets you access battle messages, chat messages, various global object data (like player information and team information), and entity construction information plus aircraft and tank positions (see FMSync and GMSync functions)
I have completed most of my inital goals, and will work to clean up the codebase as time goes on. this include reorganization / refactoring plus adding of probably sorely needed utility functions.

currently supported replay version: 18c1c (`0x00018c1c`). The parser rejects a replay with a different version and logs both versions.

This attempts to be buildable for:

Windows: MSYS2 minGW, MSVC

Linux gcc (native and WSL2)

## Building

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPARSER_ENABLE_PYTHON_BINDINGS=ON
cmake --build build
```

To build and install the Python module with pip, run `pip install .` in the repository directory.

The Python bindings need Python 3.12 (`PYTHON_VERSION_TO_USE`). Set `-DENABLE_LTO=OFF` on machines with little memory, because the LTO link uses a large quantity of memory.

## Tests

Configure with `-DPARSER_ENABLE_TESTS=ON`, then run `ctest --test-dir build --output-on-failure`. See [tests/README.md](tests/README.md) for each test.

# TODO

- [x] setup loading of char.vromfs.bin and proper translation code
- [ ] overhaul directory structure
- [ ] work on reducing certain memory usage, vromfs are rather expensive to load. maybe make a vromfs repackager?
- [ ] ADD COMMENTS
- [x] ensure LICENCE is correctly setup (the parser is BSD 3-Clause, see [LICENSE](LICENSE); the third-party code is in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md))
- [x] tests
- [ ] publish on pypi (`pyproject.toml` is ready, the upload is still to do)
