# Parser tests

Configure with `-DPARSER_ENABLE_TESTS=ON`, build, then run
`ctest --test-dir build --output-on-failure`.

## bitstream_tests

Needs no game data. It writes streams in memory and reads them back: bit reads
at each offset, compressed values, nested streams, strings, and the
`IdFieldSerializer255` and `IdFieldSerializer32` field tables. It also cuts or
changes each stream and makes sure that the read fails and does not go past the
end of the data.

## logger_tests

Needs no game data. It makes sure that a log call or a failed check before the
log handler exists does not recurse (issue #12). An error goes to stderr, and a
failed check logs one time. Then it throws `AssertException` in a release
build, or exits with code 1 in a debug build (`LDAG_DBGLEVEL=1`). The debug
case runs in a child process, so it needs a POSIX system.

## packet_regression_tests

Set `WRPL_TEST_GAME_PATH` to the War Thunder installation directory. The test
skips when that variable is absent.

Set `WRPL_TEST_REPLAY_PATH` to a server replay directory containing a rocket
with a resolvable owner and weapon to include the owner-destruction check. The
test destroys an entity in its own in-memory parser state; it does not modify
the replay files.

Set `WRPL_TEST_EARLY_REPLAY_PATH` to server replay `127b004900163e25` to check
that the parser retains UID 400's sync at 84514 ms, attaches it to its ECS
creation at 84554 ms, and reaches the end marker. Without either replay
variable, only the synthetic packet cases run.
