CFLAGS += -Wall -O2 -fstrict-flex-arrays=3 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -Wno-dangling-pointer
ARRAY_SANITIZER = -fsanitize=bounds -fsanitize=object-size
MATH_SANITIZER = -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=pointer-overflow
TRUNCATION_SANITIZER = -fsanitize=implicit-unsigned-integer-truncation -fsanitize=implicit-signed-integer-truncation

ifeq ($(findstring clang,$(CC)),clang)
UBSAN_TRAP = -fsanitize-trap=all
else
UBSAN_TRAP = -fsanitize-undefined-trap-on-error
endif

NO_STRICT_OVERFLOW = -fno-strict-overflow
DEPS = Makefile harness.h kselftest.h

EXES = fortify array-bounds

all: $(EXES)
clean:
	rm -f *.o $(EXES)

fortify.o: fortify.c $(DEPS)

array-bounds.o: array-bounds.c $(DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ARRAY_SANITIZER) $(UBSAN_TRAP) -c -o $@ $<

sanitizers.o: sanitizers.c $(DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_SANITIZER) $(TRUNCATION_SANITIZER) $(UBSAN_TRAP) -c -o $@ $<
