import time

start = time.perf_counter()
s = 0

for i in range(1_000_000):
    s += i

end = time.perf_counter()

elapsed = end - start

print("Sum:", s)
print("Elapsed:", elapsed, "sec")
print("Elapsed:", elapsed * 1000, "ms")
