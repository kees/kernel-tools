/* Build with -Wall -O2 -fstrict-flex-arrays=3 -fsanitize=bounds -fsanitize=object-size -fstrict-flex-arrays=3 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "harness.h"

#define noinline __attribute__((__noinline__))

/* Used to stop optimizer from seeing constant expressions. */
volatile int unconst = 0;

#define REPORT_SIZE(p)      do {    \
	const size_t bdos = __builtin_dynamic_object_size(p, 1); \
    \
	if (__builtin_constant_p(bdos)) { \
		if (bdos == SIZE_MAX) { \
			TH_LOG(#p " has an unknown-to-bdos size"); \
		} else { \
			TH_LOG(#p " has a fixed size: %zu (%zu elements of size %zu)", bdos, \
				bdos / sizeof(*(p)), sizeof(*(p))); \
		} \
	} else { \
		TH_LOG(#p " has a dynamic size: %zu (%zu elements of size %zu)", bdos, \
			bdos / sizeof(*(p)), sizeof(*(p))); \
	} \
	fflush(NULL); \
} while (0)

#if __has_attribute(__counted_by__)
# define __counted_by(member)	__attribute__((__counted_by__(member)))
#else
# define __counted_by(member)	/* __attribute__((__counted_by__(member))) */
#endif

#define MAX_INDEX	16
#define SIZE_BUMP	 2

enum enforcement {
	SHOULD_NOT_TRAP = 0,
	SHOULD_TRAP,
};

struct fixed {
	unsigned long flags;
	size_t count;
	int array[MAX_INDEX];
};

struct flex {
	unsigned long flags;
	size_t count;
	int array[];
};

struct annotated {
	unsigned long flags;
	size_t count;
	int array[] __counted_by(count);
};

/*
 * Test safe accesses at index 0 and index-1, then optionally check
 * what happens when accessing "index".
 */
#define TEST_ACCESS(p, index, enforcement)	do {		\
								\
	/* Index zero is in the array. */			\
	TH_LOG("safe: array[0] = 0xFF");			\
	(p)->array[0] = 0xFF;					\
	ASSERT_EQ((p)->array[0], 0xFF);				\
	TH_LOG("safe: array[%d] = 0xFF", index - 1);		\
	(p)->array[index - 1] = 0xFF;				\
	ASSERT_EQ((p)->array[index - 1], 0xFF);			\
								\
	if (enforcement == SHOULD_TRAP) {			\
		/* "index" is expected to trap. */		\
		TH_LOG("traps: array[%d] = 0xFF", index);	\
	} else {						\
		TH_LOG("ignored: array[%d] = 0xFF", index);	\
	}							\
	(p)->array[index] = 0xFF;				\
	if (enforcement == SHOULD_TRAP) {			\
		/* Don't assert: test for lack of signal. */	\
		TH_LOG("this should have been unreachable");	\
	}							\
} while (0)

/* Helper to hide the allocation size by using a leaf function. */
static struct flex * noinline alloc_flex(int index)
{
	struct flex *f;

	return malloc(sizeof(*f) + index * sizeof(*f->array));
}

/* Helper to hide the allocation size by using a leaf function. */
static struct annotated * noinline alloc_annotated(int index)
{
	struct annotated *p;

	p = malloc(sizeof(*p) + index * sizeof(*p->array));
	p->count = index;

	return p;
}

/*
 * For a structure ending with a fixed-size array, sizeof(*p) should
 * match __builtin_object_size(p, 1), which should also match
 * __builtin_dynamic_object_size(p, 1). This should work for both the
 * array itself and the object as a whole.
 */
TEST(fixed_size_seen_by_bdos)
{
	struct fixed f = { };

	REPORT_SIZE(f.array);
	EXPECT_EQ(sizeof(f) - sizeof(f.array), offsetof(typeof(f), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(f.array, 1), sizeof(f.array));
	EXPECT_EQ(__builtin_dynamic_object_size(f.array, 1), sizeof(f.array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(&f, 1), sizeof(f));
	EXPECT_EQ(__builtin_dynamic_object_size(&f, 1), sizeof(f));
}

/*
 * For a fixed-size array, the sanitizer should trap when accessing
 * beyond the largest index.
 */
TEST_SIGNAL(fixed_size_enforced_by_sanitizer, SIGILL)
{
	struct fixed f = { };
	int index = MAX_INDEX + unconst;

	REPORT_SIZE(f.array);
	TEST_ACCESS(&f, index, SHOULD_TRAP);
}

/*
 * For a structure ending with a flexible array, sizeof(*p) should
 * match the offset of the flexible array. Since the size of the
 * array is unknown, both __builtin_object_size(p, 1) and
 * __builtin_dynamic_object_size(p, 1) should report SIZE_MAX. This
 * should be true for both the array itself and the object as a whole.
 */
TEST(unknown_size_unknown_to_bdos)
{
	struct flex *p;
	int index = MAX_INDEX + unconst;

	/* Hide actual allocation size from compiler. */
	p = alloc_flex(index);

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), SIZE_MAX);
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), SIZE_MAX);
}

/*
 * For a structure ending with a flexible array, the sanitizer has
 * no information about max indexes and should not trap for any
 * accesses.
 */
TEST(unknown_size_ignored_by_sanitizer)
{
	struct flex *p;
	int index = MAX_INDEX + unconst;

	/* Hide actual allocation size from compiler. */
	p = alloc_flex(index);

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, index, SHOULD_NOT_TRAP);
}

/*
 * For a structure ending with a flexible array where the allocation
 * is visible (via the __alloc_size attribute), this size should be
 * visible to __builtin_dynamic_object_size(p, 1) for both the array
 * itself and the object as a whole. The result of sizeof() and
 * __builtin_object_size() should remain unchanged compared to the
 * "unknown" cases above.
 */
TEST(alloc_size_seen_by_bdos)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + count * sizeof(*p->array));

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), count * sizeof(*p->array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + count * sizeof(*p->array));
}

/*
 * For a structure ending with a flexible array where the allocation
 * is visible (via the __alloc_size attribute), the sanitizer should
 * trap when accessing beyond the highest index still contained by the
 * allocation.
 */
TEST_SIGNAL(alloc_size_enforced_by_sanitizer, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + count * sizeof(*p->array));

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, count, SHOULD_TRAP);
}

/*
 * For a structure ending with a flexible array where the allocation
 * is hidden, but the array size member (identified with the
 * __counted_by attribute) has been set, the calculated size should
 * be visible to __builtin_dynamic_object_size(p, 1) for both the array
 * itself and the object as a whole. The result of sizeof() and
 * __builtin_object_size() should remain unchanged compared to the
 * "unknown" cases above.
 */
TEST(counted_by_seen_by_bdos)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = alloc_annotated(index);

	REPORT_SIZE(p->array);

	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), p->count * sizeof(*p->array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + p->count * sizeof(*p->array));
}

/*
 * For a structure ending with a flexible array where the allocation
 * is hidden, but the array size member (identified with the
 * __counted_by attribute) has been set, the sanitizer should trap
 * when accessing beyond the index stored in the counted_by member.
 */
TEST_SIGNAL(counted_by_enforced_by_sanitizer, SIGILL)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = alloc_annotated(index);

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, index, SHOULD_TRAP);
}

/*
 * When both __alloc_size and __counted_by are available to calculate
 * sizes, the smaller of the two should take precedence. Check that when
 * the visible allocation is larger than the size calculated from the
 * counted_by member's value, __builtin_dynamic_object_size(p, 1) sees
 * the smaller of the two (the allocation), with everything else unchanged.
 */
TEST(alloc_size_with_smaller_counted_by_seen_by_bdos)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct annotated *p = malloc(sizeof(*p) + (count + SIZE_BUMP) * sizeof(*p->array));
	p->count = count;

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), p->count * sizeof(*p->array));
	/* Check check entire object size: this is not limited by __counted_by. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + (p->count + SIZE_BUMP) * sizeof(*p->array));
}

/*
 * When both __alloc_size and __counted_by are available to calculate
 * sizes, the smaller of the two should take precedence. Check that when
 * the visible allocation is larger than the size calculated from the
 * counted_by member's value, the sanitizer should trap when accessing
 * beyond the index stored in the counted_by member.
 */
TEST_SIGNAL(alloc_size_with_smaller_counted_by_enforced_by_sanitizer, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct annotated *p = malloc(sizeof(*p) + (count + SIZE_BUMP) * sizeof(*p->array));
	p->count = count;

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, count, SHOULD_TRAP);
}

/*
 * When both __alloc_size and __counted_by are available to calculate
 * sizes, the smaller of the two should take precedence. Check that when
 * the visible allocation that is smaller than the size calculated from
 * the counted_by member's value, __builtin_dynamic_object_size(p, 1)
 * sees the smaller of the two (the counted_by member size calculation),
 * with everything else unchanged.
 */
TEST(alloc_size_with_bigger_counted_by_seen_by_bdos)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct annotated *p = malloc(sizeof(*p) + count * sizeof(*p->array));
	p->count = count + SIZE_BUMP;

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), (p->count - SIZE_BUMP) * sizeof(*p->array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + (p->count - SIZE_BUMP) * sizeof(*p->array));
}

/*
 * When both __alloc_size and __counted_by are available to calculate
 * sizes, the smaller of the two should take precedence. Check that when
 * the visible allocation is smaller than the size calculated from the
 * counted_by member's value, the sanitizer should trap when accessing
 * beyond the highest index still contained by the allocation.
 */
TEST_SIGNAL(alloc_size_with_bigger_counted_by_enforced_by_sanitizer, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct annotated *p = malloc(sizeof(*p) + count * sizeof(*p->array));
	p->count = count + SIZE_BUMP;

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, count, SHOULD_TRAP);
}

TEST_HARNESS_MAIN
