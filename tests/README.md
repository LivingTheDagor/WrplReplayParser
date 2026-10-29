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
