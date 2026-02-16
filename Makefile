CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wno-unused-result

all: abyssc abyss_vm

abyssc: abyssc.c
	$(CC) $(CFLAGS) -o abyssc abyssc.c

abyss_vm: vm.c
	$(CC) $(CFLAGS) -o abyss_vm vm.c

run: all
	@echo "--- Compiling sample.al ---"
	./abyssc sample.al sample.aby
	@echo "--- Running VM ---"
	./abyss_vm sample.aby

clean:
	rm -f abyssc abyss_vm *.aby
