"""Python equivalent of showcase_real.al. Run before/after the AbyssLang
native demo to compare head-to-head on the same algorithms.

Usage:
    python3 showcase_equivalent.py
"""

import time


# ──────────────────────────────────────────────────────────────────────
# Benchmark 1: Mandelbrot 800x600, max 255 iterations
# ──────────────────────────────────────────────────────────────────────
def mandelbrot_iterations(cr, ci, max_iter):
    zr = 0.0
    zi = 0.0
    i = 0
    while i < max_iter:
        zr2 = zr * zr
        zi2 = zi * zi
        if zr2 + zi2 > 4.0:
            return i
        zi = 2.0 * zr * zi + ci
        zr = zr2 - zi2 + cr
        i += 1
    return max_iter


def run_mandelbrot():
    W, H, MAX_ITER = 800, 600, 255
    t = time.time()
    total_iters = 0
    for py in range(H):
        for px in range(W):
            cr = -2.0 + 3.0 * px / W
            ci = -1.0 + 2.0 * py / H
            total_iters += mandelbrot_iterations(cr, ci, MAX_ITER)
    return time.time() - t, total_iters


# ──────────────────────────────────────────────────────────────────────
# Benchmark 2: Packet signature scanner, 500,000 packets
# ──────────────────────────────────────────────────────────────────────
BENIGN, SUSPICIOUS, MALICIOUS, CRITICAL, ISOLATED = 0, 1, 2, 4, 8


def scan_packet(src_ip, dst_port, size, payload_hash):
    level = BENIGN
    if dst_port == 22 and size > 1400:
        level |= SUSPICIOUS
    if (payload_hash & 255) == 171:
        level |= MALICIOUS
    if (level & SUSPICIOUS) and (level & MALICIOUS):
        level |= CRITICAL
    entropy = (src_ip ^ payload_hash) & 65535
    if entropy > 60000:
        level |= ISOLATED
    return level


def run_packet_scanner():
    NUM = 500_000
    MASK = 0x7FFFFFFF
    t = time.time()
    threats = 0
    crits = 0
    seed = 12345
    for _ in range(NUM):
        seed = (seed * 1103515245 + 12345) & MASK
        src_ip = seed
        seed = (seed * 1103515245 + 12345) & MASK
        dst_port = seed % 65536
        seed = (seed * 1103515245 + 12345) & MASK
        size = seed % 1500
        seed = (seed * 1103515245 + 12345) & MASK
        payload_hash = seed
        threat = scan_packet(src_ip, dst_port, size, payload_hash)
        if threat != BENIGN:
            threats += 1
            if threat & CRITICAL:
                crits += 1
    return time.time() - t, threats, crits


# ──────────────────────────────────────────────────────────────────────
# Benchmark 3: Sieve of Eratosthenes, primes below 10,000,000
# ──────────────────────────────────────────────────────────────────────
def run_sieve():
    N = 10_000_000
    t = time.time()
    sieve = [0] * N
    sieve[0] = 1
    sieve[1] = 1
    i = 2
    while i * i < N:
        if sieve[i] == 0:
            j = i * i
            while j < N:
                sieve[j] = 1
                j += i
        i += 1
    prime_count = sum(1 for x in sieve if x == 0) - 2  # exclude 0 and 1 setup
    # Actually, above already marks 0 and 1 as composite, so count includes them as composite
    prime_count = 0
    for i in range(2, N):
        if sieve[i] == 0:
            prime_count += 1
    return time.time() - t, prime_count


# ──────────────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 60)
    print("  Python 3 equivalent of showcase_real.al")
    print("=" * 60)

    print("\n[1] Mandelbrot 800x600...")
    t, iters = run_mandelbrot()
    print(f"    Iterations: {iters}")
    print(f"    Time:       {t:.3f} seconds")

    print("\n[2] Packet scanner 500,000 packets...")
    t, threats, crits = run_packet_scanner()
    print(f"    Threats:    {threats}")
    print(f"    Criticals:  {crits}")
    print(f"    Time:       {t:.3f} seconds")

    print("\n[3] Prime sieve 10,000,000...")
    t, primes = run_sieve()
    print(f"    Primes:     {primes}")
    print(f"    Time:       {t:.3f} seconds")
