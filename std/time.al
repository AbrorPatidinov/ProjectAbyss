// ═══════════════════════════════════════════════════════════
//  std.time — AbyssLang Standard Timing Library
//  High-resolution monotonic clock for benchmarking.
// ═══════════════════════════════════════════════════════════

// --- Get current monotonic timestamp (seconds) ---
function std.time.now() : (float result) {
    return clock();
}

// --- Calculate elapsed time since a start timestamp ---
function std.time.elapsed(float start) : (float result) {
    float end = clock();
    return end - start;
}
