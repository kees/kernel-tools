/*
 * Execute a static QMAGIC ia32 a.out binary.
 * Copyright 2022 Kees Cook <keescook@chromium.org>
 * License: GPLv2
 *
 * For a more complete solution, see also:
 * https://github.com/siegfriedpammer/run-aout
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

/* Locally define the stuff from a.out.h since that file may disappear. */
struct a_out
{
	unsigned int a_info;	/* machine type, magic, etc */
	unsigned int a_text;	/* text size */
	unsigned int a_data;	/* data size */
	unsigned int a_bss;	/* desired bss size */
	unsigned int a_syms;	/* symbol table size */
	unsigned int a_entry;	/* entry address */
	unsigned int a_trsize;	/* text relocation size */
	unsigned int a_drsize;	/* data relocation size */
};

#define Q_MAGIC_LOAD_ADDR	0x1000
#define MMAP_MIN_ADDR_PATH	"/proc/sys/vm/mmap_min_addr"

#define ALIGN(x, a)		ALIGN_MASK(x, (unsigned long)(a) - 1)
#define ALIGN_MASK(x, mask)	(typeof(x))(((unsigned long)(x) + (mask)) & ~(mask))

static void check_mmap_min_addr(void)
{
	unsigned long addr;
	char buf[128], *result;
	FILE *proc;

	proc = fopen(MMAP_MIN_ADDR_PATH, "r");
	if (!proc)
		return;
	result = fgets(buf, sizeof(buf), proc);
	fclose(proc);
	if (!result)
		return;

	addr = strtoul(result, NULL, 0);
	if (addr <= Q_MAGIC_LOAD_ADDR)
		return;

	fprintf(stderr, "%s is set to %lu but QMAGIC a.out binaries must be mapped at %u.\n",
		MMAP_MIN_ADDR_PATH, addr, Q_MAGIC_LOAD_ADDR);
	fprintf(stderr, "To temporarily change this, run: sudo sysctl -w vm.mmap_min_addr=%u\n",
		Q_MAGIC_LOAD_ADDR);
}

int main(int argc, char *argv[], char *envp[])
{
	const unsigned long qmagic = 0x006400cc;
	struct a_out *aout;
	struct stat info;
	unsigned char *image, *image_end, *bss, *bss_end, *stack, *stack_end;
	char **p;
	unsigned long *sp;
	struct rlimit rlim;
	int pagesize;
	int fd;
	int argc_copy, envc_copy;
	char **argv_copy, **envp_copy;

	if (sizeof(void *) != 4) {
		fprintf(stderr, "Eek: I was compiled in 64-bit mode. Please build with -m32.\n");
		return 1;
	}

	if (argc < 2) {
		fprintf(stderr, "Usage: %s a.out [arg ...]\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror(argv[1]);
		return 1;
	}

	if (fstat(fd, &info) < 0) {
		perror(argv[1]);
		return 1;
	}

	if (info.st_size < sizeof(*aout)) {
		fprintf(stderr, "%s: too small to read a.out header\n", argv[1]);
		return 1;
	}

	/* Load file into memory at Q_MAGIC_LOAD_ADDR. */
	pagesize = getpagesize();
	image = mmap((void *)Q_MAGIC_LOAD_ADDR, info.st_size, PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_FIXED | MAP_PRIVATE, fd, 0);
	if (image == MAP_FAILED) {
		perror("mmap");
		check_mmap_min_addr();
		return 1;
	}
	image_end = ALIGN(image + info.st_size, pagesize);

	aout = (struct a_out *)image;
	if (aout->a_info != qmagic) {
		fprintf(stderr, "%s: not ia32 QMAGIC a.out binary (header 0x%x != expected 0x%lx)\n",
			argv[1], aout->a_info, qmagic);
		return 1;
	}

	if (aout->a_syms != 0) {
		fprintf(stderr, "%s: a.out header a_syms must be 0.\n", argv[1]);
		return 1;
	}

	if (aout->a_trsize != 0) {
		fprintf(stderr, "%s: a.out header a_trsize must be 0.\n", argv[1]);
		return 1;
	}

	if (aout->a_drsize != 0) {
		fprintf(stderr, "%s: a.out header a_drsize must be 0.\n", argv[1]);
		return 1;
	}

	/* See if .bss needs to extend beyond end of the file mmap. */
	bss = image + aout->a_text + aout->a_data;
	bss_end = bss + aout->a_bss;
	if (bss_end > image_end) {
		bss = mmap(bss, aout->a_bss, PROT_READ | PROT_WRITE,
			   MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (bss == MAP_FAILED) {
			perror("mmap bss");
			return 1;
		}
	}
	/* Zero out .bss. */
	memset(bss, 0, ALIGN(aout->a_bss, pagesize));

	/* Prepare stack, based on current stack. */
	if (getrlimit(RLIMIT_STACK, &rlim) < 0) {
		perror("getrlimit");
		return 1;
	}

	/* Default to 8MiB */
	if (rlim.rlim_cur == RLIM_INFINITY)
		rlim.rlim_cur = 8 * 1024 * 1024;

	stack = mmap(NULL, rlim.rlim_cur, PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap stack");
		return 1;
	}
	stack_end = ALIGN(stack + rlim.rlim_cur, pagesize);

	/* Fill top of stack with arg/env pointers. */
	sp = (unsigned long *)stack_end;
	sp--;

	/* do not include argv[0] */
	argc_copy = argc - 1;
	argv++;

	/* count envp */
	for (envc_copy = 0, p = envp; *p; envc_copy++, p++) ;

	/* make room for envp pointers */
	sp -= envc_copy + 1;
	envp_copy = (char **)sp;
	/* make room for argv pointers */
	sp -= argc_copy + 1;
	argv_copy = (char **)sp;

	/* store pointers and argc */
	*--sp = (unsigned long)envp_copy;
	*--sp = (unsigned long)argv_copy;
	*--sp = (unsigned long)argc_copy;

	/* copy argv pointer (contents can stay where they already are) */
	while (argc_copy--)
		*argv_copy++ = *argv++;
	*argv_copy = 0;

	/* copy envp (contents can stay where they already are) */
	while (envc_copy--)
		*envp_copy++ = *envp++;

	/* Aim sp at argc, and jump! */
	asm volatile ("movl %0, %%esp\njmp *%1\n" : : "rm" (sp), "r"(aout->a_entry) );

	/* This should be unreachable. */
	fprintf(stderr, "They found me. I don't how, but they found me.\n");
	return 2;
}
