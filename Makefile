CC = gcc
# -O3: Max optimization
# -flto: Link Time Optimization
CFLAGS = -std=gnu11 -O3 -flto -Wall -Wextra -Wno-unused-result -D_POSIX_C_SOURCE=200809L

# Bridge Configuration
RUST_DIR = std_rust
RUST_LIB = $(RUST_DIR)/target/release/libabyss_std.a

SRC = src/main.c src/utils.c src/lexer.c src/codegen.c src/symbols.c src/parser.c
OBJ = $(SRC:.c=.o)

all: rust_lib abyssc abyss_vm

# Compile the Rust Bridge
rust_lib:
	cd $(RUST_DIR) && cargo build --release

# Compile Compiler
abyssc: $(OBJ)
	$(CC) $(CFLAGS) -o abyssc $(OBJ)

# Compile VM (Link C + Rust)
abyss_vm: vm.c rust_lib
	$(CC) $(CFLAGS) -o abyss_vm vm.c $(RUST_LIB) -lpthread -ldl

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o abyssc abyss_vm *.aby
	cd $(RUST_DIR) && cargo clean
