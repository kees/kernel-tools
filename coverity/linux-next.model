/*
 * Coverity Scan model
 * https://scan.coverity.com/models
 *
 * This is a modeling file for Coverity Scan.
 * Modeling helps to avoid false positives.
 *
 * - A model file can't import any header files.
 * - Therefore only some built-in primitives like int, char and void are
 *   available but not wchar_t, NULL etc.
 * - Modeling doesn't need full structs and typedefs. Rudimentary structs
 *   and similar types are sufficient.
 * - An uninitialized local pointer is not an error. It signifies that the
 *   variable could be either NULL or have some data.
 *
 * Coverity Scan doesn't pick up modifications automatically. The model file
 * must be uploaded by an admin in the analysis settings.
 *
 * Some of this initially cribbed from http://hg.python.org/cpython/file/tip/Misc/coverity_model.c
 */

#define EAGAIN 35

int condition;

typedef int bool;

void panic(const char *msg) {
	__coverity_panic__();
}

/* Right now, coverity doesn't seem to model macros, so this isn't working.
 * Hopefully one day soon.. */
void BUG(void) {
	__coverity_panic__();
}

void unreachable(void) {
	__coverity_panic__();
}


/* Can never be <= 0. */
int num_online_cpus(void)
{
	return 4096;
}

/* don't treat these as "always true" (silence DEADCODE false positives) */
int cpu_has_fxsr(void)
{
	if (condition)
		return 1;
	else
		return 0;
}
int cpu_has(void *, int foo)
{
	if (condition)
		return 1;
	else
		return 0;
}
int static_cpu_has(int foo)
{
	if (condition)
		return 1;
	else
		return 0;
}


/* Mark data copied from userspace as tainted. */
long copy_from_user(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long _copy_from_user(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user_inatomic(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user_nocache(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user_inatomic_nocache(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}

long __copy_from_user_ll(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user_ll_nozero(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user_ll_nocache(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}
long __copy_from_user_ll_nocache_nozero(void *to, const void * from, unsigned long n)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(to);
	__coverity_writeall__(to);
}

long get_user(void *x, const void * from)
{
	__coverity_tainted_data_argument__(from);
	__coverity_tainted_data_argument__(x);
}

void *memset(void *dst, int c, size_t len)
{
	__coverity_writeall__(dst);
	return dst;
}
void *memcpy(void *dst, void *src, size_t len)
{
	__coverity_writeall__(dst);
	return dst;
}


/*
 * Allocation routines.
 *
 */
typedef unsigned int gfp_t;

void *kmalloc(size_t s, gfp_t gfp)
{
	if (condition)
		return __coverity_alloc__(s);
	else
		return 0;
}
void kfree(void* x) {
	__coverity_free__(x);
}

void *vmalloc(unsigned long size)
{
	if (condition)
		return __coverity_alloc__(size);
	else
		return 0;
}
void vfree(void* x) {
	__coverity_free__(x);
}

void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
	if (condition) {
		void *resized;

		resized = __coverity_alloc__(new_size);
		/*
		 * We can't tell coverity directly about our copy here,
		 * since we don't know the size of "p", but we can at least
		 * transfer taint and show we've written to "resized".
		 */
		__coverity_writeall__(resized);
		__coverity_tainted_data_transitive__(resized, p);
		__coverity_free__(p);
		return resized;
	} else
		return 0;
}

typedef struct {} kmem_cache;

void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
	// we don't have a 'size' argument for this alloc routine.
	if (condition)
		return __coverity_alloc_nosize__();
	else
		return 0;
}

void kmem_cache_free(struct kmem_cache *cachep, void *objp)
{
	__coverity_free__(objp);
}

typedef struct {} device;
typedef struct {} dma_attrs;
typedef long dma_addr_t;

/* Disabled: Looks like this creates too many false positives
void *dma_alloc_attrs(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp, struct dma_attrs *attrs)
{
	if (condition)
		return __coverity_alloc__(size);
	else
		return 0;
}
*/

void dma_pool_free(struct dma_pool *pool, void *vaddr, dma_addr_t dma)
{
	__coverity_free__(vaddr);
}


/*
 * format string sinks
 */
int printk(const char *fmt, ...)
{
	__coverity_format_string_sink__(fmt);
}

/* separate, generic, try_lock function */
int trylock(void *lock)
{
	/* Using this uninitialized variable allows Coverity to analyze both
	 * outcomes of this function. */
	int cond;
	if (cond)
	{
		__coverity_exclusive_lock_acquire__(lock);
		return 1;
	}
	return 0;
}

/*
 * annotate all mutexes
 */
typedef struct {} mutex;
void mutex_lock(struct mutex *lock)
{
	__coverity_exclusive_lock_acquire__(lock);
}

int mutex_trylock(struct mutex *lock)
{
	return trylock(lock);
}

int mutex_lock_interruptible(struct mutex *lock)
{
	int cond;
	if (cond)
	{
		__coverity_exclusive_lock_acquire__(lock);
		return 0;
	}
	return -1;
}

void mutex_unlock(struct mutex *lock)
{
	__coverity_exclusive_lock_release__(lock);
}

/*
 * rw locks.
 * TODO: the rest of include/linux/rwlock_api_smp.h
 */
typedef struct {} rwlock_t;

void __raw_read_lock(rwlock_t *lock)
{
	__coverity_exclusive_lock_acquire__(lock);
}
void __raw_read_unlock(rwlock_t *lock)
{
	__coverity_exclusive_lock_release__(lock);
}

void __raw_write_lock(rwlock_t *lock)
{
	__coverity_exclusive_lock_acquire__(lock);
}
void __raw_write_unlock(rwlock_t *lock)
{
	__coverity_exclusive_lock_release__(lock);
}

/*
 * bit spin locks
 */
void bit_spin_lock(int bitnum, unsigned long *addr)
{
		__coverity_exclusive_lock_acquire__(addr);
}

void bit_spin_unlock(int bitnum, unsigned long *addr)
{
		__coverity_exclusive_lock_release__(addr);
}

/*
 * Coverity doesn't understand about read-only memory segments in the
 * kernel, so it will evaluate kstrdup_const() and kfree_const() as
 * having different conditions for "is this memory in the r/o segment"
 * between the kstrdup_const() and the kfree_const() calls. In order
 * to ignore this, just treat all k*_const() helpers as not having
 * the read-only logic.
 */
char *kstrdup(const char *s, gfp_t gfp);
const char *kstrdup_const(const char *s, gfp_t gfp)
{
	return kstrdup(s, gfp);
}

void kfree_const(const char *s)
{
	kfree(s);
}

/*
 * Recognize special pointers.
 */
bool IS_ERR_OR_NULL(const void *ptr)
{
	if ((unsigned long)ptr == 0 ||
		(unsigned long)ptr >= (unsigned long)-4095)
	{
		/* Do not free pointer here. When using this freeing, Coverity will report
		 * many "double-free" false positives. */
		/* __coverity_free__(ptr); */
		return 1;
	}
	return 0;
}

/* kfree_skb and below */
void kfree_skb(struct sk_buff *skb)
{
	__coverity_free__(skb);
}

static void kfree_skbmem(struct sk_buff *skb)
{
	__coverity_free__(skb);
}

/* This function is called, somewhat nested, from alloc_skb */
void *kmalloc_reserve(size_t size, gfp_t flags, int node,
		      bool *pfmemalloc)
{
	void *addr = __coverity_alloc__(size);
	return addr;
}

int bit_spin_trylock(int bitnum, unsigned long *addr)
{
	return trylock(addr);
}

void __bit_spin_unlock(int bitnum, unsigned long *addr)
{
	__coverity_exclusive_lock_release__(addr);
}

static inline int epoll_mutex_lock(struct mutex *mutex, int depth,
				   bool nonblock)
{
	if (!nonblock)
	{
		__coverity_exclusive_lock_acquire__(mutex);
		return 0;
	}
	if (trylock(mutex))
		return 0;
	return -EAGAIN;
}

struct bus_type
{
	bool need_parent_lock;
};

struct device
{
	struct device *parent;
	struct bus_type *bus;
};

void __device_driver_lock(struct device *dev, struct device *parent)
{
	if (parent && dev->bus->need_parent_lock)
		__coverity_exclusive_lock_acquire__(parent);
	__coverity_exclusive_lock_acquire__(dev);
}

void __device_driver_unlock(struct device *dev, struct device *parent)
{
	__coverity_exclusive_lock_release__(dev);
	if (parent && dev->bus->need_parent_lock)
		__coverity_exclusive_lock_release__(parent);
}

typedef struct
{
	int val;
} raw_spinlock_t;

/* Coverity sometimes does not seem to go deep enough, hence, also model higher level functions */
void do_raw_spin_lock_flags(raw_spinlock_t *lock, unsigned long *flags)
{
	__coverity_exclusive_lock_acquire__(lock);
}

void do_raw_spin_unlock(raw_spinlock_t *lock)
{
	__coverity_exclusive_lock_release__(lock);
}

unsigned long _raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
	__coverity_exclusive_lock_acquire__(lock);
}

void __raw_spin_unlock(raw_spinlock_t *lock)
{
	__coverity_exclusive_lock_release__(lock);
}

unsigned long _raw_spin_lock_irq(raw_spinlock_t *lock)
{
	__coverity_exclusive_lock_acquire__(lock);
}

void __raw_spin_unlock_irq(raw_spinlock_t *lock)
{
	__coverity_exclusive_lock_release__(lock);
}

void _raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags)
{
	__coverity_exclusive_lock_release__(lock);
}

int _raw_spin_trylock(raw_spinlock_t *lock)
{
	return trylock(lock);
}

/* rht locks */
struct bucket_table
{
};
struct rhash_lock_head
{
};
struct rhash_head
{
};

void rht_lock(struct bucket_table *tbl, struct rhash_lock_head **bkt)
{
	__coverity_exclusive_lock_acquire__(bkt);
}

void rht_unlock(struct bucket_table *tbl, struct rhash_lock_head **bkt)
{
	__coverity_exclusive_lock_release__(bkt);
}

void rht_assign_unlock(struct bucket_table *tbl, struct rhash_lock_head **bkt, struct rhash_head *obj)
{
	__coverity_exclusive_lock_release__(bkt);
}
