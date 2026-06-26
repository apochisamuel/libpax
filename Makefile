CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g -I src/
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
all: libpax.a
libpax.a: $(OBJ)
	ar rcs $@ $^
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
test: libpax.a
	$(CC) $(CFLAGS) tests/test_basic.c -L. -lpax -lm -o test_runner && ./test_runner
clean:
	rm -f src/*.o libpax.a test_runner
