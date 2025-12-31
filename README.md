# WrplReplayParser
Warthunder .wrpl Replayer Parser written in cpp with planned ABI and python bindings

this is currently me getting the base cpp implementation up, for those who have used my python parser, this is has less features (in certain areas) but is much faster

example for the replay parser is found in experiment/dagor_replay_test/TestLoadingReplay.cpp

Due to any changes Gaijin may change to the replay format, the Replay Parser will only attempt to be compatable with the most recent non-dev release of warthunder
# TODO
setup abi

setup python bindings

while creating the abi, determine how units will be handled

work on new blk internals. this one will be a mix of a more advanced version of the current one plus a 'static' version that is expressly for loading a binary datablock with speed. will be a static version. idea derived from gaijin

setup chat handling

setup Battle message parsing and attempt to do better parse that python build

setup loading of char.vromfs.bin and proper translation code

overhaul directory structure

ADD COMMENTS

ensure LICENCE is correctly setup

# GOALS

the ability to dynamically feed the parser a packet through the abi, and examine the state of the replay at that specific point. no rewinding.

the ability to interact with any entity and get the components of any entity through the abi. 

the ability to read both chat and battle log messages, with battle logs being translatable.

the ability to see the global states through the mpi, maybe look into code generation for these? the Reflectables are fairly simple structs.
