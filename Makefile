CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra

all: vm abyssc

vm: vm.c
	$(CC) $(CFLAGS) -o abyss_vm vm.c

abyssc: abyssc.c
	$(CC) $(CFLAGS) -o abyssc abyssc.c

sample: abyssc vm sample.al
	./abyssc sample.al sample.aby
	./abyss_vm sample.aby

clean:
	rm -f abyss_vm abyssc sample.aby