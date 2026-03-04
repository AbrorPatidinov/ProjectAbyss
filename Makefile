CC = gcc
CFLAGS = -std=gnu11 -O3 -flto -Wall -Wextra -Wno-unused-result -D_POSIX_C_SOURCE=200809L

RUST_DIR = std_rust
RUST_LIB = $(RUST_DIR)/target/release/libabyss_std.a

# --- ADD src/native.c HERE ---
SRC = src/main.c src/utils.c src/lexer.c src/codegen.c src/symbols.c src/parser.c src/native.c
OBJ = $(SRC:.c=.o)

all: rust_lib abyssc abyss_vm

rust_lib:
	cd $(RUST_DIR) && cargo build --release

abyssc: $(OBJ)
	$(CC) $(CFLAGS) -o abyssc $(OBJ)

abyss_vm: vm.c rust_lib
	$(CC) $(CFLAGS) -o abyss_vm vm.c $(RUST_LIB) -lpthread -ldl -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o abyssc abyss_vm *.aby abyss_native_temp.c
	cd $(RUST_DIR) && cargo clean
