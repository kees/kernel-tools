// strict_launcher.c
//
// Run a binary that calls seccomp(SECCOMP_SET_MODE_STRICT) or
// prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT) *inside* a container that has
// already applied a SECCOMP_MODE_FILTER program.
//
// Problem: the kernel refuses STRICT-on-FILTER. seccomp_may_assign_mode()
//   returns false when current->seccomp.mode is set and != requested mode,
//   so the binary's strict call fails with EINVAL. We can't apply strict
//   before the binary starts (it needs to open its files first), and we
//   can't downgrade the container's filter.
//
// Approach: ptrace-supervise the binary. A tiny pre-exec seccomp filter
//   traps ONLY the strict-enable calls via SECCOMP_RET_TRACE. When one
//   fires, we:
//     1. cancel the original (doomed) strict syscall,
//     2. mmap a scratch page in the tracee and write a filter-mode BPF
//        program that duplicates strict's mode1 whitelist
//        (read, write, exit, rt_sigreturn -> ALLOW; else KILL),
//     3. inject seccomp()/prctl() in the *tracee* to install it (filter
//        mode stacks fine on top of the container's filter),
//     4. forge rax=0 so the binary thinks strict succeeded,
//     5. PTRACE_DETACH and let it run at full speed; the kernel now
//        enforces the strict-equivalent filter on its own.
//
// x86_64 only. arm64/riscv need register-name + __NR_* + AUDIT_ARCH changes
// and an arch-appropriate `syscall`/`svc`/`ecall` instruction width for the
// re-exec address (here: 2 bytes for `0f 05`).
//
// Build:  gcc -Wall -O2 -o strict_launcher strict_launcher.c
// Run:    ./strict_launcher /path/to/binary [args...]

#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#if !defined(__x86_64__)
#error "This launcher is x86_64-specific; adapt regs/syscall nrs for your arch."
#endif

#ifndef SECCOMP_SET_MODE_STRICT
#define SECCOMP_SET_MODE_STRICT 0
#endif
#ifndef SECCOMP_SET_MODE_FILTER
#define SECCOMP_SET_MODE_FILTER 1
#endif
#ifndef SECCOMP_MODE_STRICT
#define SECCOMP_MODE_STRICT 1
#endif
#ifndef SECCOMP_MODE_FILTER
#define SECCOMP_MODE_FILTER 2
#endif

// To faithfully duplicate STRICT, a violation kills the *thread*
// (strict does do_exit(SIGKILL)). For a multithreaded target you may
// prefer SECCOMP_RET_KILL_PROCESS; flip this define.
#define STRICT_DENY_ACTION SECCOMP_RET_KILL_THREAD

#define SYSCALL_INSN_LEN 2  // x86_64 `syscall` == 0f 05

// ---- The strict-equivalent filter-mode program -----------------------------
// Mirrors the kernel's mode1_syscalls whitelist exactly:
//   read, write, exit, rt_sigreturn  -> ALLOW ; everything else -> KILL.
// Wrong arch -> KILL (defensive; strict is arch-table relative, we pin native).
static struct sock_filter strict_equiv[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    BPF_STMT(BPF_RET | BPF_K, STRICT_DENY_ACTION),

    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_read,         4, 0),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_write,        3, 0),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_exit,         2, 0),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_rt_sigreturn, 1, 0),
    BPF_STMT(BPF_RET | BPF_K, STRICT_DENY_ACTION),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
};

// ---- The pre-exec supervisor filter ----------------------------------------
// Traps ONLY the strict-enable calls so injection can't recurse, and so the
// huge startup syscall stream is never ptrace-stopped:
//   seccomp(SECCOMP_SET_MODE_STRICT, ...)              -> TRACE
//   prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, ...)    -> TRACE
//   everything else                                    -> ALLOW
static struct sock_filter trap_strict[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_seccomp, 1, 0),  // -> seccomp chk
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_prctl,   2, 7),  // -> prctl chk / ALLOW
    // seccomp(): args[0] == SECCOMP_SET_MODE_STRICT ?
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[0])),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SECCOMP_SET_MODE_STRICT, 4, 5),
    // prctl(): args[0] == PR_SET_SECCOMP ?
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[0])),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, PR_SET_SECCOMP, 0, 3),
    //         args[1] == SECCOMP_MODE_STRICT ?
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[1])),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SECCOMP_MODE_STRICT, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
};

static void die(const char *what) {
    fprintf(stderr, "strict_launcher: %s: %s\n", what, strerror(errno));
    exit(1);
}

// ---- child: install trap filter, then exec the real binary -----------------
static void child_exec(char **argv) {
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) die("PTRACE_TRACEME");

    // NO_NEW_PRIVS is required to install a filter unprivileged, and it
    // persists across exec so we don't need to re-set it later.
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) die("PR_SET_NO_NEW_PRIVS");

    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(trap_strict) / sizeof(trap_strict[0])),
        .filter = trap_strict,
    };
    // Stacks on top of the container's existing filter.
    if (syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog) < 0)
        die("install trap filter");

    execvp(argv[0], argv);
    die("execvp");  // only on failure
}

// ---- tracee memory + register helpers --------------------------------------
static void poke_mem(pid_t pid, unsigned long addr, const void *buf, size_t len) {
    // len is rounded up to 8 by the caller; write word by word.
    const unsigned char *p = buf;
    for (size_t off = 0; off < len; off += sizeof(long)) {
        long word;
        memcpy(&word, p + off, sizeof(long));
        if (ptrace(PTRACE_POKEDATA, pid, addr + off, (void *)word) < 0)
            die("PTRACE_POKEDATA");
    }
}

// Wait for the next ptrace-stop; aborts if the tracee died unexpectedly.
static int wait_stop(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) < 0) die("waitpid");
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        fprintf(stderr, "strict_launcher: tracee gone mid-injection\n");
        exit(1);
    }
    return status;
}

// Run one syscall in the tracee by hijacking a `syscall` instruction at
// `insn`. Returns the syscall's result (rax). Restores the regs we started
// from so calls can be chained from a stable base.
static long inject_syscall(pid_t pid, unsigned long insn, long nr,
                           long a0, long a1, long a2,
                           long a3, long a4, long a5) {
    struct user_regs_struct save, r;
    if (ptrace(PTRACE_GETREGS, pid, 0, &save) < 0) die("GETREGS");
    r = save;
    r.rip = insn;       // point at `syscall` (0f 05)
    r.rax = (unsigned long)nr;
    r.orig_rax = (unsigned long)nr;
    r.rdi = a0; r.rsi = a1; r.rdx = a2;
    r.r10 = a3; r.r8  = a4; r.r9  = a5;
    if (ptrace(PTRACE_SETREGS, pid, 0, &r) < 0) die("SETREGS");

    // PTRACE_SYSCALL stops at entry, then at exit (TRACESYSGOOD => SIGTRAP|0x80).
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) die("SYSCALL entry");
    wait_stop(pid);  // syscall-entry-stop
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) die("SYSCALL exit");
    wait_stop(pid);  // syscall-exit-stop

    if (ptrace(PTRACE_GETREGS, pid, 0, &r) < 0) die("GETREGS post");
    long ret = (long)r.rax;
    if (ptrace(PTRACE_SETREGS, pid, 0, &save) < 0) die("SETREGS restore");
    return ret;
}

// Handle one trapped strict-enable call: install the strict-equivalent filter
// into the tracee and forge success. Returns 1 on success.
static int handle_strict(pid_t pid) {
    struct user_regs_struct saved;
    if (ptrace(PTRACE_GETREGS, pid, 0, &saved) < 0) die("GETREGS strict");

    int used_seccomp = (saved.orig_rax == (unsigned long)__NR_seccomp);
    unsigned long syscall_insn = saved.rip - SYSCALL_INSN_LEN;

    // 1. Cancel the doomed strict syscall: orig_rax = -1 skips it, then run
    //    to its (no-op) exit stop so the tracee is at a clean boundary.
    struct user_regs_struct r = saved;
    r.orig_rax = (unsigned long)-1;
    if (ptrace(PTRACE_SETREGS, pid, 0, &r) < 0) die("SETREGS cancel");
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) < 0) die("SYSCALL skip");
    wait_stop(pid);  // skipped-syscall exit stop

    // 2. Get a scratch page in the tracee (filter program must live in its
    //    address space). We leak it: it can't be munmap'd once the strict
    //    filter is live, and 4 KiB in a soon-to-be-locked-down process is fine.
    long scratch = inject_syscall(pid, syscall_insn, __NR_mmap,
                                  0, 4096,
                                  PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (scratch < 0 && scratch > -4096) {
        fprintf(stderr, "strict_launcher: mmap in tracee failed: %s\n",
                strerror((int)-scratch));
        return 0;
    }
    unsigned long base = (unsigned long)scratch;

    // 3. Lay out [ sock_filter[] | sock_fprog ] in the scratch page.
    size_t flen = sizeof(strict_equiv);
    size_t fprog_off = (flen + 7) & ~(size_t)7;  // 8-align the struct
    unsigned char blob[512];
    if (fprog_off + sizeof(struct sock_fprog) > sizeof(blob)) {
        fprintf(stderr, "strict_launcher: filter too big for scratch blob\n");
        return 0;
    }
    memset(blob, 0, sizeof(blob));
    memcpy(blob, strict_equiv, flen);
    struct sock_fprog fprog = {
        .len = (unsigned short)(flen / sizeof(struct sock_filter)),
        .filter = (struct sock_filter *)base,  // program starts at page base
    };
    memcpy(blob + fprog_off, &fprog, sizeof(fprog));
    size_t blob_len = (fprog_off + sizeof(fprog) + 7) & ~(size_t)7;
    poke_mem(pid, base, blob, blob_len);
    unsigned long fprog_addr = base + fprog_off;

    // 4. Install it, mirroring whichever interface the binary used (so we only
    //    rely on a syscall the container filter already permits).
    long ir;
    if (used_seccomp)
        ir = inject_syscall(pid, syscall_insn, __NR_seccomp,
                            SECCOMP_SET_MODE_FILTER, 0, (long)fprog_addr, 0, 0, 0);
    else
        ir = inject_syscall(pid, syscall_insn, __NR_prctl,
                            PR_SET_SECCOMP, SECCOMP_MODE_FILTER,
                            (long)fprog_addr, 0, 0, 0);
    if (ir != 0) {
        fprintf(stderr, "strict_launcher: filter install failed: %s\n",
                strerror((int)-ir));
        return 0;
    }

    // 5. Forge success of the original strict call and resume past it.
    r = saved;
    r.rax = 0;  // binary sees its strict call return 0
    if (ptrace(PTRACE_SETREGS, pid, 0, &r) < 0) die("SETREGS forge");
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <binary> [args...]\n", argv[0]);
        return 2;
    }

    pid_t pid = fork();
    if (pid < 0) die("fork");
    if (pid == 0) child_exec(&argv[1]);

    // First stop: the execve SIGTRAP.
    int status;
    if (waitpid(pid, &status, 0) < 0) die("waitpid initial");

    if (ptrace(PTRACE_SETOPTIONS, pid, 0,
               (void *)(PTRACE_O_TRACESECCOMP | PTRACE_O_TRACESYSGOOD |
                        PTRACE_O_EXITKILL)) < 0)
        die("PTRACE_SETOPTIONS");

    if (ptrace(PTRACE_CONT, pid, 0, 0) < 0) die("PTRACE_CONT");

    for (;;) {
        if (waitpid(pid, &status, 0) < 0) die("waitpid loop");

        if (WIFEXITED(status)) return WEXITSTATUS(status);
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "strict_launcher: tracee killed by signal %d\n",
                    WTERMSIG(status));
            return 128 + WTERMSIG(status);
        }

        // SECCOMP_RET_TRACE -> PTRACE_EVENT_SECCOMP
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP &&
            (status >> 8) == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {
            if (!handle_strict(pid)) {
                // Fail closed: never let the binary run thinking it's
                // sandboxed when it isn't.
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                return 1;
            }
            // Filter is live; detach and let it run unsupervised. The child
            // is still ours, so we can still reap it below.
            if (ptrace(PTRACE_DETACH, pid, 0, 0) < 0) die("PTRACE_DETACH");
            if (waitpid(pid, &status, 0) < 0) die("waitpid final");
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
            return 0;
        }

        // Any other stop (real signal): pass it through transparently.
        int sig = WSTOPSIG(status);
        if (sig == SIGTRAP) sig = 0;  // spurious trace trap
        if (ptrace(PTRACE_CONT, pid, 0, (void *)(long)sig) < 0)
            die("PTRACE_CONT passthrough");
    }
}
