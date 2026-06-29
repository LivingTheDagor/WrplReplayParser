#pragma once


#define ECS_EXPAND(x) x

#define ECS_FOR_EACH_1(WHAT, X)       WHAT(X)
#define ECS_FOR_EACH_2(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_1(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_3(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_2(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_4(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_3(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_5(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_4(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_6(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_5(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_7(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_6(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_8(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_7(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_9(WHAT, X, ...)  WHAT(X) ECS_EXPAND(ECS_FOR_EACH_8(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_10(WHAT, X, ...) WHAT(X) ECS_EXPAND(ECS_FOR_EACH_9(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_11(WHAT, X, ...) WHAT(X) ECS_EXPAND(ECS_FOR_EACH_10(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_12(WHAT, X, ...) WHAT(X) ECS_EXPAND(ECS_FOR_EACH_11(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_13(WHAT, X, ...) WHAT(X) ECS_EXPAND(ECS_FOR_EACH_12(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_14(WHAT, X, ...) WHAT(X) ECS_EXPAND(ECS_FOR_EACH_13(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH_15(WHAT, X, ...) WHAT(X) ECS_EXPAND(ECS_FOR_EACH_14(WHAT, __VA_ARGS__))
//... repeat as needed
#define ECS_FOR_EACH_NARG(...)        ECS_FOR_EACH_NARG_(__VA_ARGS__, ECS_FOR_EACH_RSEQ_N())
#define ECS_FOR_EACH_NARG_(...)       ECS_EXPAND(ECS_FOR_EACH_ARG_N(__VA_ARGS__))

#define ECS_FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, N, ...) N

#define ECS_FOR_EACH_RSEQ_N()       15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define ECS_FOR_EACH_(N, WHAT, ...) ECS_EXPAND(CONCATENATE(ECS_FOR_EACH_, N)(WHAT, __VA_ARGS__))
#define ECS_FOR_EACH(WHAT, ...)     ECS_FOR_EACH_(ECS_FOR_EACH_NARG(__VA_ARGS__), WHAT, __VA_ARGS__)
#ifdef _ECS_CODEGEN
#define ECS_BEFORE_ONE(a) __attribute__((annotate("@before:" #a)))
#define ECS_BEFORE(...)   ECS_FOR_EACH(ECS_BEFORE_ONE, __VA_ARGS__)
#define ECS_AFTER_ONE(a)  __attribute__((annotate("@after:" #a)))
#define ECS_AFTER(...)    ECS_FOR_EACH(ECS_AFTER_ONE, __VA_ARGS__)
#define ECS_TAG_ONE(a)    __attribute__((annotate("@tag:" #a)))
#define ECS_TAG(...)      ECS_FOR_EACH(ECS_TAG_ONE, __VA_ARGS__)
#define ECS_TRACK_ONE(a)  __attribute__((annotate("@track:" #a)))
#define ECS_TRACK(...)    ECS_FOR_EACH(ECS_TRACK_ONE, __VA_ARGS__)
#define ECS_NO_ORDER      __attribute__((annotate("@before:*")))

#define ECS_UNUSED(a)          a
#define ECS_REQUIRE_ONE(a)     __attribute__((annotate("@require:" #a)))
#define ECS_REQUIRE(...)       ECS_FOR_EACH(ECS_REQUIRE_ONE, __VA_ARGS__)
#define ECS_REQUIRE_NOT_ONE(a) __attribute__((annotate("@require_not:" #a)))
#define ECS_REQUIRE_NOT(...)               ECS_FOR_EACH(ECS_REQUIRE_NOT_ONE, __VA_ARGS__
#else
#define ECS_BEFORE_ONE(a)
#define ECS_BEFORE(...)
#define ECS_AFTER_ONE(a)
#define ECS_AFTER(...)
#define ECS_TAG_ONE(a)
#define ECS_TAG(...)
#define ECS_TRACK_ONE(a)
#define ECS_TRACK(...)
#define ECS_NO_ORDER

#define ECS_UNUSED(a) a
#define ECS_REQUIRE_ONE(a)
#define ECS_REQUIRE(...)
#define ECS_REQUIRE_NOT_ONE(a)
#define ECS_REQUIRE_NOT(...)
#endif
