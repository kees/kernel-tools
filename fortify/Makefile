CFLAGS += -Wall -O2 -fstrict-flex-arrays=3 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3
SANITIZER = -fsanitize=bounds -fsanitize=object-size -fsanitize-undefined-trap-on-error
DEPS = Makefile harness.h kselftest.h

all: fortify array-bounds
clean:
	rm -f *.o fortify array-bounds

fortify.o: fortify.c $(DEPS)

array-bounds.o: array-bounds.c $(DEPS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZER) -c -o $@ $<
