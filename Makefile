CC = gcc
CFLAGS = -std=gnu11 -O3 -flto=auto -fno-strict-aliasing -Wall -Wextra -Wno-unused-result -D_POSIX_C_SOURCE=200809L

SRC = src/main.c src/utils.c src/lexer.c src/codegen.c src/symbols.c src/parser.c src/native.c
OBJ = $(SRC:.c=.o)

all: abyssc abyss_vm

abyssc: $(OBJ)
	$(CC) $(CFLAGS) -o abyssc $(OBJ)

abyss_vm: vm.c
	$(CC) $(CFLAGS) -o abyss_vm vm.c -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o abyssc abyss_vm *.aby abyss_native_temp.c
