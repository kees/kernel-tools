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
# define __counted_by(member)	__attribute__((__element_count__(member)))
#else
# define __counted_by(member)	/* __attribute__((__element_count__(member))) */
#endif

#define MAX_INDEX	16

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

#define TEST_ACCESS(p, index)	do {			\
							\
	/* Index zero is in the array. */		\
	TH_LOG("safe: array[0] = 0xFF");		\
	(p)->array[0] = 0xFF;				\
	ASSERT_EQ((p)->array[0], 0xFF);			\
	TH_LOG("safe: array[%d] = 0xFF", index - 1);	\
	(p)->array[index - 1] = 0xFF;			\
	ASSERT_EQ((p)->array[index - 1], 0xFF);		\
							\
	/* "index" is expected to trap. */		\
	TH_LOG("trap: array[%d] = 0xFF", index);	\
	(p)->array[index] = 0xFF;			\
	/* Don't assert here. */			\
	TH_LOG("this should have been unreachable");	\
} while (0)

TEST_SIGNAL(fixed, SIGILL)
{
	struct fixed f = { };
	int index = MAX_INDEX + unconst;

	REPORT_SIZE(f.array);
	EXPECT_EQ(__builtin_dynamic_object_size(f.array, 1), 16*4);
	TEST_ACCESS(&f, index);
}

TEST_SIGNAL(dynamic, SIGILL)
{
	int count = MAX_INDEX + unconst;

	/* malloc() is marked with __attribute__((alloc_size(1))) */
	struct flex *p = malloc(sizeof(*p) + count * sizeof(*p->array));

	REPORT_SIZE(p->array);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), count * sizeof(*p->array));
	TEST_ACCESS(p, count);
}

/* Hide the allocation size by using a leaf function. */
static void noinline ignore_alloc_size(struct __test_metadata *_metadata,
				       struct annotated *p, int index)
{
	REPORT_SIZE(p->array);
	EXPECT_EQ(__builtin_dynamic_object_size(p->array, 1), p->foo * sizeof(*p->array));
	TEST_ACCESS(p, index);
}

TEST_SIGNAL(element_count, SIGILL)
{
	struct annotated *p;
	int index = MAX_INDEX + unconst;

	p = malloc(sizeof(*p) + MAX_INDEX * sizeof(*p->array));
	p->foo = MAX_INDEX;
	ignore_alloc_size(_metadata, p, index);
}

TEST_HARNESS_MAIN
