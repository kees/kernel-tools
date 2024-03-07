/* See Makefile for build flags. Lots and lots of build flags. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>

#include "harness.h"

/* Use bit width names to avoid going insane. */
typedef unsigned char	    u8;
typedef signed char	    s8;
typedef unsigned short	   u16;
typedef signed short	   s16;
typedef unsigned int	   u32;
typedef int		   s32;
typedef unsigned long long u64;
typedef long long	   s64;

#define U8_MAX		((u8)~0U)
#define S8_MAX		((s8)(U8_MAX>>1))
#define S8_MIN		((s8)(-S8_MAX - 1))
#define U16_MAX		((u16)~0U)
#define S16_MAX		((s16)(U16_MAX>>1))
#define S16_MIN		((s16)(-S16_MAX - 1))
#define U32_MAX		((u32)~0UL)
#define S32_MAX		((s32)(U32_MAX>>1))
#define S32_MIN		((s32)(-S32_MAX - 1))
#define U64_MAX		((u64)~0ULL)
#define S64_MAX		((s64)(U64_MAX>>1))
#define S64_MIN		((s64)(-S64_MAX - 1))

#define noinline __attribute__((__noinline__))

#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)

#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

#define __UNIQUE_ID(prefix) __PASTE(__PASTE(prefix, _), __COUNTER__)

#define FMTs8			"%d"
#define FMTs16			"%d"
#define FMTs32			"%d"
#define FMTs64			"%lld"

#define FMTu8			"%u"
#define FMTu16			"%u"
#define FMTu32			"%u"
#define FMTu64			"%llu"

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

#define no_sio		__attribute__((no_sanitize("signed-integer-overflow")))
#define no_uio		__attribute__((no_sanitize("unsigned-integer-overflow")))
#define no_po		__attribute__((no_sanitize("pointer-overflow")))
#define no_isit		__attribute__((no_sanitize("implicit-signed-integer-truncation")))
#define no_iuit		__attribute__((no_sanitize("implicit-unsigned-integer-truncation")))

/* To test a single sanitizer, disable all the others. */
#define UBSAN_CHECK_sio(x...)	TEST_SIGNAL(x, SIGILL)	\
				no_uio no_po no_isit no_iuit
#define UBSAN_CHECK_uio(x...)	TEST_SIGNAL(x, SIGILL)	\
				no_sio no_po no_isit no_iuit
#define UBSAN_CHECK_po(x...)	TEST_SIGNAL(x, SIGILL)	\
				no_sio no_uio no_isit no_iuit
#define UBSAN_CHECK_isit(x...)	TEST_SIGNAL(x, SIGILL)	\
				no_sio no_uio no_po no_iuit
#define UBSAN_CHECK_iuit(x...)	TEST_SIGNAL(x, SIGILL)	\
				no_sio no_uio no_po no_isit
#define UBSAN_CHECK_test(x...)	TEST(x)

#define REPORT_sio(x...)	TH_LOG(x)
#define REPORT_uio(x...)	TH_LOG(x)
#define REPORT_po(x...)		TH_LOG(x)
#define REPORT_isit(x...)	TH_LOG(x)
#define REPORT_iuit(x...)	TH_LOG(x)
#define REPORT_test(x...)	do { } while (0)

/* Not sure how to get decent test names generated... */
#define UBSAN_TEST(how, t0, t1, t1_init, op, t2, t2_init)		\
UBSAN_CHECK_ ## how(__UNIQUE_ID(how))					\
{									\
	t0 result;							\
	t1 var = (t1_init) + unconst;					\
	t2 offset = (t2_init) + unconst;				\
									\
	/* Since test names are trash, always display operation. */	\
	TH_LOG(" " #t0 " = " #t1 "(" fmt(t1) ") " oper_name(op) " " #t2 "(" fmt(t2) ")", var, offset); \
									\
	result = var oper(op) offset;					\
	REPORT_ ## how("Unexpectedly survived " #t0 " = " #t1 "(" fmt(t1) ") " oper_name(op) " " #t2 "(" fmt(t2) "): " fmt(t0), var, offset, result); \
}

/* Test a commutative operation (add, mul) */
#define UBSAN_COMMUT(how, t0, t1, t1_init, op, t2, t2_init)	\
	UBSAN_TEST(how, t0, t1, t1_init, op, t2, t2_init)	\
	UBSAN_TEST(how, t0, t2, t2_init, op, t1, t1_init)	\

#define LVALUE_S8_TESTS		\
	/* Something plus nothing, not gonna trap. */		\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, add, s8,       0)	\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, add, s16,      0)	\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, add, s32,      0)	\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, add, s64,      0)	\
	/* Something times 1, not gonna trap. */		\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, mul, s8,       1)	\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, mul, s16,      1)	\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, mul, s32,      1)	\
	UBSAN_COMMUT(test, s8, s8, S8_MAX, mul, s64,      1)	\
	/* These all all exceed S8_MAX, but don't exceed the s32 int promotion. */\
	/* -fsanitize=implicit-signed-integer-truncation */	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, add, s8,       3)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, add, s16,      3)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, add, s32,      3)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, add, s64,      3)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, mul, s8,       2)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, mul, s16,      2)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, mul, s32,      2)	\
	UBSAN_COMMUT(isit, s8, s8, S8_MAX, mul, s64,      2)

#define LVALUE_S16_TESTS		\
	/* Something plus nothing, not gonna trap. */		\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, add, s8,      0)	\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, add, s16,     0)	\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, add, s32,     0)	\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, add, s64,     0)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, add, s8,    0)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, add, s16,   0)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, add, s32,   0)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, add, s64,   0)	\
	/* Something times 1, not gonna trap. */		\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, mul, s8,      1)	\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, mul, s16,     1)	\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, mul, s32,     1)	\
	UBSAN_COMMUT(test, s16, s8, S8_MAX, mul, s64,     1)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, mul, s8,    1)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, mul, s16,   1)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, mul, s32,   1)	\
	UBSAN_COMMUT(test, s16, s16, S16_MAX, mul, s64,   1)	\
	/* These all all exceed S16_MAX, but don't exceed the s32 int promotion. */\
	/* -fsanitize=implicit-signed-integer-truncation */	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, add, s8,    3)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, add, s16,   3)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, add, s32,   3)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, add, s64,   3)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, mul, s8,    2)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, mul, s16,   2)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, mul, s32,   2)	\
	UBSAN_COMMUT(isit, s16, s16, S16_MAX, mul, s64,   2)

#define LVALUE_S32_TESTS		\
	/* Something plus nothing, not gonna trap. */		\
	UBSAN_COMMUT(test, s32, s8, S8_MAX, add, s8,      0)	\
	UBSAN_COMMUT(test, s32, s8, S8_MAX, add, s16,     0)	\
	UBSAN_COMMUT(test, s32, s8, S8_MAX, add, s32,     0)	\
	UBSAN_COMMUT(test, s32, s8, S8_MAX, add, s64,     0)	\
	UBSAN_COMMUT(test, s32, s16, S16_MAX, add, s8,    0)	\
	UBSAN_COMMUT(test, s32, s16, S16_MAX, add, s16,   0)	\
	UBSAN_COMMUT(test, s32, s16, S16_MAX, add, s32,   0)	\
	UBSAN_COMMUT(test, s32, s16, S16_MAX, add, s64,   0)	\
	UBSAN_COMMUT(test, s32, s32, S32_MAX, add, s8,    0)	\
	UBSAN_COMMUT(test, s32, s32, S32_MAX, add, s16,   0)	\
	UBSAN_COMMUT(test, s32, s32, S32_MAX, add, s32,   0)	\
	UBSAN_COMMUT(test, s32, s32, S32_MAX, add, s64,   0)	\


#define UBSAN_TESTS		\
	LVALUE_S8_TESTS		\
	LVALUE_S16_TESTS	\
	LVALUE_S32_TESTS	\

UBSAN_TESTS

TEST_HARNESS_MAIN
