/* Build with -Wall -O2 -fstrict-flex-arrays=3 -fsanitize=bounds -fsanitize=object-size -fstrict-flex-arrays=3 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "harness.h"

#define noinline __attribute__((__noinline__))

volatile int unconst = 0; /* used to stop optimizer from seeing constant expressions. */

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

#if __has_attribute(__element_count__)
# define __counted_by(member)	__attribute__((__element_count__(#member)))
#else
# define __counted_by(member)	/* __attribute__((__element_count__(#member))) */
#endif

#define MAX_INDEX	16
#define SIZE_BUMP	 2

enum enforcement {
	SHOULD_NOT_TRAP = 0,
	SHOULD_TRAP,
};

enum set_count {
	SKIP_COUNT_MEMBER = 0,
	SET_COUNT_MEMBER,
};

struct fixed {
	unsigned long flags;
	size_t foo;
	int array[MAX_INDEX];
};

struct flex {
	unsigned long flags;
	size_t foo;
	int array[];
};

struct annotated {
	unsigned long flags;
	size_t foo;
	int array[] __counted_by(foo);
};

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

TEST_SIGNAL(fixed_size_enforced_by_sanitizer, SIGILL)
{
	struct fixed f = { };
	int index = MAX_INDEX + unconst;

	REPORT_SIZE(f.array);
	TEST_ACCESS(&f, index, SHOULD_TRAP);
}

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

TEST_SIGNAL(alloc_size_enforced_by_sanitizer, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + count * sizeof(*p->array));

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, count, SHOULD_TRAP);
}

/* Hide the allocation size by using a leaf function. */
static struct annotated * noinline alloc_annotated(int index, enum set_count action)
{
	struct annotated *p;

	p = malloc(sizeof(*p) + index * sizeof(*p->array));
	if (action == SET_COUNT_MEMBER)
		p->foo = index;

	return p;
}

TEST(unknown_size_unknown_to_bdos)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = alloc_annotated(index, SKIP_COUNT_MEMBER);

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), SIZE_MAX);
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), SIZE_MAX);
}

TEST(unknown_size_ignored_by_sanitizer)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = alloc_annotated(index, SKIP_COUNT_MEMBER);

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, index, SHOULD_NOT_TRAP);
}

TEST(element_count_seen_by_bdos)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = alloc_annotated(index, SET_COUNT_MEMBER);

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), p->foo * sizeof(*p->array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + p->foo * sizeof(*p->array));
}

TEST_SIGNAL(element_count_enforced_by_sanitizer, SIGILL)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = alloc_annotated(index, SET_COUNT_MEMBER);

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, index, SHOULD_TRAP);
}

TEST(alloc_size_with_smaller_element_count_seen_by_bdos)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + (count + SIZE_BUMP) * sizeof(*p->array));
	p->foo = count;

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), p->foo * sizeof(*p->array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + p->foo * sizeof(*p->array));
}

TEST_SIGNAL(alloc_size_with_smaller_element_count_enforced_by_sanitizer, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + (count + SIZE_BUMP) * sizeof(*p->array));
	p->foo = count;

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, count, SHOULD_TRAP);
}

TEST(alloc_size_with_bigger_element_count_seen_by_bdos)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + count * sizeof(*p->array));
	p->foo = count + SIZE_BUMP;

	REPORT_SIZE(p->array);
	EXPECT_EQ(sizeof(*p), offsetof(typeof(*p), array));
	/* Check array size alone. */
	EXPECT_EQ(__builtin_object_size(p->array, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), (p->foo - SIZE_BUMP) * sizeof(*p->array));
	/* Check check entire object size. */
	EXPECT_EQ(__builtin_object_size(p, 1), SIZE_MAX);
	EXPECT_EQ(__builtin_dynamic_object_size(p, 1), sizeof(*p) + (p->foo - SIZE_BUMP) * sizeof(*p->array));
}

TEST_SIGNAL(alloc_size_with_bigger_element_count_enforced_by_sanitizer, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + count * sizeof(*p->array));
	p->foo = count + SIZE_BUMP;

	REPORT_SIZE(p->array);
	TEST_ACCESS(p, count, SHOULD_TRAP);
}

TEST_HARNESS_MAIN
