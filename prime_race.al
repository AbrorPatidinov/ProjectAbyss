// ═══════════════════════════════════════════════════════════
//  Prime Counter Benchmark — AbyssLang
//  Count all primes under 10,000,000
//  Uses: std.math, std.time, std.io
// ═══════════════════════════════════════════════════════════

import std.math;
import std.time;
import std.io;

void main() {
    int limit = 10000000;
    int count = 0;

    std.io.banner("AbyssLang Prime Counter");
    print("  Target: all primes under %int", limit);
    std.io.separator();

    float start = std.time.now();

    for (int n = 2; n < limit; n++) {
        if (std.math.is_prime(n) == 1) {
            count++;
        }
    }

    float elapsed = std.time.elapsed(start);

    std.io.newline();
    print("  Primes found  : %int", count);
    print("  Time elapsed  : %float seconds", elapsed);
    std.io.separator();
    print("  Engine: AbyssLang Native AOT | Made in Uzbekistan");
    std.io.separator();
}
