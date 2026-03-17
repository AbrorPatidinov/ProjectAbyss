# benchmark.py
import time


def is_prime(n):
    if n < 2:
        return 0
    i = 2
    while i * i <= n:
        if n % i == 0:
            return 0
        i += 1
    return 1


def main():
    print("--- Python CPU Benchmark ---")
    target = 100000
    count = 0

    start_time = time.time()

    for i in range(target + 1):
        if is_prime(i) == 1:
            count += 1

    end_time = time.time()

    print(f"Total primes found up to {target}: {count}")
    print(f"Time taken: {end_time - start_time:.4f} seconds")


if __name__ == "__main__":
    main()
