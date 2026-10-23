# Replay parser regression test

Configure with `-DPARSER_ENABLE_TESTS=ON` and build the `lux_review_regression`
target. Run `ctest --test-dir build -R lux_review_regression --output-on-failure`
with `WRPL_TEST_GAME_PATH` set to the War Thunder installation directory. The
test skips when that variable is absent.

Set `WRPL_TEST_REPLAY_PATH` to a server replay directory containing a rocket
with a resolvable owner and weapon to include the owner-destruction check. The
test destroys an entity in its own in-memory parser state; it does not modify
the replay files.

Set `WRPL_TEST_EARLY_REPLAY_PATH` to server replay `127b004900163e25` to check
that the parser retains UID 400's sync at 84514 ms, attaches it to its ECS
creation at 84554 ms, and reaches the end marker. Without either replay
variable, only the synthetic packet cases run.
