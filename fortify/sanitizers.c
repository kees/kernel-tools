/* Build with -Wall -O2 -fno-strict-overflow -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=pointer-overflow */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>

#include "harness.h"


/* To avoid spaces in function names, typedef the types with spaces. */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int uint;
typedef unsigned long ulong;

#define U8_MAX		((u8)~0U)
#define S8_MAX		((s8)(U8_MAX>>1))
#define S8_MIN		((s8)(-S8_MAX - 1))
#define U16_MAX		((u16)~0U)
#define S16_MAX		((s16)(U16_MAX>>1))
#define S16_MIN		((s16)(-S16_MAX - 1))

#define noinline __attribute__((__noinline__))

#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)

#define FMTs8			"%d"
#define FMTs16			"%d"
#define FMTint			"%d"
#define FMTlong			"%ld"

#define FMTu8			"%u"
#define FMTu16			"%u"
#define FMTuint			"%u"
#define FMTulong		"%lu"

#define fmt(type)		FMT ## type

#define	OPERadd			+
#define	OPERsub			-
#define	OPERmul			*
#define OPER_NAMEadd		"+"
#define OPER_NAMEsub		"-"
#define OPER_NAMEmul		"*"

#define oper(op)		OPER ## op
#define oper_name(op)		OPER_NAME ## op

/* Used to stop optimizer from seeing constant expressions. */
volatile int unconst = 0;
volatile int debug = 0;

#define REPORT_OKAY(x...)	if (debug) TH_LOG(x)
#define REPORT_TRAP(x...)	TH_LOG(x)

#define UBSAN_CHECK_OKAY(x...)	TEST(x)
#define UBSAN_CHECK_TRAP(x...)	TEST_SIGNAL(x, SIGILL)

#define UBSAN_TEST(how, t0, t1, t1_init, op, t2, t2_init)		\
UBSAN_CHECK_ ## how(t0 ## _eq_ ## t1 ## _ ## op ## _ ## t2)		\
{									\
	t0 result;							\
	t1 var = (t1_init) + unconst;					\
	t2 offset = (t2_init) + unconst;				\
									\
	result = var oper(op) offset;					\
	REPORT_ ## how("Unexpectedly survived " #t0 " = " #t1 "(" fmt(t1) ") " oper_name(op) " " #t2 "(" fmt(t2) "): " fmt(t0), var, offset, result); \
}

#define LVALUE_S8_TESTS		\
	UBSAN_TEST(TRAP, s8, s8, S8_MAX, add, s8,        3)	\
	UBSAN_TEST(TRAP, s8, s8, S8_MAX, add, s16,       3)	\
	UBSAN_TEST(TRAP, s8, s8, S8_MAX, add, int,       3)	\
	UBSAN_TEST(TRAP, s8, s8, S8_MAX, add, long,      3)	\
	UBSAN_TEST(TRAP, s8, s8,      3, add, s8,   S8_MAX)	\
	UBSAN_TEST(TRAP, s8, s16,     3, add, s8,   S8_MAX)	\
	UBSAN_TEST(TRAP, s8, int,     3, add, s8,   S8_MAX)	\
	UBSAN_TEST(TRAP, s8, long,    3, add, s8,   S8_MAX)	\
								\
/*
	UBSAN_TEST(TRAP, s8, s16,   S16_MAX, add)		\
	UBSAN_TEST(TRAP, s8, int,   INT_MAX, add)		\
	UBSAN_TEST(TRAP, s8, long, LONG_MAX, add)		\
	UBSAN_TEST(TRAP, s8, u8,     U8_MAX, add)		\
	UBSAN_TEST(TRAP, s8, u16,   U16_MAX, add)		\
	UBSAN_TEST(TRAP, s8, uint,   UINT_MAX, add)		\
	UBSAN_TEST(TRAP, s8, ulong, ULONG_MAX, add)		\
								\
	UBSAN_TEST(TRAP, s8, s8,     S8_MIN, sub)		\
	UBSAN_TEST(TRAP, s8, s16,   S16_MIN, sub)		\
	UBSAN_TEST(TRAP, s8, int,   INT_MIN, sub)		\
	UBSAN_TEST(TRAP, s8, long, LONG_MIN, sub)		\
	UBSAN_TEST(OKAY, s8, u8,          0, sub)		\
	UBSAN_TEST(OKAY, s8, u16,         0, sub)		\
	UBSAN_TEST(OKAY, s8, uint,        0, sub)		\
	UBSAN_TEST(OKAY, s8, ulong,       0, sub)		\
								\
	UBSAN_TEST(TRAP, s8, s8,     S8_MAX / 2, mul)		\
	UBSAN_TEST(TRAP, s8, s16,   S16_MAX / 2, mul)		\
	UBSAN_TEST(TRAP, s8, int,   INT_MAX / 2, mul)		\
	UBSAN_TEST(TRAP, s8, long, LONG_MAX / 2, mul)

#define LVALUE_U8_TESTS		\
	RHS_TESTS(TRAP, u8, u8,      S8_MAX, add)		\
	RHS_TESTS(TRAP, u8, u16,    S16_MAX, add)		\
	RHS_TESTS(TRAP, u8, uint,   INT_MAX, add)		\
	RHS_TESTS(TRAP, u8, ulong, LONG_MAX, add)		\
	RHS_TESTS(TRAP, u8, u8,      S8_MIN, sub)		\
	RHS_TESTS(TRAP, u8, u16,    S16_MIN, sub)		\
	RHS_TESTS(TRAP, u8, uint,   INT_MIN, sub)		\
	RHS_TESTS(TRAP, u8, ulong, LONG_MIN, sub)		\
	RHS_TESTS(TRAP, u8, u8,       U8_MAX / 2, mul)		\
	RHS_TESTS(TRAP, u8, u16,     U16_MAX / 2, mul)		\
	RHS_TESTS(TRAP, u8, uint,   UINT_MAX / 2, mul)		\
	RHS_TESTS(TRAP, u8, ulong, ULONG_MAX / 2, mul)
*/

#define UBSAN_TESTS		\
	LVALUE_S8_TESTS

UBSAN_TESTS

TEST_HARNESS_MAIN
