import time


def is_prime(n):
    if n < 2:
        return False
    if n < 4:
        return True
    if n % 2 == 0:
        return False
    if n % 3 == 0:
        return False
    i = 5
    while i * i <= n:
        if n % i == 0:
            return False
        if n % (i + 2) == 0:
            return False
        i += 6
    return True


limit = 10_000_000
count = 0

print("═" * 52)
print("  Python 3 Prime Counter")
print("═" * 52)
print(f"  Target: all primes under {limit}")
print("─" * 52)

start = time.monotonic()

for n in range(2, limit):
    if is_prime(n):
        count += 1

elapsed = time.monotonic() - start

print()
print(f"  Primes found  : {count}")
print(f"  Time elapsed  : {elapsed:.6f} seconds")
print("─" * 52)
print("  Engine: CPython 3 Interpreter")
print("─" * 52)
