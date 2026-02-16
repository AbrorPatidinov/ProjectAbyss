CC = gcc
# -Iinclude tells the compiler to look in the include/ folder for .h files
# -D_POSIX_C_SOURCE=200809L ensures strndup and other POSIX functions are available globally
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wno-unused-result -D_POSIX_C_SOURCE=200809L


# Source files
SRC = src/main.c src/utils.c src/lexer.c src/codegen.c src/symbols.c src/parser.c
OBJ = $(SRC:.c=.o)

# Targets
all: abyssc abyss_vm

abyssc: $(OBJ)
	$(CC) $(CFLAGS) -o abyssc $(OBJ)

abyss_vm: vm.c
	$(CC) $(CFLAGS) -o abyss_vm vm.c

# Generic rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o abyssc abyss_vm *.aby
