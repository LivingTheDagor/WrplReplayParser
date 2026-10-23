# WrplReplayParser
Warthunder .wrpl Replayer Parser written in cpp with python bindings

a feature filled wrpl replay parser that has python bindings (and will be publish on pypi soon)
this parser lets you access battle messages, chat messages, various global object data (like player information and team information), and entity construction information plus aircraft and tank positions (see FMSync and GMSync functions)
I have completed most of my inital goals, and will work to clean up the codebase as time goes on. this include reorganization / refactoring plus adding of probably sorely needed utility functions.

currently supported replay version: 18c0b

This attempts to be buildable for:

Windows: MSYS2 minGW, MSVC

Linux (WSL2) gcc

## Aircraft syncs before entity creation

Some replays contain an aircraft sync before the corresponding ECS unit is created. The parser retains that sync and attaches it to the next aircraft creation with the same low 11-bit UID. Until then, the aircraft has `provisional == True`; its UID, positions and enabled event data are available, but its identity and storage details are not yet known. `ParserState.pending_aircraft` lists aircraft still waiting for creation, including any whose creation never appears. The sync does not provide a confirmed incarnation identifier, so a pending aircraft is attached only if its last sync is less than 5 s before the creation; in 126 replays the creation came 20 to 223 ms after the first early sync. An older pending aircraft stays provisional in `pending_aircraft`. `collect_all_units()` lists only units the ECS created. These properties describe what has been loaded, and do not change when the parser is rewound. A malformed aircraft packet still causes `LoadFromReader` to raise `RuntimeError`, with its partial aircraft updates discarded.

# TODO

setup loading of char.vromfs.bin and proper translation code

overhaul directory structure

work on reducing certain memory usage, vromfs are rather expensive to load. maybe make a vromfs repackager?

ADD COMMENTS

ensure LICENCE is correctly setup

tests
