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
typedef void *		   ptr;

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

#if __has_attribute(wraps)
# define __wraps		__attribute__((wraps))
# define CHECK_WRAPS_ATTR	true
#else
# define __wraps		/**/
# define CHECK_WRAPS_ATTR	false
#endif

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

#define FMTptr			"%p"

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
#define UBSAN_trap_CHECK_sio(x...)	TEST_SIGNAL(x, SIGILL)	\
					no_uio no_po no_isit no_iuit
#define UBSAN_trap_CHECK_uio(x...)	TEST_SIGNAL(x, SIGILL)	\
					no_sio no_po no_isit no_iuit
#define UBSAN_trap_CHECK_po(x...)	TEST_SIGNAL(x, SIGILL)	\
					no_sio no_uio no_isit no_iuit
#define UBSAN_trap_CHECK_isit(x...)	TEST_SIGNAL(x, SIGILL)	\
					no_sio no_uio no_po no_iuit
#define UBSAN_trap_CHECK_iuit(x...)	TEST_SIGNAL(x, SIGILL)	\
					no_sio no_uio no_po no_isit
#define UBSAN_trap_CHECK_none(x...)	TEST_SIGNAL(x, SIGILL) \
					no_sio no_uio no_po no_isit no_iuit
#define UBSAN_trap_CHECK_any(x...)	TEST_SIGNAL(x, SIGILL)
#define UBSAN_trap_CHECK_survive(x...)	TEST(x)

/* Check that with a sanitizer enabled, there is no trap. */
#define UBSAN_survive_CHECK_sio(x...)	TEST(x)	\
					no_uio no_po no_isit no_iuit
#define UBSAN_survive_CHECK_uio(x...)	TEST(x)	\
					no_sio no_po no_isit no_iuit
#define UBSAN_survive_CHECK_po(x...)	TEST(x)	\
					no_sio no_uio no_isit no_iuit
#define UBSAN_survive_CHECK_isit(x...)	TEST(x)	\
					no_sio no_uio no_po no_iuit
#define UBSAN_survive_CHECK_iuit(x...)	TEST(x)	\
					no_sio no_uio no_po no_isit
#define UBSAN_survive_CHECK_none(x...)	TEST(x) \
					no_sio no_uio no_po no_isit no_iuit
#define UBSAN_survive_CHECK_any(x...)	TEST(x)
#define UBSAN_survive_CHECK_survive(x...) TEST(x)

#define REPORT_sio(x...)	TH_LOG(x)
#define REPORT_uio(x...)	TH_LOG(x)
#define REPORT_po(x...)		TH_LOG(x)
#define REPORT_isit(x...)	TH_LOG(x)
#define REPORT_iuit(x...)	TH_LOG(x)
#define REPORT_none(x...)	TH_LOG(x)
#define REPORT_any(x...)	TH_LOG(x)
#define REPORT_survive(x...)	if (debug) TH_LOG(x)

/* We cannot touch a NULL pointer at all, even by 0. */
#define UNCONST_sio(x...)	(x) + unconst
#define UNCONST_uio(x...)	(x) + unconst
#define UNCONST_po(x...)	(x)
#define UNCONST_isit(x...)	(x) + unconst
#define UNCONST_iuit(x...)	(x) + unconst
#define UNCONST_none(x...)	(x) + unconst
#define UNCONST_any(x...)	(x) + unconst
#define UNCONST_survive(x...)	(x) + unconst

/* Not sure how to get decent test names generated... */
#define UBSAN_trap_TEST(how, t0, t1, t1_init, op, t2, t2_init)		\
UBSAN_trap_CHECK_ ## how(__UNIQUE_ID(how))				\
{									\
	t0 result;							\
	t1 var = UNCONST_ ## how(t1_init);				\
	t2 offset = UNCONST_ ## how(t2_init);				\
									\
	/* Since test names are trash, always display operation. */	\
	TH_LOG(" (expected to trap) " #t0 " = " #t1 "(" fmt(t1) ") " oper_name(op) " " #t2 "(" fmt(t2) ")", var, offset); \
									\
	result = var oper(op) offset;					\
	REPORT_ ## how("Unexpectedly survived " #t0 " = " #t1 "(" fmt(t1) ") " oper_name(op) " " #t2 "(" fmt(t2) "): " fmt(t0), var, offset, result); \
}

#define UBSAN_survive_TEST(how, t0, t1, t1_init, op, t2, t2_init)	\
UBSAN_survive_CHECK_ ## how(__UNIQUE_ID(how))				\
{									\
	t0 result;							\
	t1 var = UNCONST_ ## how(t1_init);				\
	t2 offset = UNCONST_ ## how(t2_init);				\
	t1 var_wrap __wraps = UNCONST_ ## how(t1_init);			\
	t2 offset_wrap __wraps = UNCONST_ ## how(t2_init);		\
									\
	if (!CHECK_WRAPS_ATTR)						\
		SKIP(return, "'wraps' attribute not supported");	\
									\
	/* Since test names are trash, always display operation. */	\
	TH_LOG(" (no trap: wrapping expected) " #t0 " = " #t1 "(" fmt(t1) ") " oper_name(op) " " #t2 "(" fmt(t2) ")", var, offset); \
									\
	/* All of these should be survivable. */			\
	result = var_wrap oper(op) offset_wrap;				\
	EXPECT_TRUE(true) { TH_LOG(fmt(t0), result); }			\
	result = var_wrap oper(op) offset;				\
	EXPECT_TRUE(true) { TH_LOG(fmt(t0), result); }			\
	result = var oper(op) offset_wrap;				\
	EXPECT_TRUE(true) { TH_LOG(fmt(t0), result); }			\
}

#define UBSAN_TEST(how, t0, t1, t1_init, op, t2, t2_init)		\
	UBSAN_trap_TEST(how, t0, t1, t1_init, op, t2, t2_init)		\
	UBSAN_survive_TEST(how, t0, t1, t1_init, op, t2, t2_init)

/* Test a commutative operation (add, mul) */
#define UBSAN_COMMUT(how, t0, t1, t1_init, op, t2, t2_init)	\
	UBSAN_TEST(how, t0, t1, t1_init, op, t2, t2_init)	\
	UBSAN_TEST(how, t0, t2, t2_init, op, t1, t1_init)	\

#define LVALUE_S8_TESTS		\
	/* Something plus nothing, not gonna trap. */		\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, add, s8,       0)	\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, add, s16,      0)	\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, add, s32,      0)	\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, add, s64,      0)	\
	/* Something times 1, not gonna trap. */		\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, mul, s8,       1)	\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, mul, s16,      1)	\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, mul, s32,      1)	\
	UBSAN_COMMUT(survive, s8, s8, S8_MAX, mul, s64,      1)	\
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
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, add, s8,      0)	\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, add, s16,     0)	\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, add, s32,     0)	\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, add, s64,     0)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, add, s8,    0)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, add, s16,   0)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, add, s32,   0)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, add, s64,   0)	\
	/* Something times 1, not gonna trap. */		\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, mul, s8,      1)	\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, mul, s16,     1)	\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, mul, s32,     1)	\
	UBSAN_COMMUT(survive, s16, s8, S8_MAX, mul, s64,     1)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, mul, s8,    1)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, mul, s16,   1)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, mul, s32,   1)	\
	UBSAN_COMMUT(survive, s16, s16, S16_MAX, mul, s64,   1)	\
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
	UBSAN_COMMUT(survive, s32, s8, S8_MAX, add, s8,      0)	\
	UBSAN_COMMUT(survive, s32, s8, S8_MAX, add, s16,     0)	\
	UBSAN_COMMUT(survive, s32, s8, S8_MAX, add, s32,     0)	\
	UBSAN_COMMUT(survive, s32, s8, S8_MAX, add, s64,     0)	\
	UBSAN_COMMUT(survive, s32, s16, S16_MAX, add, s8,    0)	\
	UBSAN_COMMUT(survive, s32, s16, S16_MAX, add, s16,   0)	\
	UBSAN_COMMUT(survive, s32, s16, S16_MAX, add, s32,   0)	\
	UBSAN_COMMUT(survive, s32, s16, S16_MAX, add, s64,   0)	\
	UBSAN_COMMUT(survive, s32, s32, S32_MAX, add, s8,    0)	\
	UBSAN_COMMUT(survive, s32, s32, S32_MAX, add, s16,   0)	\
	UBSAN_COMMUT(survive, s32, s32, S32_MAX, add, s32,   0)	\
	UBSAN_COMMUT(survive, s32, s32, S32_MAX, add, s64,   0)	\
	/* TODO */						\
	UBSAN_COMMUT(sio, s32, s32, S32_MAX, add, s8,    3)	\
	UBSAN_COMMUT(sio, s32, s32, S32_MAX, add, s16,   3)	\
	UBSAN_COMMUT(sio, s32, s32, S32_MAX, add, s32,   3)	\
	/*UBSAN_COMMUT(sio, s32, s32, S32_MAX, add, s64, 3) */	\
	UBSAN_COMMUT(sio, s32, s32, S32_MAX, mul, s8,    2)	\
	UBSAN_COMMUT(sio, s32, s32, S32_MAX, mul, s16,   2)	\
	UBSAN_COMMUT(sio, s32, s32, S32_MAX, mul, s32,   2)	\
	/*UBSAN_COMMUT(sio, s32, s32, S32_MAX, mul, s64,   2) */	\
	UBSAN_TEST(sio, s32, s32, S32_MIN, sub, s8,    9)	\
	UBSAN_TEST(sio, s32, s32, S32_MIN, sub, s16,   9)	\
	UBSAN_TEST(sio, s32, s32, S32_MIN, sub, s32,   9)	\
	/*UBSAN_TEST(sio, s32, s32, S32_MIN, sub, s64,   9) */	\

#define LVALUE_S64_TESTS	/* TODO */

#define LVALUE_U8_TESTS		/* TODO */

#define LVALUE_U16_TESTS	/* TODO */

#define LVALUE_U32_TESTS		\
	/* TODO */						\
	UBSAN_COMMUT(uio, u32, u32, U32_MAX, add, u8,    3)	\
	UBSAN_COMMUT(uio, u32, u32, U32_MAX, add, u16,   3)	\
	UBSAN_COMMUT(uio, u32, u32, U32_MAX, add, u32,   3)	\
	/*UBSAN_COMMUT(uio, u32, u32, U32_MAX, add, u64,   3)*/	\
	UBSAN_COMMUT(uio, u32, u32, U32_MAX, mul, u8,    2)	\
	UBSAN_COMMUT(uio, u32, u32, U32_MAX, mul, u16,   2)	\
	UBSAN_COMMUT(uio, u32, u32, U32_MAX, mul, u32,   2)	\
	/*UBSAN_COMMUT(uio, u32, u32, U32_MAX, mul, u64,   2)*/	\
	UBSAN_TEST(uio, u32, u32, 0, sub, u8,    9)	\
	UBSAN_TEST(uio, u32, u32, 0, sub, u16,   9)	\
	UBSAN_TEST(uio, u32, u32, 0, sub, u32,   9)	\
	/*UBSAN_TEST(uio, u32, u32, 0, sub, u64,   9)*/	\

#define LVALUE_U64_TESTS	/* TODO */

/*
 * Pointer values cannot currently have the "wrap" attribute, so
 * don't use UBSAN_TEST, instead just the trapping tests.
 */
#define LVALUE_PTR_TESTS		\
	/* All within normal pointer ranges. */			\
	UBSAN_trap_TEST(survive, ptr, ptr, (void *)1, add, s32, 100)	\
	UBSAN_trap_TEST(survive, ptr, ptr, (void *)100, sub, s32, 50)	\
	UBSAN_trap_TEST(survive, ptr, ptr, (void *)(100), add, s32, INT_MAX / 2) \
	UBSAN_trap_TEST(survive, ptr, ptr, (void *)(-1), sub, s32, INT_MAX / 2)	\
	/* Operating on NULL should trap. (Even 0 ?!) */	\
	UBSAN_trap_TEST(po, ptr, ptr, NULL, add, s32, 0)		\
	UBSAN_trap_TEST(po, ptr, ptr, NULL, sub, s32, 0)		\
	UBSAN_trap_TEST(po, ptr, ptr, NULL, add, s32, 10)		\
	UBSAN_trap_TEST(po, ptr, ptr, NULL, sub, s32, 10)		\
	/* Overflow and underflow should trap. */		\
	UBSAN_trap_TEST(po, ptr, ptr, (void *)(-1), add, s32, 2)	\
	UBSAN_trap_TEST(po, ptr, ptr, (void *)1, sub, s32, 2)	\


#define UBSAN_TESTS		\
	LVALUE_S8_TESTS		\
	LVALUE_S16_TESTS	\
	LVALUE_S32_TESTS	\
	LVALUE_S64_TESTS	\
	LVALUE_U8_TESTS		\
	LVALUE_U16_TESTS	\
	LVALUE_U32_TESTS	\
	LVALUE_U64_TESTS	\
	LVALUE_PTR_TESTS

UBSAN_TESTS

TEST_HARNESS_MAIN
