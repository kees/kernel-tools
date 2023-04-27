#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <setjmp.h>

#include "harness.h"

/* Make sure "ptr" is not elided by the compiler. */
#define barrier_data(ptr) __asm__ __volatile__("": :"r"(ptr) :"memory")

FIXTURE(check) {
};

static void raise_usr1(int nr, siginfo_t *info, void *ucontext)
{
	raise(SIGUSR1);
}

/*
 * The test harness doesn't have a sane way to accept multiple signals
 * so we cheat and raise USR1 for either of our expected signals. Note
 * a bit of weirdness, here, since the test harness will raise SIGABRT
 * on any ASSERT failures, so we cannot use the ASSERT family here.
 */
FIXTURE_SETUP(check) {
        struct sigaction act = {
		.sa_flags = SA_SIGINFO,
		.sa_sigaction = raise_usr1,
	};

	/* glibc raises SIGABRT */
        EXPECT_EQ(sigaction(SIGABRT, &act, NULL), 0);
	/* musl with fortify_headers raises SIGILL */
        EXPECT_EQ(sigaction(SIGILL, &act, NULL), 0);
}

FIXTURE_TEARDOWN(check) {
}

TEST_F_SIGNAL(check, memset, SIGUSR1)
{
	char buf[4];
	volatile int too_big = sizeof(buf) + 1; /* Avoid compile-time evaluation. */

	memset(buf, 0, too_big);
	barrier_data(buf);
}

TEST_HARNESS_MAIN
